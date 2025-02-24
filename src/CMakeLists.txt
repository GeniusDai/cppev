set (LIB
    lib/logger.cc
    lib/nio.cc
    lib/utils.cc
    lib/sysconfig.cc
    lib/event_loop_epoll.cc
    lib/event_loop_kqueue.cc
    lib/event_loop_common.cc
    lib/tcp.cc
    lib/subprocess.cc
    lib/ipc.cc
    lib/lock.cc
    lib/runnable.cc
    lib/dynamic_loader.cc
    lib/thread_pool.cc
)

option(CPPEV_DEBUG "print debug log" OFF)

add_library(cppev SHARED ${LIB})

target_include_directories(cppev PUBLIC include)
target_compile_options(cppev PUBLIC -O3 -Wall -Wextra)
target_link_libraries(cppev dl pthread)
if (CPPEV_DEBUG)
    target_compile_definitions(cppev PRIVATE -DCPPEV_DEBUG)
endif()

if (CMAKE_HOST_APPLE)
    message(STATUS "Platform : Apple")
else()
    message(STATUS "Platform : Linux")
    target_link_libraries(cppev rt)
    target_link_options(cppev PRIVATE "-Wl,--no-undefined")
endif()
