cc_binary(
    name = "file_transfer_client",
    srcs = [
        "config.h",
        "file_transfer_client.cc",
    ],
    deps = [
        "//src:cppev",
    ],
    dynamic_deps = [
        "//src:cppev_dynamic",
    ],
    linkopts = [
        "-Wl,-rpath=$ORIGIN/../../src",
    ],
    visibility = [
        "//:__pkg__",
    ],
)

cc_binary(
    name = "file_transfer_server",
    srcs = [
        "config.h",
        "file_transfer_server.cc",
    ],
    deps = [
        "//src:cppev",
    ],
    dynamic_deps = [
        "//src:cppev_dynamic",
    ],
    linkopts = [
        "-Wl,-rpath=$ORIGIN/../../src",
    ],
    visibility = [
        "//:__pkg__",
    ],
)
