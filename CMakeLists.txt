cmake_minimum_required(VERSION 3.0.0)

project(libcppev)

if (CMAKE_HOST_UNIX)
    message("Platform : Unix")
endif()

set(CMAKE_CXX_FLAGS "-std=c++17 -O0 -Wall -g -pthread")
include_directories(src/include)

add_subdirectory(src)
add_subdirectory(unittest)
add_subdirectory(examples)

install(
    TARGETS cppev
    LIBRARY DESTINATION lib
)
install(
    DIRECTORY src/include/cppev DESTINATION include
)
