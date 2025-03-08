cc_library(
    name = "cppev",
    srcs = glob([
        "lib/*.cc",
    ]),
    hdrs = glob([
        "include/cppev/*.h",
    ]),
    copts = [
        "-fvisibility=hidden",
    ],
    includes = [
        "include/",
    ],
    linkopts = [
        "-lpthread",
    ] + select({
        "@platforms//os:linux": [
            "-lrt",
            "-ldl",
        ],
        "@platforms//os:osx": [],
    }),
    visibility = [
        "//visibility:public",
    ],
)

cc_shared_library(
    name = "cppev_dynamic",
    user_link_flags = select({
        "@platforms//os:linux": [
            "-Wl,--no-undefined",
        ],
        # osx -undefined is deprecated
        "@platforms//os:osx": [],
    }),
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "cppev",
    ],
)
