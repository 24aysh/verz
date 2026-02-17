#include "../../include/hash_object.h"
#include "../../include/utils.h"
#include <fstream>
#include <iostream>

void hash_object(std::string path, std::string flag) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open file\n");
  }
  std::string content{std::istreambuf_iterator<char>(file),
                      std::istreambuf_iterator<char>()};
  file.close();

  bool shouldWrite = (flag == "-w");
  std::string hash = createBlobObject(content, shouldWrite);

  std::cout << hash << "\n";
}

int cmd_hash_object(int argc, char *argv[]) {
  std::string flag = "";
  std::string filePath = "";

  if (argc >= 4 && std::string(argv[2]) == "-w") {
    flag = "-w";
    filePath = "./" + std::string(argv[3]);
  } else if (argc == 3) {
    filePath = "./" + std::string(argv[2]);
  } else {
    std::cerr << "Usage : verz hash-object [-w] <file>\n";
    return EXIT_FAILURE;
  }
  hash_object(filePath, flag);
  return EXIT_SUCCESS;
}
