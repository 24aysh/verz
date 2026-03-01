#pragma once
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

int cmd_branch(int argc, char *argv[]);
int cmd_switch(int argc, char *argv[]);
int cmd_delete_branch(int argc, char *argv[]);
std::string current_branch();

std::string branch_ref_path(const std::string &name);
