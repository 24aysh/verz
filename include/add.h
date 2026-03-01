#pragma once
#include <filesystem>
#include <string>
#include <vector>

struct IndexEntry {
  std::string mode;
  std::string sha_hex;
  std::string path;
};

int cmd_add(int argc, char *argv[]);

void stage_path(const std::filesystem::path &target,
                const std::filesystem::path &root,
                std::vector<IndexEntry> &entries);

std::vector<IndexEntry> read_index();
void write_index(const std::vector<IndexEntry> &entries);
