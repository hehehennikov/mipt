#include <iostream>

#include <earley/algo/user.hpp>

#include <earley/io/user.hpp>

int main() {
  try {
    auto [G, words] = earley::io::read_problem(std::cin);
    earley::io::process_and_write(G, words, std::cout);
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << std::endl;
    return 2;
  }
  return 0;
}