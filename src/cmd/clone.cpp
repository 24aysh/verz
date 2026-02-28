#include "../../include/clone.h"
#include "../../include/utils.h"

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

std::vector<unsigned char>
resolveDelta(const std::vector<unsigned char> &base,
             const std::vector<unsigned char> &delta) {
  int deltaOffset = 0;
  uint64_t sourceSize = 0, targetSize = 0;

  uint8_t b = delta[deltaOffset++];
  sourceSize = b & 0x7F;
  int shift = 7;
  while (b & 0x80) {
    b = delta[deltaOffset++];
    sourceSize |= (b & 0x7F) << shift;
    shift += 7;
  }

  b = delta[deltaOffset++];
  targetSize = b & 0x7F;
  shift = 7;
  while (b & 0x80) {
    b = delta[deltaOffset++];
    targetSize |= (b & 0x7F) << shift;
    shift += 7;
  }

  if (sourceSize != base.size()) {
    std::cout << "Source size expected: " << sourceSize
              << ", actual: " << base.size() << "\n";
    throw std::runtime_error("Base size mismatch");
  }

  std::vector<unsigned char> result(targetSize);
  size_t resultOffset = 0;

  while (deltaOffset < delta.size()) {
    uint8_t opcode = delta[deltaOffset++];
    if (opcode & 0x80) {
      uint64_t copyOffset = 0, copySize = 0;
      if (opcode & 0x01)
        copyOffset |= delta[deltaOffset++];
      if (opcode & 0x02)
        copyOffset |= delta[deltaOffset++] << 8;
      if (opcode & 0x04)
        copyOffset |= delta[deltaOffset++] << 16;
      if (opcode & 0x08)
        copyOffset |= delta[deltaOffset++] << 24;
      if (opcode & 0x10)
        copySize |= delta[deltaOffset++];
      if (opcode & 0x20)
        copySize |= delta[deltaOffset++] << 8;
      if (opcode & 0x40)
        copySize |= delta[deltaOffset++] << 16;
      for (size_t i = 0; i < copySize; i++) {
        result[resultOffset++] = base[copyOffset + i];
      }
    } else {
      uint64_t insertSize = opcode & 0x7F;
      for (size_t i = 0; i < insertSize; i++) {
        result[resultOffset++] = delta[deltaOffset++];
      }
    }
  }

  return result;
}

void parsePackFile(UploadPackParser &p, const std::string &commitSha,
                   const std::string &root) {
  if (memcmp(p.packfile.data(), "PACK", 4) != 0) {
    throw std::runtime_error("Invalid packet");
  }

  uint32_t version = read_be32(p.packfile.data() + 4);
  uint32_t nObjects = read_be32(p.packfile.data() + 8);

  size_t offset = 12;
  std::unordered_map<size_t, std::vector<unsigned char>> objectCache;
  std::unordered_map<std::string, size_t> hashCache;
  std::unordered_map<size_t, uint8_t> typeCache;
  std::vector<unsigned char> decompressed;

  for (size_t i = 0; i < nObjects; i++) {
    uint32_t uncompressedSize = 0, consumedBytes = 0;
    uint8_t type = 0;
    size_t objOffset = offset;

    uint8_t firstByte = p.packfile[offset++];
    type = (firstByte >> 4) & 0x07;

    offset++;
    while (firstByte & 0x80) {
      firstByte = p.packfile[offset];
      uncompressedSize <<= 7;
      uncompressedSize |= (firstByte & 0x7F);
      offset++;
    }
    if (type <= 4) {
      decompressed = decompress_continuous(p.packfile, consumedBytes, offset,
                                           uncompressedSize);

      objectCache[objOffset] = decompressed;
      typeCache[objOffset] = type;
      std::string header;
      switch (type) {
      case 1:
        header = "commit " + std::to_string(decompressed.size()) + '\0';
        break;
      case 2:
        header = "tree " + std::to_string(decompressed.size()) + '\0';
        break;
      case 3:
        header = "blob " + std::to_string(decompressed.size()) + '\0';
        break;
      case 4:
        header = "tag " + std::to_string(decompressed.size()) + '\0';
        break;
      default:
        throw std::runtime_error("Unknown object type");
      }

      std::vector<unsigned char> fullContent;
      fullContent.insert(fullContent.end(), header.begin(), header.end());
      fullContent.insert(fullContent.end(), decompressed.begin(),
                         decompressed.end());
      std::vector<unsigned char> compressedData = compress_data(fullContent);
      std::array<unsigned char, 20> hash;
      SHA1(fullContent.data(), fullContent.size(), hash.data());
      std::string shaHex =
          binaryToHex(reinterpret_cast<const char *>(hash.data()));
      hashCache[shaHex] = objOffset;
      std::string objectHash = createGitObject(
          std::to_string(type),
          reinterpret_cast<const char *>(compressedData.data()), true);
      offset += consumedBytes;
    } else {
      if (type == 6) {
        // ofs-delta
        uint64_t ofs = 0;
        uint8_t b;

        do {
          b = p.packfile[offset++];
          ofs = (ofs << 7) | (b & 0x7F);
          if (b & 0x80) {
            ofs++;
          }
        } while (b & 0x80);

        size_t baseOffset = objOffset - ofs;
        typeCache[objOffset] = typeCache[baseOffset];
        std::string header;
        decompressed = decompress_continuous(p.packfile, consumedBytes, offset,
                                             uncompressedSize);
        std::vector<unsigned char> targetObject =
            resolveDelta(objectCache[baseOffset], decompressed);

        switch (typeCache[baseOffset]) {
        case 1:
          header = "commit " + std::to_string(targetObject.size()) + '\0';
          break;
        case 2:
          header = "tree " + std::to_string(targetObject.size()) + '\0';
          break;
        case 3:
          header = "blob " + std::to_string(targetObject.size()) + '\0';
          break;
        case 4:
          header = "tag " + std::to_string(targetObject.size()) + '\0';
          break;
        default:
          throw std::runtime_error("Unknown object type");
        }
        std::vector<unsigned char> fullContent;

        fullContent.insert(fullContent.end(), header.begin(), header.end());
      }
    }
  }
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

size_t writeCallbackRefDiscovery(void *ptr, size_t size, size_t nmemb,
                                 void *userdata) {
  size_t bytes = size * nmemb;
  auto *buf = static_cast<std::vector<uint8_t> *>(userdata);
  uint8_t *data = static_cast<uint8_t *>(ptr);

  buf->insert(buf->end(), data, data + bytes);
  return bytes;
}

std::vector<uint8_t> discoverRefs(CURL *curl, std::string url) {
  std::vector<uint8_t> responseBuffer;

  url += ".git/info/refs?service=git-upload-pack";
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "mygit/0.1");
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
  // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallbackRefDiscovery);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    throw std::runtime_error("Failed to discover refs");
  }
  curl_easy_reset(curl);
  return responseBuffer;
}

