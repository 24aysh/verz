#pragma once
#include <string>

// Conversion utilities
std::string binaryToHex(const std::string &binary);
std::string hexToBinary(const std::string &hex);

// Compression utilities
std::string zlibCompress(const std::string &data);
std::string zlibDecompress(const std::string &compressed);

// Hash utilities
std::string calcSHA1(const std::string &content);

// Git object path utilities
std::string getObjectPath(const std::string &hash);
bool objectExists(const std::string &hash);

std::string readGitObject(const std::string &hash);
void writeGitObject(const std::string &content, const std::string &hash);
std::string createGitObject(const std::string &type, const std::string &content,
                            bool write = false);
std::string createBlobObject(const std::string &content, bool write = false);
std::string createTreeObject(const std::string &entries, bool write = false);
