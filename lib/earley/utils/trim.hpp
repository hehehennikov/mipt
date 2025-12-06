#pragma once

#include <string>
#include <vector>

#include <sstream>

namespace earley::utils {

inline std::string Trim(const std::string& sv) {
  std::size_t a = 0;
  auto b = sv.size();
  while (a < b
         && std::isspace((unsigned char)sv[a])) {
    ++a;
  }
  while (b > a && std::isspace((unsigned char)sv[b - 1])) {
    --b;
  }

  return sv.substr(a, b - a);
}

inline auto SplitWs(const std::string& s) {
  std::vector<std::string> out;
  std::istringstream iss(s);
  for (std::string tok; iss >> tok;) {
    out.push_back(tok);
  }

  return out;
}

}  // namespace earley::utils