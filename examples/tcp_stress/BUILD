cc_library(
    name = "lib",
    hdrs = [
        "config.h"
    ],
)

cc_binary(
    name = "simple_client",
    srcs = [
        "simple_client.cc",
    ],
    deps = [
        "//src:cppev",
        "//examples/tcp_stress:lib",
    ]
)

cc_binary(
    name = "simple_server",
    srcs = [
        "simple_server.cc",
    ],
    deps = [
        "//src:cppev",
        "//examples/tcp_stress:lib",
    ]
)