void handlePktLine(UploadPackParser &p, std::string &payload) {
  if (p.phase == Phase::READ_ACK) {
    if (payload.substr(0, 3) == "NAK") {
      p.phase = Phase::READ_SIDE_BAND;
      return;
    }
    return;
  }

  if (p.phase == Phase::READ_SIDE_BAND) {
    char band = payload[0];
    const char *data = payload.data() + 1;
    size_t len = payload.size() - 1;

    switch (band) {
    case '\x01':
      p.packfile.insert(p.packfile.end(), data, data + len);
      break;
    case '\x02':
      break;
    case '\x03':
      throw std::runtime_error(std::string("Remote error : ") +
                               std::string(data, len));
    }
  }
}
void parseRecToPktLine(UploadPackParser &p) {
  size_t offset = 0;

  while (true) {
    if (p.buf.size() - offset < 4)
      break;

    char lenbuf[5];
    memcpy(lenbuf, p.buf.data() + offset, 4);
    lenbuf[4] = '\0';
    int len = std::strtol(lenbuf, nullptr, 16);
    if (len == 0) {
      offset += 4;
      p.phase = Phase::DONE;
      continue;
    }

    if (p.buf.size() - offset < (size_t)len)
      break;

    std::string payload(reinterpret_cast<char *>(p.buf.data() + offset + 4),
                        len - 4);
    handlePktLine(p, payload);

    offset += len;
  }

  p.buf.erase(p.buf.begin(), p.buf.begin() + offset);
}

size_t writeCallbackPackFileReceive(void *ptr, size_t size, size_t nmemb,
                                    void *userdata) {
  size_t bytes = size * nmemb;
  auto *p = static_cast<UploadPackParser *>(userdata);
  auto *data = static_cast<uint8_t *>(ptr);

  p->buf.insert(p->buf.end(), data, data + bytes);
  parseRecToPktLine(*p);

  return bytes;
}

UploadPackParser makeRequest(CURL *curl, const std::string &request,
                             std::string url) {
  UploadPackParser response;

  url += ".git/git-upload-pack";
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.data());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request.size());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallbackPackFileReceive);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_HTTPGET, 0L);
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "mygit/0.1");
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

  struct curl_slist *headers = nullptr;
  headers = curl_slist_append(
      headers, "Content-Type: application/x-git-upload-pack-request");
  headers = curl_slist_append(headers,
                              "Accept: application/x-git-upload-pack-result");
  headers = curl_slist_append(headers, "Expect: ");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  CURLcode res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    throw std::runtime_error("Failed to make request");
  }
  curl_easy_reset(curl);

  return response;
}

void clone(std::string &url, std::string root) {
  curl_global_init(CURL_GLOBAL_DEFAULT);
  CURL *curl = curl_easy_init();
  std::vector<uint8_t> responseBuffer = discoverRefs(curl, url);
  std::cout << "Discovered refs:\n";
  std::unordered_map<std::string, std::string> refs = readRefs(responseBuffer);
  std::cout << "Refs read\n";
  std::string head = resolveHead(refs);
  std::string oid = refs[head];
  oid.erase(--oid.end());

  std::string request =
      makePktLine("want " + oid + " side-band-64k ofs-delta thin-pack\n");
  request += makePktLine("done\n");

  request += "0000";
  request += makePktLine("done\n");

  std::cout << "Package negotiation done, sending request ...\n";

  UploadPackParser packfile = makeRequest(curl, request, url);

  std::cout << "Packfile received, parsing...\n";
  std::string commitSha = initialiseGitRepo(root, refs);
  parsePackFile(packfile, commitSha, root);

  curl_easy_cleanup(curl);
  curl_global_cleanup();
}