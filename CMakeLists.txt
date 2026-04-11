cmake_minimum_required(VERSION 4.1)
project(SO2 C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_executable(SO2 main.c
        src/text_manipulation/parsers.c
        src/utility.c
        include/utility.h
        src/text_manipulation/variable_recognizer.c
        src/testing/maintest.c
        cmake-build-debug/testcase1.c
        include/parsers.h
)


add_executable(esegui_test
        src/text_manipulation/parsers.c
        src/utility.c
        include/utility.h
        src/text_manipulation/variable_recognizer.c
        src/testing/maintest.c
        cmake-build-debug/testcase1.c
        include/parsers.h
        include/stats.h)