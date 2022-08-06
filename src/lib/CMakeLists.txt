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
    linux_specific.cc
)

add_library("cppev" SHARED ${LIB})
