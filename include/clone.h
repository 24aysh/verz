#pragma once
#include <cstdint>
#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

enum class Phase { READ_ACK, READ_SIDE_BAND, DONE };

struct UploadPackParser {
  Phase phase = Phase::READ_ACK;
  std::vector<uint8_t> buf;
  std::vector<uint8_t> packfile;
};

struct PktLine {
  bool flush;
  std::string data;
};

int cmd_clone(int argc, char *argv[]);
void clone(std::string url, std::string root);
PktLine readPktLine(std::vector<uint8_t> &responseBuffer, size_t &offset);
std::unordered_map<std::string, std::string>
readRefs(std::vector<uint8_t> &responseBuffer);
std::string resolveHead(std::unordered_map<std::string, std::string> &refs);
std::string
initialiseGitRepo(const std::string &root,
                  std::unordered_map<std::string, std::string> &refs);
uint32_t read_be32(uint8_t *b);
