cc_test(
    name = "test_async_logger",
    srcs = [
        "test_async_logger.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_buffer",
    srcs = [
        "test_buffer.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_utils",
    srcs = [
        "test_utils.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_ipc",
    srcs = [
        "test_ipc.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_scheduler",
    srcs = [
        "test_scheduler.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_nio_evlp",
    srcs = [
        "test_nio_evlp.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_lock",
    srcs = [
        "test_lock.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_subprocess",
    srcs = [
        "test_subprocess.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_thread_pool",
    srcs = [
        "test_thread_pool.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "test_runnable",
    srcs = [
        "test_runnable.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest_main",
    ],
)

cc_binary(
    name = "libLoaderTestFunctions.so",
    srcs = [
        "LoaderTestBase.h",
        "LoaderTestImpl.h",
        "LoaderTestImpl.cc",
        "LoaderTestFunctions.cc",
    ],
    copts = [
        "-fPIC",
    ],
    target_compatible_with = [
        "@platforms//os:linux",
    ],
    linkshared = True,
)

cc_binary(
    name = "libLoaderTestFunctions.dylib",
    srcs = [
        "LoaderTestBase.h",
        "LoaderTestImpl.h",
        "LoaderTestImpl.cc",
        "LoaderTestFunctions.cc",
    ],
    copts = [
        "-fPIC",
    ],
    target_compatible_with = [
        "@platforms//os:osx",
    ],
    linkshared = True,
)

cc_test(
    name = "test_dynamic_loader",
    srcs = [
        "LoaderTestBase.h",
        "test_dynamic_loader.cc",
    ],
    deps = [
        "//src:cppev",
        "@googletest//:gtest_main",
    ] +
    select({
        "@platforms//os:linux" : [
            ":libLoaderTestFunctions.so",
        ],
        "@platforms//os:osx" : [
            ":libLoaderTestFunctions.dylib",
        ],
    }),
)
