#include "../../include/register.h"

int cmd_register(int argc, char *argv[]) {
  if (argc != 4) {
    std::cerr << "Usage: verz register <username> <email>\n";
    return EXIT_FAILURE;
  }
  registerUserName(argv[2]);
  registerUserEmail(argv[3]);
  return EXIT_SUCCESS;
}

void registerUserName(std::string username) {
  std::string dir = ".verz/user/";
  std::filesystem::create_directories(dir);

  std::ofstream out(dir + "config", std::ios::binary);
  out.write(username.data(), username.size());
  out << "\n";
  out.close();
}

void registerUserEmail(std::string email) {
  std::ofstream out(".verz/user/config", std::ios::binary | std::ios::app);
  out.write(email.data(), email.size());
  out << "\n";
  out.close();
}