load("//bztools:toolchain.bzl", "shell_command_toolchain")

toolchain_type(name = "toolchain_type")

shell_command_toolchain(
    name = "linux_command",
    cp = "/bin/cp",
    tar = "/bin/tar",
)

shell_command_toolchain(
    name = "osx_command",
    cp = "/bin/cp",
    tar = "/usr/bin/tar",
)

toolchain(
    name = "linux_command_toolchain",
    exec_compatible_with = [
        "@platforms//os:linux",
    ],
    toolchain = "linux_command",
    toolchain_type = "toolchain_type",
)

toolchain(
    name = "osx_command_toolchain",
    exec_compatible_with = [
        "@platforms//os:osx",
    ],
    toolchain = "osx_command",
    toolchain_type = "toolchain_type",
)
