cc_library(
    name = "cppev",
    hdrs = glob(
        include = ["include/cppev/*.h"],
    ),
    srcs = glob(
        include = ["lib/*.cc"],
    ),
    linkopts = [
        "-pthread",
        "-ldl",
    ] +
    select({
        "@platforms//os:linux" : [
            "-lrt"
        ],
        "@platforms//os:osx" : [],
    }),
    strip_include_prefix = "include/",
    visibility = [
        "//visibility:public",
    ]
)
