cc_library(
    name = "cppev",
    srcs = glob([
        "lib/*.cc",
    ]),
    hdrs = glob([
        "include/cppev/*.h",
    ]),
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
    linkstatic = True,
    visibility = [
        "//visibility:public",
    ],
)

cc_shared_library(
    name = "cppev_dynamic",
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "cppev",
    ],
)
