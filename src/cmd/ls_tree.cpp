#include "../../include/ls_tree.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <zlib.h>

int cmd_ls_tree(int argc, char *argv[]) {
  std::string flag = "";
  std::string hash = "";
  if (argc >= 4) {
    flag = std::string(argv[2]);
    hash = std::string(argv[3]);
  } else if (argc == 3) {
    hash = std::string(argv[2]);
  } else {
    std::cerr << "Usage : verz ls-tree [-t] <hash>\n";
    return EXIT_FAILURE;
  }
  ls_tree(hash, flag);
  return EXIT_SUCCESS;
}

void ls_tree(std::string hash, std::string flag, std::string prefix) {
  std::string filePath =
      ".verz/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);

  std::ifstream file(filePath, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open file: " + filePath);
  }

  std::vector<char> compressed((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
  file.close();

  z_stream stream = {};
  stream.next_in = reinterpret_cast<Bytef *>(compressed.data());
  stream.avail_in = compressed.size();

  if (inflateInit(&stream) != Z_OK) {
    std::cerr << "inflateInit failed\n";
    exit(-1);
  }

  std::string decompressed;
  char buffer[4096];

  while (true) {
    stream.next_out = reinterpret_cast<Bytef *>(buffer);
    stream.avail_out = sizeof(buffer);

    int ret = inflate(&stream, Z_NO_FLUSH);

    decompressed.append(buffer, sizeof(buffer) - stream.avail_out);

    if (ret == Z_STREAM_END) {
      break;
    }
    if (ret != Z_OK) {
      std::cerr << "inflate failed\n";
      inflateEnd(&stream);
      exit(-1);
    }
  }
  inflateEnd(&stream);

  // Parse header: "tree <size>\0"
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
    while (pos < decompressed.size()) {
      size_t spacePos = decompressed.find(' ', pos);
      if (spacePos == std::string::npos)
        break;

      std::string mode = decompressed.substr(pos, spacePos - pos);

      // Find the null byte separating name from the binary SHA
      size_t nameNull = decompressed.find('\0', spacePos + 1);
      if (nameNull == std::string::npos)
        break;

      std::string name =
          decompressed.substr(spacePos + 1, nameNull - spacePos - 1);

      if (nameNull + 1 + 20 > decompressed.size())
        break;
      std::string binarySha = decompressed.substr(nameNull + 1, 20);
      std::string hexHash = binaryToHex(binarySha);

      std::string type =
          (mode == "40000" || mode == "040000") ? "tree" : "blob";

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

      // Recursive traversal for -r flag
      if (flag == "-r" && obj.type == "tree") {
        ls_tree(obj.hash, flag, fullPath);
      }
    }
  }
}

std::string binaryToHex(std::string &binary) {
  const unsigned char *data =
      reinterpret_cast<const unsigned char *>(binary.c_str());
  std::stringstream ss;
  for (size_t i = 0; i < binary.size(); i++) {
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(data[i]);
  }
  return ss.str();
}