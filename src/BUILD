cc_library(
    name = "cppev",
    hdrs = glob([
        "include/cppev/*.h",
    ]),
    srcs = glob([
        "lib/*.cc",
    ]),
    includes = [
        "include/",
    ],
    linkopts = [
        "-lpthread",
        "-ldl",
    ] + select({
        "@platforms//os:linux" : [
            "-lrt"
        ],
        "@platforms//os:osx" : [],
    }),
    visibility = [
        "//visibility:public",
    ],
)

cc_shared_library(
    name = "cppev_dynamic",
    deps = [
        "cppev",
    ],
    visibility = [
        "//visibility:public",
    ],
)
