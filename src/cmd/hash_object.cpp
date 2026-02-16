#include "../../include/hash_object.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <zlib.h>

std::string calcSHA1(std::string content) {
  unsigned char hash[SHA_DIGEST_LENGTH];
  SHA1(reinterpret_cast<const unsigned char *>(content.c_str()), content.size(),
       hash);

  std::stringstream ss;
  for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(hash[i]);
  }
  return ss.str();
}

void writeGitObject(std::string content, std::string hash) {

  z_stream stream{};
  deflateInit(&stream, Z_DEFAULT_COMPRESSION);

  stream.next_in =
      reinterpret_cast<Bytef *>(const_cast<char *>(content.data()));
  stream.avail_in = content.size();

  std::string compressed;
  char buffer[4096];

  int ret;
  do {
    stream.next_out = reinterpret_cast<Bytef *>(buffer);
    stream.avail_out = sizeof(buffer);

    ret = deflate(&stream, Z_FINISH);
    compressed.append(buffer, sizeof(buffer) - stream.avail_out);
  } while (ret == Z_OK);

  deflateEnd(&stream);

  std::string dir = ".git/objects/" + hash.substr(0, 2);
  std::filesystem::create_directories(dir);

  std::ofstream out(dir + "/" + hash.substr(2));
  out.write(compressed.data(), compressed.size());
  out.close();
}

void hash_object(std::string path, std::string flag) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open file\n");
  }
  std::string content{std::istreambuf_iterator<char>(file),
                      std::istreambuf_iterator<char>()};
  std::string header = "blob " + std::to_string(content.size());
  header += '\0';
  std::string object = header + content;
  std::string hash = calcSHA1(object);
  std::cout << hash << "\n";

  if (flag == "-w") {
    writeGitObject(object, hash);
  }
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
