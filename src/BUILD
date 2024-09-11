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
    ] + select({
        "@platforms//os:linux" : [
            "-lrt",
            "-ldl",
        ],
        "@platforms//os:osx" : [],
    }),
    visibility = [
        "//visibility:public",
    ],
    linkstatic = True,
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
