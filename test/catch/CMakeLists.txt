cmake_minimum_required(VERSION 2.8)

project("estdlib-unit-tests")

set(CMAKE_CXX_STANDARD 11)
# FIX: Just brute forcing this for now to get util.embedded imported stuff tested
add_definitions(-D__POSIX__)

INCLUDE_DIRECTORIES(../../ext/Catch2/single_include)
set(ESTDLIB_DIR ../../src)

add_subdirectory(${ESTDLIB_DIR} estdlib)
include_directories(${ESTDLIB_DIR})

add_executable(${PROJECT_NAME} "main.cpp"
    array-test.cpp
    experimental-test.cpp
    list-test.cpp
    map-test.cpp
    memory-test.cpp
    queue-test.cpp
    string-test.cpp
    streambuf-test.cpp
    vector-test.cpp
    )

target_link_libraries(${PROJECT_NAME} estd_lib)
