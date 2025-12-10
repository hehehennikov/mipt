#include <iostream>

#ifdef EARLEY

#include <earley/algo/user.hpp>
#include <earley/io/user.hpp>

#endif

#ifdef LRK

#include <lrk/algo/user.hpp>
#include <lrk/io/user.hpp>

#endif

int main() {
  try {
#ifdef EARLEY
    auto [G, words] = earley::io::ReadProblem(std::cin);
    earley::io::ProcessAndWrite(G, words, std::cout);
#endif

#ifdef LRK
    auto [G, words] = lrk::io::ReadProblem(std::cin);
    lrk::io::ProcessAndWrite(G, words, std::cout);
#endif
  } catch (const std::exception& ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 2;
  }
  return 0;
}