#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace earley {

struct Grammar {
 public:
  struct Prod {
     int lhs;
     std::string rhs;
   };  // rhs is sequence of chars (terminals or nonterms)

 public:
  void validate_and_build() const {
    if (nonterms.empty()) {
      throw std::runtime_error("Grammar: no nonterminals");
    }
    if (start < 0 || start >= (int)nonterms.size()) {
      throw std::runtime_error("Grammar: invalid start symbol");
    }
    for (const auto& p : prods) {
      if (p.lhs < 0 || p.lhs >= (int)nonterms.size()) {
        throw std::runtime_error("Grammar: production lhs out of range");
      }
      for (char c : p.rhs) {
        if (not(std::isupper((unsigned char)c) || term_set.contains(c))) {
          throw std::runtime_error(
              std::string("Grammar: unknown symbol in RHS: ") + c);
        }
      }
    }
  }

 public:
  std::vector<char> nonterms;                 // index -> symbol
  std::unordered_map<char, int> nonterm_idx;  // symbol -> index
  std::unordered_set<char> term_set;          // set of terminal symbols
  std::vector<Prod> prods;                    // list of productions
  int start = -1;                             // index into nonterms
};

}  // namespace earley