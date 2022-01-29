set (LIB
    nio.cc
    async_logger.cc
    common_utils.cc
    sysconfig.cc
    event_loop_epoll.cc
    event_loop_kqueue.cc
    tcp.cc
    lock.cc
)

add_library("cppev" SHARED ${LIB})
