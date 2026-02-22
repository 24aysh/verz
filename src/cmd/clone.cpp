#include "../../include/clone.h"
#include <filesystem>
#include <fstream>
#include <string>

PktLine readPktLine(std::vector<uint8_t> &responseBuffer, size_t &offset) {
  if (offset + 4 > responseBuffer.size()) {
    throw std::runtime_error("Unexpected EOF");
  }
  std::string lenStr(reinterpret_cast<const char *>(&responseBuffer[offset]),
                     4);
  offset += 4;

  int len = std::stoi(lenStr, nullptr, 16);

  if (len == 0)
    return {1, ""};

  int payloadLen = len - 4;
  std::string payload(reinterpret_cast<const char *>(&responseBuffer[offset]),
                      payloadLen);
  offset += payloadLen;

  return {0, payload};
}

std::unordered_map<std::string, std::string>
readRefs(std::vector<uint8_t> &responseBuffer) {
  std::unordered_map<std::string, std::string> refs;
  size_t offset = 0;

  PktLine pkt =
      readPktLine(responseBuffer, offset);   // # service=git-upload-pack
  pkt = readPktLine(responseBuffer, offset); // flush

  pkt = readPktLine(responseBuffer,
                    offset); // First line with capabilities and stuff
  auto nullPos = pkt.data.find('\0');

  std::string refPart = pkt.data.substr(0, nullPos);

  auto space = refPart.find(' ');
  std::string oid = refPart.substr(0, space);
  std::string refName = refPart.substr(space + 1);
  oid.push_back(' ');
  refs[refName] = oid;

  while (true) {
    auto pkt = readPktLine(responseBuffer, offset);
    if (pkt.flush)
      break;

    auto space = pkt.data.find(' ');
    std::string oid = pkt.data.substr(0, space);
    std::string refName = pkt.data.substr(space + 1);
    refName = refName.substr(0, refName.find('\n'));

    refs[refName] = oid;
  }

  return refs;
}

std::string resolveHead(std::unordered_map<std::string, std::string> &refs) {
  std::string head;

  for (auto &[x, y] : refs) {
    if (*(--y.end()) == ' ') {
      head = x;
      break;
    }
  }

  return head;
}

std::string
initialiseGitRepo(const std::string &root,
                  std::unordered_map<std::string, std::string> &refs) {
  std::filesystem::create_directory(root);
  std::filesystem::create_directory(root + "/.git");
  std::filesystem::create_directory(root + "/.git/objects");
  std::filesystem::create_directory(root + "/.git/refs");
  std::filesystem::create_directory(root + "/.git/refs/heads");

  std::ofstream headFile(root + "/.git/HEAD");
  headFile << "ref: refs/heads/master\n";
  headFile.close();

  std::string commitSha;
  if (refs.count("refs/heads/master")) {
    commitSha = refs["refs/heads/master"];
    std::ofstream masterRefFile(root + "/.git/refs/heads/master");
    masterRefFile << commitSha << "\n";
    masterRefFile.close();
  } else if (refs.count("refs/heads/main")) {
    commitSha = refs["refs/heads/main"];
    std::ofstream mainRefFile(root + "/.git/refs/heads/main");
    mainRefFile << commitSha << "\n";
    mainRefFile.close();

    std::ofstream headFile(root + "/.git/HEAD");
    headFile << "ref: refs/heads/main\n";
    headFile.close();
  } else {
    throw std::runtime_error("Neither master nor main branch found in refs");
  }

  return commitSha;
}

uint32_t read_be32(uint8_t *b) {
  return (uint32_t(b[0]) << 24) | (uint32_t(b[1]) << 16) |
         (uint32_t(b[2]) << 8) | (uint32_t(b[3]));
}

std::string makePktLine(const std::string &payload) {
  uint16_t len = payload.size() + 4;
  char buf[5];
  snprintf(buf, sizeof(buf), "%04x", len);
  return std::string(buf) + payload;
}