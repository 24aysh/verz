#pragma once
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int cmd_commit(int argc, char *argv[]);

std::string get_head_commit();

void update_head(const std::string &sha);
