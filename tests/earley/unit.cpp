#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include <earley/algo/user.hpp>

#include <earley/io/user.hpp>

TEST(EarleyIOTests, ExampleFromStatement) {
    const std::string input =
        "1 2 2\n"
        "S\n"
        "ab\n"
        "S-> aSbS\n"
        "S -> \n"
        "S\n"
        "2\n"
        "aababb\n"
        "aabbba\n";
    const std::string expected = "Yes\nNo\n";

    std::istringstream in(input);
    std::ostringstream out;

    auto [G, words] = earley::io::read_problem(in);
    earley::io::process_and_write(G, words, out);

    EXPECT_EQ(out.str(), expected);
}

TEST(EarleyIOTests, EpsilonAndNonAlphabet) {
    const std::string input =
        "1 0 1\n"
        "S\n"
        "\n"
        "S -> \n"       // production S -> epsilon
        "S\n"
        "2\n"
        "\n"            // first word: epsilon
        "a\n";          // second word: 'a' not in terminals, should be No
    const std::string expected = "Yes\nNo\n";

    std::istringstream in(input);
    std::ostringstream out;

    auto [G, words] = earley::io::read_problem(in);
    earley::io::process_and_write(G, words, out);

    EXPECT_EQ(out.str(), expected);
}

TEST(EarleyIOTests, SimpleConcatenation) {
    const std::string input =
        "1 2 1\n"
        "S\n"
        "ab\n"
        "S->ab\n"
        "S\n"
        "2\n"
        "ab\n"
        "a\n";
    const std::string expected = "Yes\nNo\n";

    std::istringstream in(input);
    std::ostringstream out;

    auto [G, words] = earley::io::read_problem(in);
    earley::io::process_and_write(G, words, out);

    EXPECT_EQ(out.str(), expected);
}

TEST(EarleyIOTests, LeftRecursiveGrammarManyAs) {
    const std::string input =
        "1 1 2\n"
        "S\n"
        "a\n"
        "S->SS\n"
        "S->a\n"
        "S\n"
        "3\n"
        "a\n"
        "aa\n"
        "\n"; // epsilon
    const std::string expected = "Yes\nYes\nNo\n";

    std::istringstream in(input);
    std::ostringstream out;

    auto [G, words] = earley::io::read_problem(in);
    earley::io::process_and_write(G, words, out);

    EXPECT_EQ(out.str(), expected);
}

TEST(EarleyIOTests, WordWithUnknownTerminal) {
    const std::string input =
        "1 1 1\n"
        "S\n"
        "a\n"
        "S->a\n"
        "S\n"
        "1\n"
        "b\n";
    const std::string expected = "No\n";

    std::istringstream in(input);
    std::ostringstream out;

    auto [G, words] = earley::io::read_problem(in);
    earley::io::process_and_write(G, words, out);

    EXPECT_EQ(out.str(), expected);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}