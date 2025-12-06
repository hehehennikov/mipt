cmake_minimum_required(VERSION 4.0)
project(prac)

set(CMAKE_CXX_STANDARD 23)

# fetch content
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main
)
FetchContent_MakeAvailable(googletest)
enable_testing()

# Earley

## target lib
file(GLOB_RECURSE EARLEY_HEADERS lib/earley/*.hpp)

add_library(earley STATIC
        ${EARLEY_HEADERS}
        lib/earley/hello.cpp)

message(${EARLEY_HEADERS})

target_include_directories(earley PUBLIC
        lib
)

## tests
add_executable(earley_test tests/earley/unit.cpp)
target_link_libraries(earley_test earley GTest::gtest_main)

add_executable(prac main.cpp)
target_link_libraries(prac PUBLIC earley)