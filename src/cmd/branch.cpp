#include "../../include/branch.h"
#include "../../include/commit.h"
#include "../../include/utils.h"

std::string branch_ref_path(const std::string &name) {
  return ".verz/refs/heads/" + name;
}

std::string current_branch() {
  std::ifstream f(".verz/HEAD");
  if (!f)
    throw std::runtime_error("fatal: not a verz repository");
  std::string line;
  std::getline(f, line);
  const std::string prefix = "ref: refs/heads/";
  if (line.rfind(prefix, 0) != 0)
    return ""; // detached HEAD
  std::string name = line.substr(prefix.size());
  if (!name.empty() && name.back() == '\n')
    name.pop_back();
  return name;
}

static std::string read_branch_sha(const std::string &refPath) {
  std::ifstream f(refPath);
  if (!f)
    return "";
  std::string sha;
  std::getline(f, sha);
  if (!sha.empty() && sha.back() == '\n')
    sha.pop_back();
  return sha;
}

static void checkout_commit(const std::string &commitSha,
                            const std::filesystem::path &root);

static void checkout_tree(const std::string &treeSha,
                          const std::filesystem::path &dir) {
  std::string rawTree = readGitObject(treeSha);

  size_t nullPos = rawTree.find('\0');
  if (nullPos == std::string::npos)
    throw std::runtime_error("Malformed tree object: " + treeSha);
  std::string treeData = rawTree.substr(nullPos + 1);

  std::filesystem::create_directories(dir);

  size_t pos = 0;
  while (pos < treeData.size()) {
    // mode (e.g. "100644" or "40000")
    size_t spacePos = treeData.find(' ', pos);
    if (spacePos == std::string::npos)
      break;
    std::string mode = treeData.substr(pos, spacePos - pos);
    pos = spacePos + 1;

    // name (null-terminated)
    size_t nullPos2 = treeData.find('\0', pos);
    if (nullPos2 == std::string::npos)
      break;
    std::string name = treeData.substr(pos, nullPos2 - pos);
    pos = nullPos2 + 1;

    // 20-byte binary SHA
    if (pos + 20 > treeData.size())
      break;
    std::string binSha = treeData.substr(pos, 20);
    pos += 20;
    std::string sha = binaryToHex(binSha);

    std::filesystem::path entryPath = dir / name;

    if (mode == "40000" || mode == "040000") {
      std::filesystem::create_directories(entryPath);
      checkout_tree(sha, entryPath);
    } else {
      // blob — read raw object, strip header, write file
      std::string rawBlob = readGitObject(sha);
      size_t blobNull = rawBlob.find('\0');
      std::string content = (blobNull != std::string::npos)
                                ? rawBlob.substr(blobNull + 1)
                                : rawBlob;

      std::ofstream out(entryPath, std::ios::binary | std::ios::trunc);
      if (!out)
        throw std::runtime_error("Cannot write: " + entryPath.string());
      out.write(content.data(), content.size());
    }
  }
}

