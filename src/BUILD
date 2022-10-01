cc_library(
    name = "cppev",
    hdrs = [
        "include/cppev/cppev.h",
        "include/cppev/sysconfig.h",
        "include/cppev/common_utils.h",
        "include/cppev/buffer.h",
        "include/cppev/async_logger.h",
        "include/cppev/timer.h",
        "include/cppev/lru_cache.h",
        "include/cppev/lock.h",
        "include/cppev/runnable.h",
        "include/cppev/thread_pool.h",
        "include/cppev/ipc.h",
        "include/cppev/subprocess.h",
        "include/cppev/event_loop.h",
        "include/cppev/nio.h",
        "include/cppev/tcp.h",
    ],
    srcs = [
        "lib/async_logger_buffered.cc",
        "lib/async_logger_common.cc",
        "lib/async_logger_hashed.cc",
        "lib/event_loop_kqueue.cc",
        "lib/event_loop_epoll.cc",
        "lib/common_utils.cc",
        "lib/sysconfig.cc",
        "lib/nio.cc",
        "lib/ipc.cc",
        "lib/subprocess.cc",
        "lib/tcp.cc",
    ],
    linkopts = select({
        "@platforms//os:linux" : [
            "-pthread",
            "-lrt"
        ],
        "@platforms//os:osx" : [
            "-pthread"
        ],
    }),
    strip_include_prefix = "include/",
    visibility = [
        "//visibility:public",
    ]
)
