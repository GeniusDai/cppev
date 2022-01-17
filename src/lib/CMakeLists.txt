set (LIB
    nio.cc
    event_loop.cc
    common_utils.cc
    async_logger.cc
    sysconfig.cc
)

add_library("cppev" SHARED ${LIB})
