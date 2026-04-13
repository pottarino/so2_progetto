cmake_minimum_required(VERSION 4.1)
project(SO2 C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

add_executable(SO2
        main.c
        src/text_manipulation/parsers.c
        src/utility.c
        src/processing/hashtable.c
        src/text_manipulation/variable_recognizer.c
        src/processing/stats.c
        cmake-build-debug/Testing/test.txt

)