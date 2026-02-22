#include "../include/cat_file.h"
#include "../include/clone.h"
#include "../include/commit_tree.h"
#include "../include/hash_object.h"
#include "../include/init.h"
#include "../include/ls_tree.h"
#include "../include/register.h"
#include "../include/write_tree.h"
#include <iostream>

int main(int argc, char *argv[]) {

  if (argc < 2) {
    std::cerr << "Usage: verz <command> [<args>]\n";
    return 1;
  }

  std::string command = argv[1];

  if (command == "init") {
    return cmd_init();
  }

  // verz register <username> <email>
  if (command == "register") {
    return cmd_register(argc, argv);
  }

  if (command == "cat-file") {
    return cmd_cat_file(argc, argv);
  }

  if (command == "hash-object") {
    return cmd_hash_object(argc, argv);
  }

  if (command == "ls-tree") {
    return cmd_ls_tree(argc, argv);
  }

  if (command == "write-tree") {
    return cmd_write_tree();
  }

  // verz commit-tree <tree-hash> -p <parent-hash> <message>
  if (command == "commit-tree") {
    return cmd_commit_tree(argc, argv);
  }

  if (command == "clone") {
    return cmd_clone(argc, argv);
  }

  std::cerr << "Unknown command\n";
  return 1;
}
