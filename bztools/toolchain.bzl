ShellCommandInfo = provider(
    doc = "Information about shell commands location in different platforms.",
    fields = [
        "cp",
        "tar",
    ],
)

def _shell_command_toolchain_impl(ctx):
    toolchain_info = platform_common.ToolchainInfo(
        shell_command_info = ShellCommandInfo(
            cp = ctx.attr.cp,
            tar = ctx.attr.tar,
        ),
    )
    return [toolchain_info]

shell_command_toolchain = rule(
    implementation = _shell_command_toolchain_impl,
    attrs = {
        "cp": attr.string(),
        "tar": attr.string(),
    },
)
