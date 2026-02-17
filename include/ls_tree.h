#pragma once
#include <string>

struct treeObject {
  std::string mode;
  std::string type;
  std::string hash;
  std::string fileName;
};

int cmd_ls_tree(int argc, char *argv[]);
std::string binaryToHex(std::string &binary);
void ls_tree(std::string hash, std::string flag, std::string prefix = "");