set (LIB
    nio.cc
    event_loop_epoll.cc
    event_loop_kqueue.cc
    common_utils.cc
    async_logger.cc
    sysconfig.cc
    tcp.cc
)

add_library("cppev" SHARED ${LIB})
