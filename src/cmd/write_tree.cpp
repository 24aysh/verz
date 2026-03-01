#include "../../include/write_tree.h"
#include "../../include/utils.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <vector>

int cmd_write_tree() {
  std::string hash = write_tree(std::filesystem::current_path());
  std::cout << hash << std::endl;
  return EXIT_SUCCESS;
}

std::string write_tree(std::filesystem::path path) {

  std::vector<TreeEntry> entries;
  for (const auto &entry : std::filesystem::directory_iterator(path)) {
    std::string name = entry.path().filename().string();
    if (name == ".verz") {
      continue;
    }

    TreeEntry tree_entry;
    tree_entry.name = name;

    if (std::filesystem::is_directory(entry.path())) {
      tree_entry.mode = "040000";
      tree_entry.sha_hex = write_tree(entry.path());
    } else {
      std::filesystem::perms perms = entry.status().permissions();
      bool is_exec = (perms & std::filesystem::perms::owner_exec) !=
                     std::filesystem::perms::none;
      tree_entry.mode = is_exec ? "100755" : "100644";
      std::ifstream file(entry.path(), std::ios::binary);
      std::string content((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
      tree_entry.sha_hex = createBlobObject(content, /*write=*/true);
    }

    entries.push_back(tree_entry);
  }
  std::sort(entries.begin(), entries.end());
  std::string tree_content;
  for (const auto &e : entries) {
    tree_content += e.mode + " " + e.name + '\0' + hexToBinary(e.sha_hex);
  }
  return createTreeObject(tree_content, /*write=*/true);
}