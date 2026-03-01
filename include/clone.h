#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <zlib.h>

const int CHUNK = 16384;

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

size_t writeCallbackPackFileReceive(void *ptr, size_t size, size_t nmemb,
                                    void *userdata);

void clone(std::string &url, std::string root);

std::vector<unsigned char>
resolveDelta(const std::vector<unsigned char> &base,
             const std::vector<unsigned char> &delta);

void handlePktLine(UploadPackParser &p, std::string &payload);

void parseRecToPktLine(UploadPackParser &p);

std::vector<uint8_t> discoverRefs(CURL *curl, std::string url);

PktLine readPktLine(std::vector<uint8_t> &responseBuffer, size_t &offset);

std::unordered_map<std::string, std::string>

readRefs(std::vector<uint8_t> &responseBuffer);

size_t writeCallbackRefDiscovery(void *ptr, size_t size, size_t nmemb,
                                 void *userdata);

UploadPackParser makeRequest(CURL *curl, const std::string &request,
                             std::string url);

std::string resolveHead(std::unordered_map<std::string, std::string> &refs);

std::string
initialiseGitRepo(const std::string &root,
                  std::unordered_map<std::string, std::string> &refs);

uint32_t read_be32(uint8_t *b);

std::string makePktLine(const std::string &payload);

void parsePackFile(UploadPackParser &p, const std::string &commitSha,
                   const std::string &root);

std::vector<unsigned char>

decompress_continuous(std::vector<uint8_t> &packfile, uint32_t &consumedBytes,
                      size_t start, uint32_t &expectedUncompressedSize);

std::vector<unsigned char>
compress_data(std::vector<unsigned char> &fullContent);

void parseTree(
    std::string &treeSha, std::string pathPrefix,
    std::unordered_map<std::string, size_t> &hashCache,
    std::unordered_map<size_t, std::vector<unsigned char>> &objectCache);
