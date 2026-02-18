#pragma once
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int cmd_commit_tree(int argc, char *argv[]);
std::string commit_tree(std::string tree_hash, std::string parent_hash,
                        std::string message);
std::string getUserEmail();
std::string getUserName();
std::string getUserConfig();

std::string getTimestamp();
std::string getTimezone();
