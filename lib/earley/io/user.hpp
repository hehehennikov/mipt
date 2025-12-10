#pragma once

#include <bits/stdc++.h>

#include <earley/algo/user.hpp>

namespace earley::io {

inline auto ReadProblem(std::istream& in) {
  auto read_header = [&] {
    int N;
    int Sigma;
    int P;
    if (not (in >> N >> Sigma >> P)) {
      throw std::runtime_error("Failed to read N Sigma P");
    }

    std::string tmp;
    std::getline(in, tmp);

    return std::tuple{N, Sigma, P};
  };

  auto read_line_trim = [&] {
    std::string s;
    if (not std::getline(in, s)) {
      throw std::runtime_error("Unexpected end of input while reading a line");
    }

    return utils::Trim(s);
  };

  auto parse_symbol_list = [&](int expected_count, std::string const& line,
                               const char kind_desc) {
    auto tokens = utils::SplitWs(line);
    std::vector<std::string> out;
    if ((int)tokens.size() == expected_count) {
      out = tokens;
      return out;
    }
    if ((int)line.size() == expected_count) {
      out.reserve(expected_count);
      for (auto c : line) {
        out.emplace_back(1, c);
      }

      return out;
    }
    if (expected_count == 0) {
      return out;  // empty
    }
    throw std::runtime_error(std::string("Symbol list has wrong format for ") + kind_desc);
  };

  auto parse_production_line = [&](std::string const& line,
                                   earley::Grammar& G) {
    auto s = utils::Trim(line);

    if (s.empty()) {
      return;
    }

    std::size_t pos = s.find("->");
    if (pos == std::string::npos) {
      throw std::runtime_error("Production missing '->'");
    }

    auto lhs = utils::Trim(s.substr(0, pos));
    auto rhs = utils::Trim(s.substr(pos + 2));

    if (lhs.size() != 1
        || !std::isupper((unsigned char)lhs[0])) {
      throw std::runtime_error("Invalid LHS in production");
    }
    auto L = lhs[0];
    if (not G.nonterm_idx.contains(L)) {
      throw std::runtime_error(std::string("LHS symbol not declared: ") + L);
    }
    std::string rhs_compact;
    rhs_compact.reserve(rhs.size());

    for (auto c : rhs) {
      if (not std::isspace((unsigned char)c)) {
        rhs_compact.push_back(c);
      }
    }
    for (auto c : rhs_compact) {
      if (not (std::isupper((unsigned char)c)
          || G.term_set.contains(c))) {
        throw std::runtime_error(std::string("Symbol in RHS not declared: ") + c);
      }
    }
    Grammar::Prod p;
    p.lhs = G.nonterm_idx.at(L);
    p.rhs = std::move(rhs_compact);
    G.prods.push_back(std::move(p));
  };

  auto [N, Sigma, P] = read_header();

  auto nonterms_line = read_line_trim();
  auto nt_list = parse_symbol_list(N, nonterms_line, 'N');

  auto terms_line = read_line_trim();
  auto t_list = parse_symbol_list(Sigma, terms_line, 'T');

  Grammar G;
  G.nonterms.reserve(nt_list.size());
  for (auto const& s : nt_list) {
    if (s.size() != 1
        || not std::isupper((unsigned char)s[0])) {
      throw std::runtime_error("Invalid nonterminal symbol");
    }
    auto c = s[0];
    if (G.nonterm_idx.contains(c)) {
      throw std::runtime_error("Duplicate nonterminal");
    }

    G.nonterm_idx.emplace(c, (int)G.nonterms.size());
    G.nonterms.push_back(c);
  }

  for (auto const& s : t_list) {
    if (s.size() != 1) {
      throw std::runtime_error("Terminal must be single char");
    }
    auto c = s[0];
    G.term_set.insert(c);
  }

  int read_prods = 0;
  while (read_prods < P) {
    std::string line;
    if (not std::getline(in, line)) {
      throw std::runtime_error("Missing production line");
    }

    auto tline = utils::Trim(line);
    if (tline.empty()) {
      continue;
    }
    parse_production_line(tline, G);
    ++read_prods;
  }

  auto start_line = read_line_trim();
  if (start_line.empty()) {
    throw std::runtime_error("Start symbol missing");
  }
  if (start_line.size() != 1
      || not std::isupper((unsigned char)start_line[0])) {
    throw std::runtime_error("Invalid start symbol");
  }

  auto S0 = start_line[0];
  if (not G.nonterm_idx.contains(S0)) {
    throw std::runtime_error("Start symbol not declared");
  }
  G.start = G.nonterm_idx.at(S0);

  auto mline = read_line_trim();
  if (mline.empty()) {
    throw std::runtime_error("m line empty");
  }

  int m = 0;
  try {
    m = std::stoi(mline);
  } catch (...) {
    throw std::runtime_error("Invalid m");
  }
  if (m < 0) {
    throw std::runtime_error("Invalid m");
  }

  std::vector<std::string> words;
  words.reserve(static_cast<std::size_t>(m));
  for (int i = 0; i < m; ++i) {
    std::string w;
    if (not std::getline(in, w)) {
      w.clear();
    }

    words.push_back(utils::Trim(w));
  }

  G.validate_and_build();

  return std::pair{std::move(G), std::move(words)};
}

inline void ProcessAndWrite(Grammar const& G,
                              std::vector<std::string> const& words,
                              std::ostream& out) {
  EarleyParser parser;
  parser.set_observer(nullptr);
  parser.fit(G);

  auto is_valid_word = [&](std::string const& w) -> bool {
    return std::ranges::all_of(w, [&](char c){
      return G.term_set.contains(c);
    });
  };

  for (auto const& w : words) {
    if (not is_valid_word(w)) {
      out << "No\n";
      continue;
    }
    auto accepted = parser.predict(w);
    out << (accepted ? "Yes\n" : "No\n");
  }
}

}  // namespace earley::io