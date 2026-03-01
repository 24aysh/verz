#include "../../include/log.h"
#include "../../include/commit.h"
#include "../../include/utils.h"
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static std::string parse_field(const std::string &body,
                               const std::string &key) {
  size_t pos = body.find(key);
  if (pos == std::string::npos)
    return "";
  size_t start = pos + key.size();
  size_t end = body.find('\n', start);
  return body.substr(start, end - start);
}

static void walk_log(int maxCount) {
  std::string sha = get_head_commit();
  if (sha.empty()) {
    std::cout << "No commits yet.\n";
    return;
  }

  int count = 0;
  while (!sha.empty()) {
    if (maxCount > 0 && count >= maxCount)
      break;

    std::string raw;
    try {
      raw = readGitObject(sha);
    } catch (const std::exception &e) {
      std::cerr << "error: cannot read commit " << sha << "\n";
      break;
    }

    size_t nullPos = raw.find('\0');
    if (nullPos == std::string::npos)
      break;
    std::string body = raw.substr(nullPos + 1);

    std::string parent = parse_field(body, "parent ");
    std::string author = parse_field(body, "author ");

    // Message is everything after the first blank line
    std::string message;
    size_t blank = body.find("\n\n");
    if (blank != std::string::npos)
      message = body.substr(blank + 2);
    while (!message.empty() && message.back() == '\n')
      message.pop_back();

    // Parse "Name <email> <timestamp> <tz>" from author line
    std::string authorName, authorEmail, dateStr;
    {
      std::istringstream ss(author);
      std::vector<std::string> tokens;
      std::string tok;
      while (ss >> tok)
        tokens.push_back(tok);

      std::string tz, ts;
      if (tokens.size() >= 2) {
        tz = tokens.back();
        tokens.pop_back();
      }
      if (tokens.size() >= 1) {
        ts = tokens.back();
        tokens.pop_back();
      }

      // Rebuild name + email
      for (auto &t : tokens) {
        if (!authorName.empty())
          authorName += ' ';
        authorName += t;
      }
      size_t lt = authorName.find('<');
      if (lt != std::string::npos) {
        authorEmail = authorName.substr(lt);
        authorName = authorName.substr(0, lt);
        while (!authorName.empty() && authorName.back() == ' ')
          authorName.pop_back();
      }

      // Format timestamp
      try {
        std::time_t t = std::stol(ts);
        char buf[64];
        if (std::strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y",
                          std::localtime(&t)))
          dateStr = std::string(buf) + " " + tz;
      } catch (...) {
        dateStr = ts + " " + tz;
      }
    }

    // Print git-log style output
    std::cout << "\033[33mcommit " << sha << "\033[0m\n";
    std::cout << "Author: " << authorName << " " << authorEmail << "\n";
    std::cout << "Date:   " << dateStr << "\n";
    std::cout << "\n    " << message << "\n\n";

    sha = parent;
    ++count;
  }
}

int cmd_log(int argc, char *argv[]) {
  if (!std::filesystem::exists(".verz")) {
    std::cerr << "fatal: not a verz repository\n";
    return EXIT_FAILURE;
  }

  int maxCount = 0;
  for (int i = 2; i < argc; i++) {
    std::string arg = argv[i];
    if ((arg == "-n" || arg == "--max-count") && i + 1 < argc) {
      try {
        maxCount = std::stoi(argv[++i]);
      } catch (...) {
      }
    } else if (arg.rfind("-n", 0) == 0 && arg.size() > 2) {
      try {
        maxCount = std::stoi(arg.substr(2));
      } catch (...) {
      }
    } else if (arg.rfind("--max-count=", 0) == 0) {
      try {
        maxCount = std::stoi(arg.substr(12));
      } catch (...) {
      }
    }
  }

  walk_log(maxCount);
  return EXIT_SUCCESS;
}
