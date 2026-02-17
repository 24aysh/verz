#include "../../include/ls_tree.h"
#include "../../include/utils.h"
#include <iostream>
#include <vector>

int cmd_ls_tree(int argc, char *argv[]) {
  std::string flag = "";
  std::string hash = "";
  if (argc >= 4) {
    flag = std::string(argv[2]);
    hash = std::string(argv[3]);
  } else if (argc == 3) {
    hash = std::string(argv[2]);
  } else {
    std::cerr << "Usage : verz ls-tree [-r|--name-only] <hash>\n";
    return EXIT_FAILURE;
  }
  ls_tree(hash, flag);
  return EXIT_SUCCESS;
}

void ls_tree(std::string hash, std::string flag, std::string prefix) {
  std::string decompressed = readGitObject(hash);

  size_t nullPos = decompressed.find('\0');
  if (nullPos == std::string::npos) {
    std::cerr << "Invalid tree object\n";
    exit(-1);
  }

  std::string header = decompressed.substr(0, nullPos);

  if (header.rfind("tree ", 0) != 0) {
    std::cerr << "Not a tree object (header was: " << header << ")\n";
    exit(-1);
  }

  size_t pos = nullPos + 1;
  std::vector<treeObject> entries;

  while (pos < decompressed.size()) {
    size_t spacePos = decompressed.find(' ', pos);
    if (spacePos == std::string::npos)
      break;

    std::string mode = decompressed.substr(pos, spacePos - pos);

    size_t nameNull = decompressed.find('\0', spacePos + 1);
    if (nameNull == std::string::npos)
      break;

    std::string name =
        decompressed.substr(spacePos + 1, nameNull - spacePos - 1);

    if (nameNull + 1 + 20 > decompressed.size())
      break;

    std::string binarySha = decompressed.substr(nameNull + 1, 20);
    std::string hexHash = binaryToHex(binarySha);

    std::string type = (mode == "40000" || mode == "040000") ? "tree" : "blob";

    treeObject obj;
    obj.mode = mode;
    obj.type = type;
    obj.hash = hexHash;
    obj.fileName = name;
    entries.push_back(obj);

    pos = nameNull + 1 + 20;
  }

  for (const auto &obj : entries) {
    std::string fullPath =
        prefix.empty() ? obj.fileName : prefix + "/" + obj.fileName;

    if (flag == "--name-only") {
      std::cout << fullPath << "\n";
    } else {
      std::string paddedMode = obj.mode;
      if (paddedMode.size() < 6)
        paddedMode = std::string(6 - paddedMode.size(), '0') + paddedMode;
      std::cout << paddedMode << " " << obj.type << " " << obj.hash << "\t"
                << fullPath << "\n";
    }

    if (flag == "-r" && obj.type == "tree") {
      ls_tree(obj.hash, flag, fullPath);
    }
  }
}