set (LIB
    nio.cc
    async_logger_common.cc
    async_logger_buffered.cc
    async_logger_hashed.cc
    common_utils.cc
    sysconfig.cc
    event_loop_epoll.cc
    event_loop_kqueue.cc
    tcp.cc
    subprocess.cc
    ipc.cc
    dynamic_loader.cc
)

add_library(cppev SHARED ${LIB})
target_link_libraries(cppev dl)

if (CMAKE_HOST_APPLE)
    message("Platform : Apple")
else()
    message("Platform : Linux")
    target_link_libraries(cppev rt)
endif()
