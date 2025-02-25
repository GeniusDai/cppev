cc_binary(
    name = "test_logger",
    srcs = [
        "test_logger.cc",
    ],
    linkopts = select({
        "@platforms//os:linux": [
            "-lstdc++fs",
        ],
        "@platforms//os:osx": [],
    }),
    deps = [
        "//src:cppev",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "test_buffer",
    srcs = [
        "test_buffer.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "test_utils",
    srcs = [
        "test_utils.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "test_ipc",
    srcs = [
        "config.h",
        "test_ipc.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "test_scheduler",
    srcs = [
        "test_scheduler.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "test_nio",
    srcs = [
        "test_nio.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "test_evlp",
    srcs = [
        "test_evlp.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "test_lock",
    srcs = [
        "config.h",
        "test_lock.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "test_subprocess",
    srcs = [
        "test_subprocess.cc",
    ],
    data = [
        "hello.py",
    ],
    linkopts = select({
        "@platforms//os:linux": [
            "-lstdc++fs",
        ],
        "@platforms//os:osx": [],
    }),
    linkstatic = True,
    visibility = [
        "//examples:__pkg__",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "test_thread_pool",
    srcs = [
        "test_thread_pool.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "test_runnable",
    srcs = [
        "test_runnable.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest",
    ],
)

cc_binary(
    name = "libDynamicLoaderTestImplFunctions.so",
    srcs = [
        "DynamicLoaderTestImpl.cc",
        "DynamicLoaderTestImpl.h",
        "DynamicLoaderTestImplFunctions.cc",
        "DynamicLoaderTestInterface.h",
    ],
    linkshared = True,
    target_compatible_with = [
        "@platforms//os:linux",
    ],
)

cc_binary(
    name = "libDynamicLoaderTestImplFunctions.dylib",
    srcs = [
        "DynamicLoaderTestImpl.cc",
        "DynamicLoaderTestImpl.h",
        "DynamicLoaderTestImplFunctions.cc",
        "DynamicLoaderTestInterface.h",
    ],
    linkshared = True,
    target_compatible_with = [
        "@platforms//os:osx",
    ],
)

# Attention! In ubuntu-20.04.
# The function dlerror and dlsym is undefined in the test_dynamic_loader.pic.o
# and cc_shared_library won't transfer linkopts. So cannot use //src:cppev_dynamic.
cc_test(
    name = "test_dynamic_loader",
    srcs = [
        "DynamicLoaderTestInterface.h",
        "test_dynamic_loader.cc",
    ],
    data = select({
        "@platforms//os:linux": [
            ":libDynamicLoaderTestImplFunctions.so",
        ],
        "@platforms//os:osx": [
            ":libDynamicLoaderTestImplFunctions.dylib",
        ],
    }),
    linkopts = select({
        "@platforms//os:linux": [
            "-lstdc++fs",
        ],
        "@platforms//os:osx": [],
    }),
    linkstatic = True,
    visibility = [
        "//examples:__pkg__",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest",
    ],
)
