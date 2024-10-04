load("toolchain.bzl", "shell_command_toolchain")

toolchain_type(name = "toolchain_type")

shell_command_toolchain(
    name = "shell_command_linux",
    cp = "/bin/cp",
    tar = "/bin/tar",
)

shell_command_toolchain(
    name = "shell_command_osx",
    cp = "/bin/cp",
    tar = "/usr/bin/tar",
)

toolchain(
    name = "shell_command_linux_toolchain",
    exec_compatible_with = [
        "@platforms//os:linux",
    ],
    target_compatible_with = [
        "@platforms//os:linux",
    ],
    toolchain = "shell_command_linux",
    toolchain_type = "toolchain_type",
)

toolchain(
    name = "shell_command_osx_toolchain",
    exec_compatible_with = [
        "@platforms//os:osx",
    ],
    target_compatible_with = [
        "@platforms//os:osx",
    ],
    toolchain = "shell_command_osx",
    toolchain_type = "toolchain_type",
)
