#include "../../include/commit.h"
#include "../../include/add.h"
#include "../../include/commit_tree.h"
#include "../../include/write_tree.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static std::string resolve_head_ref_path() {
  std::ifstream headFile(".verz/HEAD");
  if (!headFile)
    throw std::runtime_error("fatal: could not open .verz/HEAD");

  std::string line;
  std::getline(headFile, line);
  // line looks like: "ref: refs/heads/master"
  const std::string prefix = "ref: ";
  if (line.rfind(prefix, 0) != 0)
    throw std::runtime_error("fatal: detached HEAD not supported");

  std::string refPath = line.substr(prefix.size());
  // strip trailing newline if any
  if (!refPath.empty() && refPath.back() == '\n')
    refPath.pop_back();

  return ".verz/" + refPath;
}

std::string get_head_commit() {
  std::string refPath = resolve_head_ref_path();
  std::ifstream f(refPath);
  if (!f)
    return "";

  std::string sha;
  std::getline(f, sha);
  if (!sha.empty() && sha.back() == '\n')
    sha.pop_back();
  return sha;
}

void update_head(const std::string &sha) {
  std::string refPath = resolve_head_ref_path();

  std::filesystem::create_directories(
      std::filesystem::path(refPath).parent_path());

  std::ofstream f(refPath, std::ios::trunc);
  if (!f)
    throw std::runtime_error("fatal: could not write " + refPath);
  f << sha << "\n";
}

int cmd_commit(int argc, char *argv[]) {
  if (!std::filesystem::exists(".verz")) {
    std::cerr << "fatal: not a verz repository (no .verz directory)\n";
    return EXIT_FAILURE;
  }

  if (!std::filesystem::exists(".verz/user/config")) {
    std::cerr << "error: user not registered\n";
    std::cerr << "Use 'verz register <username> <email>' to register\n";
    return EXIT_FAILURE;
  }

  std::string message;
  for (int i = 2; i < argc; i++) {
    std::string flag = argv[i];
    if (flag == "-m" && i + 1 < argc) {
      message = argv[++i];
    } else {
      std::cerr << "Unknown flag: " << flag << "\n";
      std::cerr << "Usage: verz commit -m <message>\n";
      return EXIT_FAILURE;
    }
  }

  if (message.empty()) {
    std::cerr << "error: commit message cannot be empty\n";
    std::cerr << "Usage: verz commit -m <message>\n";
    return EXIT_FAILURE;
  }

  std::vector<IndexEntry> index = read_index();
  if (index.empty()) {
    std::cout << "On branch master\nnothing to commit, working tree clean\n";
    return EXIT_SUCCESS;
  }

  std::string treeHash = write_tree(std::filesystem::current_path());

  std::string parentHash;
  try {
    parentHash = get_head_commit();
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  std::string commitHash = commit_tree(treeHash, parentHash, message);

  try {
    update_head(commitHash);
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  write_index({});

  std::string branch = "master";
  std::ifstream headFile(".verz/HEAD");
  if (headFile) {
    std::string line;
    std::getline(headFile, line);
    const std::string prefix = "ref: refs/heads/";
    if (line.rfind(prefix, 0) == 0)
      branch = line.substr(prefix.size());
    if (!branch.empty() && branch.back() == '\n')
      branch.pop_back();
  }

  std::cout << "[" << branch << " " << commitHash.substr(0, 7) << "] "
            << message << "\n";
  return EXIT_SUCCESS;
}
