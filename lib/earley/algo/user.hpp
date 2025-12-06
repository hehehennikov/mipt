#pragma once

#include <earley/utils/trim.hpp>
#include <expected>

#include "grammar.hpp"
#include "iobserver.hpp"

namespace earley {

class EarleyParser {
 private:
  struct Item {
    int prod;
    int dot;
    int start;
  };

  struct ItemEq {
    bool operator()(Item const& a, Item const& b) const noexcept {
      return a.prod == b.prod && a.dot == b.dot && a.start == b.start;
    }
  };

  struct ItemHash {
    std::size_t operator()(Item const& it) const noexcept {
      auto v = (uint64_t)(it.prod + 0x9e3779b97f4a7c15ULL);
      v ^= ((uint64_t)it.dot + 0x9e3779b97f4a7c15ULL) + (v << 6) + (v >> 2);
      v ^= ((uint64_t)it.start + 0x9e3779b97f4a7c15ULL) + (v << 6) + (v >> 2);

      return v;
    }
  };

 public:
  EarleyParser() = default;

 public:
  // set user observer (optional)
  void set_observer(IParserObserver* obs) {
    observer = obs;
  }

 public:
  void fit(Grammar const& G_in) {
    G = G_in;
    G.validate_and_build();

    augmented_lhs = (int)G.nonterms.size();
    prods = G.prods;
    Grammar::Prod aug;
    aug.lhs = augmented_lhs;
    aug.rhs = std::string(1, G.nonterms[G.start]);
    prods.push_back(aug);
    augmented_prod_index = (int)prods.size() - 1;

    prods_by_lhs.assign(augmented_lhs + 1, {});
    for (int i = 0; i < (int)prods.size(); ++i) {
      const auto L = prods[i].lhs;
      if (L < 0
          || L > augmented_lhs) {
        throw std::runtime_error("EarleyParser: production lhs out of range");
      }
      prods_by_lhs[L].push_back(i);
    }
  }

  auto predict(const std::string& word) const {
    if (G.nonterms.empty()) {
      throw std::runtime_error("EarleyParser: grammar not fitted");
    }

    if (observer) {
      observer->on_parse_begin(word);
    }

    const auto& w = word;

    auto n = w.size();

    std::vector<std::vector<Item>> S(n + 1);
    std::vector<std::unordered_set<Item, ItemHash, ItemEq>> seen(n + 1);

    auto push = [&](int k, const Item& it) {
      if (seen[k].insert(it).second) {
        S[k].push_back(it);
        if (observer) {
          observer->on_add_item(k, it.prod, it.dot, it.start);
        }
        return true;
      }
      return false;
    };

    Item startItem{augmented_prod_index, 0, 0};
    push(0, startItem);

    for (std::size_t k = 0; k <= n; ++k) {
      for (std::size_t idx = 0; idx < S[k].size(); ++idx) {
        const auto cur = S[k][idx];
        const auto& rhs = prods[cur.prod].rhs;
        if (cur.dot < (int)rhs.size()) {
          char sym = rhs[cur.dot];
          if (is_nonterm(sym)) {
            // predictor
            auto A = G.nonterm_idx.at(sym);
            for (auto pj : prods_by_lhs[A]) {
              Item nit{pj, 0, (int)k};
              push((int)k, nit);
            }
          } else {
            // scanner
            if (k < n
                && w[k] == sym) {
              Item nit{cur.prod, cur.dot + 1, cur.start};
              push((int)k + 1, nit);
            }
          }
        } else {
          // completer
          auto j = cur.start;
          auto B_lhs = prods[cur.prod].lhs;

          for (std::size_t ii = 0; ii < S[j].size(); ++ii) {
            auto it2 = S[j][ii];
            const auto& rhs2 = prods[it2.prod].rhs;
            if (it2.dot < (int)rhs2.size()) {
              char expect = rhs2[it2.dot];
              if (is_nonterm(expect)
                  && G.nonterm_idx.at(expect) == B_lhs) {
                Item nit{it2.prod, it2.dot + 1, it2.start};
                push((int)k, nit);
              }
            }
          }
        }
      }
    }

    Item accept{augmented_prod_index,
                (int)prods[augmented_prod_index].rhs.size(), 0};
    auto ok = seen[n].contains(accept);

    if (observer) {
      observer->on_parse_end(word, ok);
    }
    return ok;
  }

 private:
  bool is_nonterm(char c) const noexcept {
    return G.nonterm_idx.contains(c);
  }

 private:
  Grammar G;
  std::vector<Grammar::Prod> prods;
  std::vector<std::vector<int>> prods_by_lhs;
  int augmented_lhs = -1;
  int augmented_prod_index = -1;

  // observer (not owned)
  IParserObserver* observer = nullptr;
};

}  // namespace earley