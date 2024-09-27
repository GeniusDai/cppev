cc_test(
    name = "test_async_logger",
    srcs = [
        "test_async_logger.cc",
    ],
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
    name = "test_nio_evlp",
    srcs = [
        "test_nio_evlp.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest",
    ],
)

cc_test(
    name = "test_lock",
    srcs = [
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
        "//unittest:hello.py",
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
    name = "libLoaderTestFunctions.so",
    srcs = [
        "LoaderTestBase.h",
        "LoaderTestFunctions.cc",
        "LoaderTestImpl.cc",
        "LoaderTestImpl.h",
    ],
    linkshared = True,
    target_compatible_with = [
        "@platforms//os:linux",
    ],
)

cc_binary(
    name = "libLoaderTestFunctions.dylib",
    srcs = [
        "LoaderTestBase.h",
        "LoaderTestFunctions.cc",
        "LoaderTestImpl.cc",
        "LoaderTestImpl.h",
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
        "LoaderTestBase.h",
        "test_dynamic_loader.cc",
    ],
    data = select({
        "@platforms//os:linux": [
            ":libLoaderTestFunctions.so",
        ],
        "@platforms//os:osx": [
            ":libLoaderTestFunctions.dylib",
        ],
    }),
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
