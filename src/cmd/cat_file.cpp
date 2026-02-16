#include "../../include/cat_file.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <zlib.h>

static std::string getGitBlob(const std::string &filepath) {
  std::ifstream file(filepath, std::ios::binary);
  if (!file) {
    std::cerr << "Failed to open file\n";
    return "";
  }

  std::vector<char> compressed{std::istreambuf_iterator<char>(file),
                               std::istreambuf_iterator<char>()};

  z_stream stream{};
  stream.next_in = reinterpret_cast<Bytef *>(compressed.data());
  stream.avail_in = compressed.size();

  if (inflateInit(&stream) != Z_OK) {
    std::cerr << "inflateInit failed\n";
    return "";
  }

  std::string decompressed;
  char buffer[4096];

  while (true) {
    stream.next_out = reinterpret_cast<Bytef *>(buffer);
    stream.avail_out = sizeof(buffer);

    int ret = inflate(&stream, Z_NO_FLUSH);

    decompressed.append(buffer, sizeof(buffer) - stream.avail_out);

    if (ret == Z_STREAM_END)
      break;

    if (ret != Z_OK) {
      std::cerr << "inflate failed\n";
      inflateEnd(&stream);
      return "";
    }
  }

  inflateEnd(&stream);

  size_t nullPos = decompressed.find('\0');
  if (nullPos == std::string::npos) {
    std::cerr << "Invalid git object format\n";
    return "";
  }

  return decompressed.substr(nullPos + 1);
}

int cmd_cat_file(int argc, char *argv[]) {
  if (argc < 4) {
    std::cerr << "Usage: cat-file -p <sha>\n";
    return 1;
  }

  std::string hash = argv[3];
  std::string filename =
      ".git/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);

  std::string content = getGitBlob(filename);

  if (content.empty()) {
    std::cerr << "Failed to read blob\n";
    return 1;
  }

  std::cout << content;
  return 0;
}
