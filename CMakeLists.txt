cmake_minimum_required(VERSION 3.12.0)

project(cppev)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE on)

add_subdirectory(src)
add_subdirectory(examples)

find_package(GTest)
if (GTest_FOUND)
    add_subdirectory(unittest)
endif()

install(
    TARGETS cppev LIBRARY DESTINATION lib
)
install(
    DIRECTORY src/include/cppev DESTINATION include
)
