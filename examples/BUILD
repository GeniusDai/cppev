load("//bztools:package.bzl", "package_files")

# Attention:
# Cannot package staticly linked cc_test, action conflict reported for the object file.
package_files(
    name = "cppev_examples",
    testonly = True,
    dev = True,
    files = [
        "//examples/file_transfer:file_transfer_client",
        "//examples/file_transfer:file_transfer_server",
        "//examples/tcp_stress:simple_client",
        "//examples/tcp_stress:simple_server",
    ],
)
