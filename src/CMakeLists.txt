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
)

add_library(cppev SHARED ${LIB})
target_link_libraries(cppev dl)

if (CMAKE_HOST_APPLE)
    message("Platform : Apple")
else()
    message("Platform : Linux")
    target_link_libraries(cppev rt)
endif()
