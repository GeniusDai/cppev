cc_binary(
    name = "simple_client",
    srcs = [
        "config.h",
        "simple_client.cc",
    ],
    linkstatic = False,
    visibility = [
        "//examples:__pkg__",
    ],
    deps = [
        "//src:cppev",
    ],
)

cc_binary(
    name = "simple_server",
    srcs = [
        "config.h",
        "simple_server.cc",
    ],
    linkstatic = False,
    visibility = [
        "//examples:__pkg__",
    ],
    deps = [
        "//src:cppev",
    ],
)
