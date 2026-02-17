#pragma once
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

struct TreeEntry {
  std::string mode;
  std::string sha_hex;
  std::string name;

  bool operator<(const TreeEntry &other) const { return name < other.name; }
};
int cmd_write_tree();
std::string write_tree(std::filesystem::path path);
