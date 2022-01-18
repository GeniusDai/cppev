cmake_minimum_required(VERSION 3.0.0)

project("libcppev")

set(CMAKE_CXX_FLAGS "-O0 -Wall -g -std=c++11 -pthread")

include_directories("src/include")

add_subdirectory("src")
add_subdirectory("unittest")
add_subdirectory("examples")

install(
    TARGETS "cppev"
    LIBRARY DESTINATION lib
)
install(
    DIRECTORY "include/cppev" DESTINATION include
)