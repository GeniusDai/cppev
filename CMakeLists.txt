cmake_minimum_required(VERSION 3.0.0)

project("libcppev")

if (CMAKE_HOST_APPLE)
    message("Platform : macOS")
    set(CMAKE_CXX_FLAGS "-O0 -Wall -g -std=c++17 -pthread")
elseif(CMAKE_HOST_UNIX)
    message("Platform : Linux")
    set(CMAKE_CXX_FLAGS "-O0 -Wall -g -std=c++17 -pthread -lrt")
endif()

include_directories("src/include")

add_subdirectory("src")
add_subdirectory("unittest")
add_subdirectory("examples")

install(
    TARGETS "cppev"
    LIBRARY DESTINATION lib
)
install(
    DIRECTORY "src/include/cppev" DESTINATION include
)
