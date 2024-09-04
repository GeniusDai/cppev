set (LIB
    lib/nio.cc
    lib/utils.cc
    lib/sysconfig.cc
    lib/event_loop_epoll.cc
    lib/event_loop_kqueue.cc
    lib/tcp.cc
    lib/subprocess.cc
    lib/ipc.cc
    lib/lock.cc
    lib/async_logger.cc
    lib/runnable.cc
)

add_library(cppev SHARED ${LIB})

target_include_directories(cppev PUBLIC include)
target_compile_options(cppev PUBLIC -O2 -Wall -Wextra)
target_link_libraries(cppev dl)
target_link_libraries(cppev pthread)

if (CMAKE_HOST_APPLE)
    message(STATUS "Platform : Apple")
else()
    message(STATUS "Platform : Linux")
    target_link_libraries(cppev rt)
endif()
