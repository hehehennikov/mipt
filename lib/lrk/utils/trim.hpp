#pragma once

#include <string>

#include <sstream>

#include <ranges>

#include <cctype>

namespace lrk::utils {

inline std::string Trim(const std::string& s) {
  auto is_space = [](unsigned char c){
      return std::isspace(c);
  };

  return s
         | std::views::drop_while(is_space)
         | std::views::reverse
         | std::views::drop_while(is_space)
         | std::views::reverse
         | std::ranges::to<std::string>();
}

} // namespace lrk::utils