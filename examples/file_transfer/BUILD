cc_library(
    name = "lib",
    hdrs = [
        "config.h"
    ],
)

cc_binary(
    name = "file_transfer_client",
    srcs = [
        "file_transfer_client.cc",
    ],
    deps = [
        "//src:cppev",
        "//examples/file_transfer:lib",
    ],
)

cc_binary(
    name = "file_transfer_server",
    srcs = [
        "file_transfer_server.cc",
    ],
    deps = [
        "//src:cppev",
        "//examples/file_transfer:lib",
    ],
)
