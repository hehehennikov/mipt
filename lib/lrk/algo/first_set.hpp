#pragma once

#include <map>
#include <set>
#include <vector>

#include <algorithm>
#include <ranges>

#include "grammar.hpp"

namespace lrk::algo {

class KLanguageOps {
 public:
  explicit KLanguageOps(int k)
      : k_(k) {}

  [[nodiscard]]
  Word Truncate(Word w) const {
    if ((int)w.size() > k_) {
      w.resize(k_);
    }
    while ((int)w.size() < k_) {
      w.push_back(END_MARKER);
    }

    return w;
  }

 private:
  static Word Significant(const Word& w) {
    Word s;
    for (auto x : w) {
      if (x == END_MARKER) {
        break;
      }

      s.push_back(x);
    }

    return s;
  }

 public:
  [[nodiscard]]
  Word ConcatWords(const Word& w1, const Word& w2) const {
    auto a = Significant(w1);
    auto b = Significant(w2);
    auto res = a;

    res.insert(res.end(), b.begin(), b.end());
    if ((int)res.size() > k_) {
      res.resize(k_);
    }
    while ((int)res.size() < k_) {
      res.push_back(END_MARKER);
    }

    return res;
  }

  [[nodiscard]]
  std::set<Word> ConcatSets(const std::set<Word>& A,
                            const std::set<Word>& B) const {
    std::set<Word> result;
    if (A.empty() || B.empty()) {
      return result;
    }

    for (const auto& w1 : A) {
      for (const auto& w2 : B) {
        result.insert(ConcatWords(w1, w2));
      }
    }

    return result;
  }

 private:
  int k_;
};

class FirstKComputer {
 public:
  FirstKComputer(const Grammar& G, int k)
      : G_(G), ops_(k) {
    Compute();
  }

  [[nodiscard]]
  auto GetFirst(const std::vector<SymbolID>& sequence) const {
    std::set<Word> result;
    result.insert(ops_.Truncate(Word{}));

    for (auto sym : sequence) {
      std::set<Word> sym_first;
      if (Grammar::IsTerminal(sym)) {
        sym_first.insert(ops_.Truncate(Word{sym}));
      } else {
        if (not first_sets_.contains(sym)) {
          sym_first = {};
        } else {
          sym_first = first_sets_.at(sym);
        }
      }
      result = ops_.ConcatSets(result, sym_first);

      if (result.empty()) {
        break;
      }
    }
    return result;
  }

 public:
  [[nodiscard]]
  const KLanguageOps& Ops() const {
    return ops_;
  }

 private:
  void Compute() {
    for (const auto& id : G_.nonterm_map | std::views::values) {
      first_sets_[id] = {};
    }

    if (G_.nonterm_map.contains('@')) {
      first_sets_[G_.nonterm_map.at('@')] = {};
    }

    auto changed = true;
    while (changed) {
      changed = false;
      for (const auto& [lhs, rhs] : G_.prods) {
        std::set<Word> rhs_first;
        rhs_first.insert(ops_.Truncate(Word{}));

        for (SymbolID sym : rhs) {
          std::set<Word> next_set;
          if (Grammar::IsTerminal(sym)) {
            next_set.insert(ops_.Truncate(Word{sym}));
          } else {
            next_set = first_sets_[sym];
          }

          rhs_first = ops_.ConcatSets(rhs_first, next_set);
        }

        auto size_before = first_sets_[lhs].size();
        first_sets_[lhs].insert(rhs_first.begin(), rhs_first.end());

        if (first_sets_[lhs].size() > size_before) {
          changed = true;
        }
      }
    }
  }

 private:
  const Grammar& G_;
  KLanguageOps ops_;
  std::map<SymbolID, std::set<Word>> first_sets_;
};

}  // namespace lrk::algo