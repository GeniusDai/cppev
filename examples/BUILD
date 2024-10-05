load("//bztools:package.bzl", "package_files")

# Attention:
# Cannot package staticly linked cc_test, action conflict reported for the object file.
package_files(
    name = "cppev_examples",
    testonly = True,
    dev = True,
    excludes = [
        "//examples/nio_evlp:config.h",
    ],
    files = [
        "//examples/file_transfer:file_transfer_client",
        "//examples/file_transfer:file_transfer_server",
        "//examples/nio_evlp:tcp_client",
        "//examples/nio_evlp:tcp_server",
        "//examples/nio_evlp:udp_client",
        "//examples/nio_evlp:udp_server",
        "//examples/tcp_stress:simple_client",
        "//examples/tcp_stress:simple_server",
    ],
)
