#include "../../include/add.h"
#include "../../include/utils.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Index persistence
// ---------------------------------------------------------------------------

std::vector<IndexEntry> read_index() {
  std::vector<IndexEntry> entries;
  std::ifstream f(".verz/index");
  if (!f)
    return entries; // empty index is fine

  std::string line;
  while (std::getline(f, line)) {
    if (line.empty())
      continue;
    std::istringstream ss(line);
    IndexEntry e;
    ss >> e.mode >> e.sha_hex >> e.path;
    if (!e.path.empty())
      entries.push_back(e);
  }
  return entries;
}

void write_index(const std::vector<IndexEntry> &entries) {
  std::ofstream f(".verz/index", std::ios::trunc);
  if (!f) {
    std::cerr << "error: could not write .verz/index\n";
    return;
  }
  for (const auto &e : entries)
    f << e.mode << " " << e.sha_hex << " " << e.path << "\n";
}

// ---------------------------------------------------------------------------
// Staging helpers
// ---------------------------------------------------------------------------

static bool should_skip(const std::filesystem::path &p) {
  for (const auto &part : p) {
    const std::string s = part.string();
    if (s == ".verz" || s == ".git")
      return true;
  }
  return false;
}

void stage_path(const std::filesystem::path &target,
                const std::filesystem::path &root,
                std::vector<IndexEntry> &entries) {
  if (should_skip(target))
    return;

  if (std::filesystem::is_directory(target)) {
    for (const auto &de :
         std::filesystem::recursive_directory_iterator(target)) {
      if (de.is_regular_file() && !should_skip(de.path())) {
        stage_path(de.path(), root, entries);
      }
    }
    return;
  }

  if (!std::filesystem::is_regular_file(target)) {
    std::cerr << "warning: '" << target.string()
              << "' is not a regular file, skipping\n";
    return;
  }

  // Read file content and hash it
  std::ifstream file(target, std::ios::binary);
  if (!file) {
    std::cerr << "error: cannot open '" << target.string() << "'\n";
    return;
  }
  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
  std::string sha = createBlobObject(content, /*write=*/true);

  // Determine mode
  std::filesystem::perms perms = std::filesystem::status(target).permissions();
  bool is_exec = (perms & std::filesystem::perms::owner_exec) !=
                 std::filesystem::perms::none;
  std::string mode = is_exec ? "100755" : "100644";

  // Repo-relative path
  std::string relPath = std::filesystem::relative(target, root).string();

  // Update existing entry or append new one
  auto it =
      std::find_if(entries.begin(), entries.end(),
                   [&](const IndexEntry &e) { return e.path == relPath; });
  if (it != entries.end()) {
    it->sha_hex = sha;
    it->mode = mode;
  } else {
    entries.push_back({mode, sha, relPath});
  }
}

// ---------------------------------------------------------------------------
// Command entry point
// ---------------------------------------------------------------------------

int cmd_add(int argc, char *argv[]) {
  if (!std::filesystem::exists(".verz")) {
    std::cerr << "fatal: not a verz repository (no .verz directory)\n";
    return EXIT_FAILURE;
  }

  if (argc < 3) {
    std::cerr << "Usage: verz add <pathspec>...\n";
    return EXIT_FAILURE;
  }

  std::filesystem::path root = std::filesystem::current_path();
  std::vector<IndexEntry> entries = read_index();

  for (int i = 2; i < argc; i++) {
    std::string arg = argv[i];
    std::filesystem::path target;

    if (arg == ".") {
      target = root;
    } else {
      target = std::filesystem::path(arg);
      if (target.is_relative())
        target = root / target;
    }

    if (!std::filesystem::exists(target)) {
      std::cerr << "fatal: pathspec '" << arg << "' did not match any files\n";
      return EXIT_FAILURE;
    }

    stage_path(target, root, entries);
  }

  // Sort by path for determinism
  std::sort(
      entries.begin(), entries.end(),
      [](const IndexEntry &a, const IndexEntry &b) { return a.path < b.path; });

  write_index(entries);
  return EXIT_SUCCESS;
}
