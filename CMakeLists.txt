cmake_minimum_required(VERSION 3.0.0)

project(cppev)

if (CMAKE_HOST_UNIX)
    message(STATUS "Platform : Unix")
endif()

set(CMAKE_CXX_FLAGS "-std=c++17 -O2 -Wall")
set(CMAKE_POSITION_INDEPENDENT_CODE on)

include_directories(src/include)

add_subdirectory(src)
add_subdirectory(examples)

find_package(GTest)
if (GTest_FOUND)
    add_subdirectory(unittest)
endif()

install(
    TARGETS cppev
    LIBRARY DESTINATION lib
)
install(
    DIRECTORY src/include/cppev DESTINATION include
)
