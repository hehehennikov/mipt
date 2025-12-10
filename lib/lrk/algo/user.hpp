#pragma once

#include <map>
#include <set>
#include <stack>
#include <vector>

#include <stdexcept>

#include <memory>

#include "first_set.hpp"
#include "grammar.hpp"

namespace lrk::algo {

struct Item {
  auto operator<=>(const Item&) const = default;

  size_t prod_idx;
  size_t dot_pos;
  Word lookahead;
};

using State = std::set<Item>;

enum class ActionType {
  Shift,
  Reduce,
  Accept
};

struct Action {
  bool operator==(const Action&) const = default;

  ActionType type;
  int value;
};

class LRKParser {
 public:
  explicit LRKParser(int k = 1)
      : k_(k) {}

  void Fit(const Grammar& input_g);
  bool Predict(const std::string& word) const;

 private:
  State Closure(const State& I) const;
  State GoToState(const State& I, SymbolID X) const;
  void BuildTables();

 private:
  int k_;
  Grammar G_;
  Grammar augmented_G_;

  std::vector<std::map<Word, Action>> action_table_;
  std::vector<std::map<SymbolID, int>> goto_table_;

  std::unique_ptr<FirstKComputer> first_computer_;
};

inline void LRKParser::Fit(const Grammar& input_g) {
  if (k_ < 0) {
    throw std::runtime_error("Invalid k");
  }

  G_ = input_g;
  augmented_G_ = input_g;

  auto old_start = augmented_G_.start_symbol;
  auto new_start = AUG_START_ID;

  augmented_G_.nonterm_map['@'] = new_start;
  augmented_G_.id_to_char[new_start] = '@';
  augmented_G_.start_symbol = new_start;

  augmented_G_.AddProd(new_start, {old_start});

  auto effective_k = std::max(1, k_);
  first_computer_ = std::make_unique<FirstKComputer>(augmented_G_, effective_k);

  BuildTables();
}

inline State LRKParser::Closure(const State& I) const {
  State J = I;
  bool changed = true;
  while (changed) {
    changed = false;
    State new_items;
    for (const auto& [prod_idx, dot_pos, lookahead] : J) {
      const auto& [lhs, rhs] = augmented_G_.prods[prod_idx];

      if (dot_pos >= rhs.size()) {
        continue;
      }

      auto B = rhs[dot_pos];
      if (Grammar::IsTerminal(B)) {
        continue;
      }

      std::vector<SymbolID> beta;
      for (std::size_t i = dot_pos + 1; i < rhs.size(); ++i) {
        beta.push_back(rhs[i]);
      }

      auto first_beta = first_computer_->GetFirst(beta);
      std::set<Word> lookahead_set;
      lookahead_set.insert(lookahead);

      auto new_lookaheads =
          first_computer_->Ops().ConcatSets(first_beta, lookahead_set);

      for (std::size_t p_idx = 0; p_idx < augmented_G_.prods.size(); ++p_idx) {
        if (augmented_G_.prods[p_idx].lhs == B) {
          for (const auto& w : new_lookaheads) {
            Item newItem{p_idx, 0, w};
            if (not J.contains(newItem)
                && not new_items.contains(newItem)) {
              new_items.insert(newItem);
              changed = true;
            }
          }
        }
      }
    }

    J.insert(new_items.begin(), new_items.end());
  }

  return J;
}

inline State LRKParser::GoToState(const State& I, SymbolID X) const {
  State J;
  for (const auto& [prod_idx, dot_pos, lookahead] : I) {
    if (const auto& [lhs, rhs] = augmented_G_.prods[prod_idx];
        dot_pos < rhs.size()
        && rhs[dot_pos] == X) {
      J.insert({prod_idx, dot_pos + 1, lookahead});
    }
  }

  return Closure(J);
}

inline void LRKParser::BuildTables() {
  auto effective_k = std::max(1, k_);

  Word start_lookahead(effective_k, END_MARKER);
  auto start_prod_idx = augmented_G_.prods.size() - 1;
  auto start_state = Closure({{start_prod_idx, 0, start_lookahead}});

  std::vector<State> C;
  std::map<State, int> state_map;

  C.push_back(start_state);
  state_map[start_state] = 0;

  action_table_.clear();
  goto_table_.clear();

  for (std::size_t i = 0; i < C.size(); ++i)
  {
    if (action_table_.size() <= i) {
      action_table_.resize(i + 1);
      goto_table_.resize(i + 1);
    }

    std::set<SymbolID> transition_syms;
    for (const auto& item : C[i]) {
      if (const auto& [lhs, rhs] = augmented_G_.prods[item.prod_idx];
          item.dot_pos < rhs.size()) {
        transition_syms.insert(rhs[item.dot_pos]);
          }
    }

    for (auto X : transition_syms) {
      auto next = GoToState(C[i], X);
      if (next.empty()) {
        continue;
      }

      if (not state_map.contains(next)) {
        state_map[next] = (int)C.size();
        C.push_back(next);
      }

      auto next_idx = state_map[next];

      if (Grammar::IsTerminal(X)) {
        for (const auto& [prod_idx, dot_pos, lookahead] : C[i]) {
          if (const auto& [lhs, rhs] = augmented_G_.prods[prod_idx];
              dot_pos < rhs.size()
              && rhs[dot_pos] == X) {
            std::vector<SymbolID> tail;
            for (std::size_t p = dot_pos + 1; p < rhs.size(); ++p) {
              tail.push_back(rhs[p]);
            }

            std::set<Word> la_set;
            la_set.insert(lookahead);

            auto tail_first = first_computer_->GetFirst(tail);
            auto full_tail =
                first_computer_->Ops().ConcatSets(tail_first, la_set);

            std::set<Word> u_set;
            for (auto w : full_tail) {
              Word uw;

              uw.push_back(X);
              uw.insert(uw.end(), w.begin(), w.end());
              uw = first_computer_->Ops().Truncate(uw);
              u_set.insert(uw);
            }

            for (const auto& u : u_set) {
              if (action_table_[i].contains(u)) {
                if (auto [type, value] = action_table_[i][u];
                    type == ActionType::Shift) {
                  if (value != next_idx) {
                    throw std::runtime_error(
                        "LR(k) Conflict: Shift/Shift to different states");
                  }
                    } else if (type == ActionType::Reduce) {
                      action_table_[i][u] = {ActionType::Shift, next_idx};
                    } else {
                      throw std::runtime_error(
                          "LR(k) Conflict: unexpected existing Accept");
                    }
              } else {
                action_table_[i][u] = {ActionType::Shift, next_idx};
              }
            }
              }
        }
      } else {
        goto_table_[i][X] = next_idx;
      }
    }

    for (const auto& [prod_idx, dot_pos, lookahead] : C[i]) {
      if (const auto& [lhs, rhs] = augmented_G_.prods[prod_idx];
          dot_pos == rhs.size()) {
        if (lhs == augmented_G_.start_symbol) {
          auto correct_end = true;
          for (auto s : lookahead) {
            if (s != END_MARKER) {
              correct_end = false;
            }
          }

          if (correct_end) {
            action_table_[i][lookahead] = {ActionType::Accept, 0};

            continue;
          }
        }

        if (k_ == 0) {
          for (const auto& tid : augmented_G_.term_map | std::views::values) {
            auto key = first_computer_->Ops().Truncate(Word{tid});
            if (action_table_[i].contains(key)) {
              auto [type, value] = action_table_[i][key];
              if (type == ActionType::Shift) {
                continue;
              }
              if (type == ActionType::Reduce &&
                  value != (int)prod_idx)
                throw std::runtime_error("LR(0) Conflict: Reduce/Reduce");
            }
            action_table_[i][key] = {ActionType::Reduce, (int)prod_idx};
          }
          if (auto end_key = first_computer_->Ops().Truncate(Word{END_MARKER});
              action_table_[i].contains(end_key)) {
            if (auto [type, value] = action_table_[i][end_key]; type == ActionType::Shift) {
            } else if (type == ActionType::Reduce &&
                       value != (int)prod_idx) {
              throw std::runtime_error("LR(0) Conflict: Reduce/Reduce");
                       } else {
                         action_table_[i][end_key] = {ActionType::Reduce,
                                                      (int)prod_idx};
                       }
              } else {
                action_table_[i][end_key] = {ActionType::Reduce,
                                             (int)prod_idx};
              }
        } else {
          if (action_table_[i].contains(lookahead)) {
            auto [type, value] = action_table_[i][lookahead];
            if (type == ActionType::Shift) {
              continue;
            }
            if (type == ActionType::Reduce &&
                value != (int)prod_idx)
              throw std::runtime_error("LR(k) Conflict: Reduce/Reduce");
          }
          action_table_[i][lookahead] = {ActionType::Reduce,
                                              (int)prod_idx};
        }
      }
    }
  }
}

inline bool LRKParser::Predict(const std::string& word) const {
  std::stack<int> state_stack;
  state_stack.push(0);

  auto effective_k = std::max(1, k_);

  Word input;
  for (auto c : word) {
    if (not G_.term_map.contains(c)) {
      return false;
    }

    input.push_back(G_.term_map.at(c));
  }
  for (int i = 0; i < effective_k; ++i) {
    input.push_back(END_MARKER);
  }

  std::size_t ip = 0;
  while (true) {
    if (state_stack.empty()) {
      return false;
    }
    auto s = state_stack.top();

    Word u;
    for (std::size_t j = 0; j < (std::size_t)effective_k; ++j) {
      if (ip + j < input.size()) {
        u.push_back(input[ip + j]);
      } else {
        u.push_back(END_MARKER);
      }
    }

    if (not action_table_[s].contains(u)) {
      return false;
    }

    if (auto [type, value] = action_table_[s].at(u);
        type == ActionType::Shift) {
      state_stack.push(value);
      ip++;
    } else if (type == ActionType::Reduce) {
      const auto& [lhs, rhs] = augmented_G_.prods[value];
      for (std::size_t l = 0; l < rhs.size(); ++l) {
        if (state_stack.empty()) {
          return false;
        }

        state_stack.pop();
      }
      if (state_stack.empty()) {
        return false;
      }

      auto top = state_stack.top();

      if (not goto_table_[top].contains(lhs)) {
        return false;
      }

      state_stack.push(goto_table_[top].at(lhs));
    } else if (type == ActionType::Accept) {
      return true;
    }
  }
}

}  // namespace lrk::algo
