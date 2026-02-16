#include "../../include/init.h"
#include <filesystem>
#include <fstream>
#include <iostream>

int cmd_init() {
  try {
    std::filesystem::create_directory(".verz");
    std::filesystem::create_directory(".verz/objects");
    std::filesystem::create_directory(".verz/refs");

    std::ofstream headFile(".verz/HEAD");
    headFile << "ref: refs/heads/main\n";

    std::cout << "Initialized verz directory\n";
    return 0;

  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}
