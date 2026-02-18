#include "../../include/commit_tree.h"
#include "../../include/utils.h"
#include <cstddef>
#include <string>

int cmd_commit_tree(int argc, char *argv[]) {
  if (!std::filesystem::exists(".verz/user/config")) {
    std::cerr << "User not registered\n";
    std::cerr << "Use 'verz register <username> <email>' to register\n";
    return EXIT_FAILURE;
  }

  if (argc < 3) {
    std::cerr << "Usage: verz commit-tree <tree-hash> [-p <parent-hash>] -m "
                 "<message>\n";
    return EXIT_FAILURE;
  }

  std::string tree_hash = argv[2];
  std::string parent_hash = "";
  std::string message = "";

  // Parse flags: -p <parent-hash> and -m <message>
  for (int i = 3; i < argc; i++) {
    std::string flag = argv[i];
    if (flag == "-p" && i + 1 < argc) {
      parent_hash = argv[++i];
    } else if (flag == "-m" && i + 1 < argc) {
      message = argv[++i];
    } else {
      std::cerr << "Unknown flag: " << flag << "\n";
      std::cerr << "Usage: verz commit-tree <tree-hash> [-p <parent-hash>] -m "
                   "<message>\n";
      return EXIT_FAILURE;
    }
  }

  std::cout << commit_tree(tree_hash, parent_hash, message) << std::endl;
  return EXIT_SUCCESS;
}

std::string commit_tree(std::string treeHash, std::string parentHash,
                        std::string message) {

  std::string content = "";
  content += "tree ";
  content += treeHash;
  content += "\n";

  if (!parentHash.empty()) {
    content += "parent ";
    content += parentHash;
    content += "\n";
  }

  std::string authorLine = "";
  authorLine += getUserName();
  authorLine += " ";
  authorLine += getUserEmail();
  authorLine += " ";
  authorLine += getTimestamp();
  authorLine += " ";
  authorLine += getTimezone();

  content += "author ";
  content += authorLine;
  content += "\n";

  content += "committer ";
  content += authorLine;
  content += "\n";

  content += "\n";
  content += message;
  content += "\n";

  // Build the full object: "commit <size>\0<content>"
  std::string object = "commit ";
  object += std::to_string(content.size());
  object += '\0';
  object += content;

  std::string hash = calcSHA1(object);
  writeGitObject(object, hash);
  return hash;
}

std::string getUserConfig() {
  std::ifstream file(".verz/user/config", std::ios::binary);
  std::string content;

  if (!file) {
    throw std::runtime_error("Failed to open file : .verz/user/config");
  }

  content = std::string((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
  file.close();
  return content;
}

std::string getUserEmail() {
  std::string email = "";
  std::string config = getUserConfig();
  size_t pos = config.find("\n");
  if (pos == std::string::npos) {
    throw std::runtime_error("Invalid verz user config\n");
  }
  email = config.substr(pos + 1);
  if (!email.empty() && email.back() == '\n') {
    email.pop_back();
  }
  email = "<" + email + ">";
  return email;
}

std::string getUserName() {
  std::string name = "";
  std::string config = getUserConfig();
  size_t pos = config.find("\n");
  if (pos == std::string::npos) {
    throw std::runtime_error("Invalid verz user config\n");
  }
  name = config.substr(0, pos);
  if (!name.empty() && name.back() == '\n') {
    name.pop_back();
  }
  return name;
}

std::string getTimestamp() {
  std::time_t now = std::time(nullptr);
  return std::to_string(now);
}

std::string getTimezone() {
  std::time_t now = std::time(nullptr);
  std::tm *local = std::localtime(&now);
  char buf[7]; // +HHMM\0
  std::strftime(buf, sizeof(buf), "%z", local);
  return std::string(buf);
}
