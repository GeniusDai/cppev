cc_library(
    name = "cppev_interface",
    hdrs = glob(
        include = ["include/cppev/*.h"],
    ),
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
)

cc_library(
    name = "cppev_lib",
    srcs = glob(
        include = ["lib/*.cc"],
    ),
    deps = [
        "cppev_interface",
    ],
)

cc_shared_library(
    name = "cppev_dylib",
    deps = [
        "cppev_lib",
    ],
)

cc_library(
    name = "cppev",
    srcs = [
        "cppev_dylib",
    ],
    deps = [
        "cppev_interface",
    ],
    visibility = [
        "//visibility:public",
    ],
)
