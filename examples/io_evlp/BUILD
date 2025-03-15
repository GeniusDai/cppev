package(default_visibility = ["//examples:__pkg__"])

cc_library(
    name = "lib",
    hdrs = [
        "config.h",
    ],
)

cc_binary(
    name = "tcp_client",
    srcs = [
        "tcp_client.cc",
    ],
    dynamic_deps = [
        "//src:cppev_dynamic",
    ],
    deps = [
        ":lib",
        "//src:cppev",
    ],
)

cc_binary(
    name = "tcp_server",
    srcs = [
        "tcp_server.cc",
    ],
    dynamic_deps = [
        "//src:cppev_dynamic",
    ],
    deps = [
        ":lib",
        "//src:cppev",
    ],
)

cc_binary(
    name = "udp_client",
    srcs = [
        "udp_client.cc",
    ],
    deps = [
        ":lib",
        "//src:cppev",
    ],
)

cc_binary(
    name = "udp_server",
    srcs = [
        "udp_server.cc",
    ],
    deps = [
        ":lib",
        "//src:cppev",
    ],
)
