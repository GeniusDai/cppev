config_setting(
    name = "cppev_debug",
    values = {
        "define": "cppev_debug=true",
    },
)

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
    local_defines = select({
        "//conditions:default": [],
        "cppev_debug": [
            "CPPEV_DEBUG",
        ],
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
