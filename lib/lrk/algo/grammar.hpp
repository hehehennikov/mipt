#pragma once

#include <unordered_map>
#include <vector>

namespace lrk::algo {

using SymbolID = int;
using Word = std::vector<SymbolID>;

constexpr SymbolID END_MARKER = -2;

constexpr SymbolID TERM_START_ID = 1000;
constexpr SymbolID AUG_START_ID = 100000;

struct Production {
  auto operator<=>(const Production&) const = default;

  SymbolID lhs;
  Word rhs;
};

class Grammar {
 public:
  static bool IsTerminal(SymbolID s) {
    if (s == END_MARKER) {
      return true;
    }
    if (s == AUG_START_ID) {
      return false;
    }
    return (s >= TERM_START_ID && s < AUG_START_ID);
  }

  std::size_t AddProd(SymbolID lhs, Word rhs) {
    prods.push_back({lhs, std::move(rhs)});

    return prods.size() - 1;
  }

 public:
  std::vector<Production> prods;

  std::unordered_map<char, SymbolID> term_map;
  std::unordered_map<char, SymbolID> nonterm_map;

  std::unordered_map<SymbolID, char> id_to_char;

  SymbolID start_symbol = 0;
};

}  // namespace lrk::algo
