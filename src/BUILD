cc_library(
    name = "cppev",
    hdrs = glob(
        include = ["include/cppev/*.h"],
    ),
    srcs = glob(
        include = ["lib/*.cc"],
    ),
    linkopts = select({
        "@platforms//os:linux" : [
            "-pthread",
            "-lrt"
        ],
        "@platforms//os:osx" : [
            "-pthread"
        ],
    }),
    strip_include_prefix = "include/",
    visibility = [
        "//visibility:public",
    ]
)
