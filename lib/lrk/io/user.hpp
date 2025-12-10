#pragma once

#include <lrk/utils/trim.hpp>
#include <lrk/algo/grammar.hpp>
#include <lrk/algo/user.hpp>

namespace lrk::io {

inline auto ReadProblem(std::istream& in) {
    auto read_line = [&] {
        std::string s;
        if (not std::getline(in, s)) {
            throw std::runtime_error("Unexpected end");
        }

        return utils::Trim(s);
    };

    int N, P;
    if (int Sigma; !(in >> N >> Sigma >> P)) {
        throw std::runtime_error("Read error");
    }
    std::string tmp;
    std::getline(in, tmp);

    algo::Grammar G;

    std::string nt_line = read_line();
    for (auto c : nt_line) {
        if (not std::isspace(c)) {
            if (not G.nonterm_map.contains(c)) {
                auto id = (int)G.nonterm_map.size();

                G.nonterm_map[c] = id;
                G.id_to_char[id] = c;
            }
        }
    }

    for (auto t_line = read_line();
         auto c : t_line) {
        if (not std::isspace(c)) {
            if (not G.term_map.contains(c)) {
                auto id = algo::TERM_START_ID + (int)G.term_map.size();

                G.term_map[c] = id;
                G.id_to_char[id] = c;
            }
        }
    }

    for (int i = 0; i < P; ++i) {
        auto line = read_line();
        if (line.empty()) {
            continue;
        }

        auto pos = line.find("->");
        if (pos == std::string::npos) {
            throw std::runtime_error("No ->");
        }

        auto lhs_s = utils::Trim(line.substr(0, pos));
        if (lhs_s.size() != 1) {
            throw std::runtime_error("LHS must be 1 char");
        }

        auto lhs_c = lhs_s[0];

        auto rhs_s = utils::Trim(line.substr(pos + 2));

        auto lhs = G.nonterm_map.at(lhs_c);
        algo::Word rhs;
        for (auto c : rhs_s) {
            if (std::isspace(c)) {
                continue;
            }
            if (G.nonterm_map.contains(c)) {
                rhs.push_back(G.nonterm_map.at(c));
            } else if (G.term_map.contains(c)) {
                rhs.push_back(G.term_map.at(c));
            } else {
                throw std::runtime_error("Unknown symbol in RHS");
            }
        }

        G.AddProd(lhs, rhs);
    }

    auto start_s = read_line();
    if (start_s.empty()) {
        throw std::runtime_error("No start");
    }
    G.start_symbol = G.nonterm_map.at(start_s[0]);

    auto m = std::stoi(read_line());
    std::vector<std::string> words;
    for (int i = 0; i < m; ++i) {
        std::string w;
        std::getline(in, w);

        words.push_back(utils::Trim(w));
    }

    return std::pair{std::move(G), std::move(words)};
}

inline void ProcessAndWrite(const algo::Grammar& G, const std::vector<std::string>& words, std::ostream& out) {
#ifdef EARLEY
    static constexpr int K = 0;
#endif

    algo::LRKParser parser(K);

    try {
        parser.Fit(G);
    } catch (...) {
        throw;
    }

    for (const auto& w : words) {
        out << (parser.Predict(w) ? "Yes" : "No") << "\n";
    }
}

} // namespace lrk::io