load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def load_googletest():
    # googletest release-1.11.0
    http_archive(
        name = "googletest",
        urls = ["https://github.com/google/googletest/archive/e2239ee6043f73722e7aa812a459f54a28552929.zip"],
        strip_prefix = "googletest-e2239ee6043f73722e7aa812a459f54a28552929",
    )
