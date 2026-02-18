#pragma once
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int cmd_register(int argc, char *argv[]);
void registerUserName(std::string username);
void registerUserEmail(std::string email);
