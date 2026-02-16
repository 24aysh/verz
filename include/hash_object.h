#pragma once
#include <string>

int cmd_hash_object(int argc, char *argv[]);
std::string calcSHA1(std::string content);
void hash_object(std::string path, std::string flag);
void writeGitObject(std::string content, std::string hash);