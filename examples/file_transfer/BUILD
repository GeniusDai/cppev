cc_binary(
    name = "file_transfer_client",
    srcs = [
        "config.h",
        "file_transfer_client.cc",
    ],
    dynamic_deps = [
        "//src:cppev_dynamic",
    ],
    linkopts = select({
        "@platforms//os:linux": [
            "-Wl,-rpath,$ORIGIN/../../src",
        ],
        "@platforms//os:osx": [],
    }),
    visibility = [
        "//examples:__pkg__",
    ],
    deps = [
        "//src:cppev",
    ],
)

cc_binary(
    name = "file_transfer_server",
    srcs = [
        "config.h",
        "file_transfer_server.cc",
    ],
    dynamic_deps = [
        "//src:cppev_dynamic",
    ],
    linkopts = select({
        "@platforms//os:linux": [
            "-Wl,-rpath,$ORIGIN/../../src",
        ],
        "@platforms//os:osx": [],
    }),
    visibility = [
        "//examples:__pkg__",
    ],
    deps = [
        "//src:cppev",
    ],
)
