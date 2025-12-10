cmake_minimum_required(VERSION 4.0)
project(prac)

set(CMAKE_CXX_STANDARD 23)

# main target

add_executable(prac main.cpp)

# fetch content
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main
)
FetchContent_MakeAvailable(googletest)
enable_testing()

# algo selection options
option(EARLEY OFF)
option(LRK OFF)

if (${EARLEY} AND ${LRK})
    message(FATAL_ERROR "You cannot build with both LR(K) and Earley")
endif()

if (${EARLEY})
    ## target lib
    file(GLOB_RECURSE EARLEY_HEADERS lib/earley/*.hpp)

    add_library(earley STATIC
            ${EARLEY_HEADERS}
            lib/earley/hello.cpp)

    target_include_directories(earley PUBLIC
            lib
    )

    ## tests
    add_executable(earley_test tests/earley/unit.cpp)
    target_link_libraries(earley_test earley GTest::gtest_main)

    target_compile_definitions(earley PUBLIC EARLEY)
    target_link_libraries(prac PUBLIC earley)
endif ()

if (${LRK})
    if (NOT ${K} AND NOT ${K} EQUAL 0)
        message(FATAL_ERROR "You must define K using LR(K)")
    endif ()

    ## target lib
    file(GLOB_RECURSE LRK_HEADERS lib/lrk/*.hpp)

    add_library(lrk STATIC
            ${LRK_HEADERS}
            lib/lrk/hello.cpp)

    target_include_directories(lrk PUBLIC
            lib
    )

    ## tests
    add_executable(lrk_test tests/lrk/unit.cpp)
    target_link_libraries(lrk_test lrk GTest::gtest_main)

    target_compile_definitions(lrk PUBLIC LRK K=${K})
    target_link_libraries(prac PUBLIC lrk)
endif ()