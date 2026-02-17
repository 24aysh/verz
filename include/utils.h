#pragma once
#include <string>

std::string binaryToHex(const std::string &binary);
std::string hexToBinary(const std::string &hex);

std::string zlibCompress(const std::string &data);
std::string zlibDecompress(const std::string &compressed);

std::string readGitObject(const std::string &hash);
