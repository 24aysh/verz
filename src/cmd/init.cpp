#include "../../include/init.h"
#include <filesystem>
#include <fstream>
#include <iostream>

int cmd_init() {
  try {
    std::filesystem::create_directory(".git");
    std::filesystem::create_directory(".git/objects");
    std::filesystem::create_directory(".git/refs");

    std::ofstream headFile(".git/HEAD");
    headFile << "ref: refs/heads/main\n";

    std::cout << "Initialized git directory\n";
    return 0;

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