static void checkout_commit(const std::string &commitSha,
                            const std::filesystem::path &root) {
  if (commitSha.empty())
    return; // nothing to check out on a brand-new branch

  std::string rawCommit = readGitObject(commitSha);
  // strip header
  size_t n = rawCommit.find('\0');
  std::string commitContent =
      (n != std::string::npos) ? rawCommit.substr(n + 1) : rawCommit;

  // Parse "tree <sha>\n..."
  if (commitContent.rfind("tree ", 0) != 0)
    throw std::runtime_error("Cannot parse commit object: " + commitSha);
  std::string treeSha = commitContent.substr(5, 40);

  // Remove all tracked files/dirs (everything except .verz)
  for (const auto &entry : std::filesystem::directory_iterator(root)) {
    std::string name = entry.path().filename().string();
    if (name == ".verz")
      continue;
    std::filesystem::remove_all(entry.path());
  }

  checkout_tree(treeSha, root);
}
int cmd_branch(int argc, char *argv[]) {
  if (!std::filesystem::exists(".verz")) {
    std::cerr << "fatal: not a verz repository\n";
    return EXIT_FAILURE;
  }

  if (argc < 3) {
    std::string active = current_branch();
    std::filesystem::path heads = ".verz/refs/heads";
    if (!std::filesystem::exists(heads)) {
      std::cout << "(no branches yet)\n";
      return EXIT_SUCCESS;
    }
    std::vector<std::string> names;
    for (const auto &e : std::filesystem::directory_iterator(heads))
      names.push_back(e.path().filename().string());
    std::sort(names.begin(), names.end());
    for (const auto &n : names)
      std::cout << (n == active ? "* " : "  ") << n << "\n";
    return EXIT_SUCCESS;
  }

  std::string name = argv[2];

  // Validate name
  if (name.find('/') != std::string::npos ||
      name.find(' ') != std::string::npos) {
    std::cerr << "fatal: invalid branch name '" << name << "'\n";
    return EXIT_FAILURE;
  }

  std::string refPath = branch_ref_path(name);
  if (std::filesystem::exists(refPath)) {
    std::cerr << "fatal: A branch named '" << name << "' already exists\n";
    return EXIT_FAILURE;
  }

  // Get current HEAD sha
  std::string headSha = get_head_commit();
  if (headSha.empty()) {
    std::cerr << "fatal: not a valid object name: 'HEAD'"
                 " (no commits on current branch)\n";
    return EXIT_FAILURE;
  }

  std::filesystem::create_directories(".verz/refs/heads");
  std::ofstream f(refPath);
  if (!f) {
    std::cerr << "fatal: cannot create branch ref\n";
    return EXIT_FAILURE;
  }
  f << headSha << "\n";
  std::cout << "Branch '" << name << "' created at " << headSha.substr(0, 7)
            << "\n";
  return EXIT_SUCCESS;
}

int cmd_switch(int argc, char *argv[]) {
  if (!std::filesystem::exists(".verz")) {
    std::cerr << "fatal: not a verz repository\n";
    return EXIT_FAILURE;
  }

  if (argc < 3) {
    std::cerr << "Usage: verz switch <branch>\n";
    return EXIT_FAILURE;
  }

  std::string name = argv[2];
  std::string refPath = branch_ref_path(name);

  if (!std::filesystem::exists(refPath)) {
    std::cerr << "error: pathspec '" << name
              << "' did not match any branch known to verz\n";
    return EXIT_FAILURE;
  }

  std::string currentBranch = current_branch();
  if (currentBranch == name) {
    std::cout << "Already on '" << name << "'\n";
    return EXIT_SUCCESS;
  }

  // Update .verz/HEAD
  std::ofstream headFile(".verz/HEAD", std::ios::trunc);
  if (!headFile) {
    std::cerr << "fatal: cannot write .verz/HEAD\n";
    return EXIT_FAILURE;
  }
  headFile << "ref: refs/heads/" << name << "\n";
  headFile.close();

  // Check out the branch's commit into the working tree
  std::string branchSha = read_branch_sha(refPath);
  try {
    checkout_commit(branchSha, std::filesystem::current_path());
  } catch (const std::exception &e) {
    std::cerr << "error: " << e.what() << "\n";
    return EXIT_FAILURE;
  }

  std::cout << "Switched to branch '" << name << "'\n";
  return EXIT_SUCCESS;
}

int cmd_delete_branch(int argc, char *argv[]) {
  if (!std::filesystem::exists(".verz")) {
    std::cerr << "fatal: not a verz repository\n";
    return EXIT_FAILURE;
  }

  if (argc < 3) {
    std::cerr << "Usage: verz delete-branch <branch>\n";
    return EXIT_FAILURE;
  }

  std::string name = argv[2];

  if (current_branch() == name) {
    std::cerr << "error: Cannot delete the branch '" << name
              << "' which you are currently on\n";
    return EXIT_FAILURE;
  }

  std::string refPath = branch_ref_path(name);
  if (!std::filesystem::exists(refPath)) {
    std::cerr << "error: branch '" << name << "' not found\n";
    return EXIT_FAILURE;
  }

  std::string sha = read_branch_sha(refPath);
  std::filesystem::remove(refPath);
  std::cout << "Deleted branch '" << name << "' (was " << sha.substr(0, 7)
            << ")\n";
  return EXIT_SUCCESS;
}
