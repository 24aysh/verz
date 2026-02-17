#include "../../include/utils.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <zlib.h>

std::string binaryToHex(const std::string &binary) {
  const unsigned char *data =
      reinterpret_cast<const unsigned char *>(binary.c_str());
  std::stringstream ss;
  for (size_t i = 0; i < binary.size(); i++) {
    ss << std::hex << std::setw(2) << std::setfill('0')
       << static_cast<int>(data[i]);
  }
  return ss.str();
}

std::string hexToBinary(const std::string &hex) {
  std::string binary;
  for (size_t i = 0; i < hex.length(); i += 2) {
    std::string byteString = hex.substr(i, 2);
    char byte = static_cast<char>(std::stoi(byteString, nullptr, 16));
    binary.push_back(byte);
  }
  return binary;
}

std::string zlibCompress(const std::string &data) {
  z_stream stream{};
  deflateInit(&stream, Z_DEFAULT_COMPRESSION);

  stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(data.data()));
  stream.avail_in = data.size();

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
  return compressed;
}

std::string zlibDecompress(const std::string &compressed) {
  z_stream stream{};
  stream.next_in =
      reinterpret_cast<Bytef *>(const_cast<char *>(compressed.data()));
  stream.avail_in = compressed.size();

  if (inflateInit(&stream) != Z_OK) {
    throw std::runtime_error("inflateInit failed");
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
      inflateEnd(&stream);
      throw std::runtime_error("inflate failed");
    }
  }

  inflateEnd(&stream);
  return decompressed;
}

std::string readGitObject(const std::string &hash) {
  std::string filePath =
      ".verz/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);

  std::ifstream file(filePath, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Failed to open file: " + filePath);
  }

  std::vector<char> compressed{std::istreambuf_iterator<char>(file),
                               std::istreambuf_iterator<char>()};
  file.close();

  std::string compressedStr(compressed.begin(), compressed.end());
  return zlibDecompress(compressedStr);
}
