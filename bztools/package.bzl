CollectedFileInfo = provider(
    doc = "Collect executables and dynamic libraries.",
    fields = {
        "run_files": "depset[File]; The collected runtime files.",
        "dev_files": "depset[File]; The collected development files.",
    },
)

ATTR_ASPECTS = [
    "data",
    "srcs",
    "deps",
    "dynamic_deps",
]

def _collect_files_aspect_impl(target, ctx):
    print("Aspect for file collection in {}({})".format(ctx.rule.kind, target.label))

    run_files_direct = []
    run_files_transitive = []

    dev_files_direct = []
    dev_files_transitive = []

    if DefaultInfo not in target:
        fail("{} doesn't have DefaultInfo!".format(target.label))
    if ctx.rule.kind == "cc_binary":
        run_files_transitive.append(target[DefaultInfo].files)

        # The dynamic library in runfiles is introduced by two symbolic links pointing to
        # the same file in $output_path. It may cause a file duplication issue but it's OK.
        run_files_transitive.append(target[DefaultInfo].default_runfiles.files)
    elif ctx.rule.kind == "cc_library" and ctx.attr.dev:
        dev_files_direct += ctx.rule.files.hdrs
        dev_files_transitive.append(target[DefaultInfo].files)
    elif ctx.rule.kind == "cc_shared_library" and ctx.attr.dev:
        dev_files_transitive.append(target[DefaultInfo].files)

    for attr in ATTR_ASPECTS:
        if not hasattr(ctx.rule.attr, attr):
            continue
        for dep in getattr(ctx.rule.attr, attr):
            if CollectedFileInfo in dep:
                run_files_transitive.append(dep[CollectedFileInfo].run_files)
                dev_files_transitive.append(dep[CollectedFileInfo].dev_files)

    return CollectedFileInfo(
        run_files = depset(direct = run_files_direct, transitive = run_files_transitive),
        dev_files = depset(direct = dev_files_direct, transitive = dev_files_transitive),
    )

collect_files_aspect = aspect(
    doc = "Collect file info.",
    implementation = _collect_files_aspect_impl,
    attr_aspects = ATTR_ASPECTS,
    attrs = {
        "dev": attr.bool(
            doc = "Whether package development files.",
            mandatory = True,
        ),
    },
    provides = [
        CollectedFileInfo,
    ],
)

def _package_files_impl(ctx):
    run_files_transitive = []
    dev_files_transitive = []
    for file in ctx.attr.files:
        if CollectedFileInfo not in file:
            fail("{} doesn't have CollectedFileInfo!".format(file.label))
        run_files_transitive.append(file[CollectedFileInfo].run_files)
        dev_files_transitive.append(file[CollectedFileInfo].dev_files)

    files_hash = {
        "run": run_files_transitive,
        "dev": dev_files_transitive,
    }

    files_to_package = []

    for prefix in ["run", "dev"]:
        file_origins = sorted(depset(transitive = files_hash[prefix]).to_list())
        for file_origin in file_origins:
            file_origin_dir = file_origin.dirname.removeprefix(ctx.bin_dir.path + "/")
            file_target_path = "{}/{}/{}/{}".format(ctx.label.name, prefix, file_origin_dir, file_origin.basename)
            file_target = ctx.actions.declare_file(file_target_path)
            ctx.actions.run_shell(
                mnemonic = "CopyFile",
                command = "/bin/cp -p -L {} {}".format(file_origin.path, file_target.dirname),
                inputs = [file_origin],
                outputs = [file_target],
            )
            files_to_package.append(file_target)

    tarball = ctx.actions.declare_file("{}.tar.gz".format(ctx.label.name))
    command = "/usr/bin/tar -h -zcvf {} {}".format(tarball.path, " ".join([file.path for file in files_to_package]))
    ctx.actions.run_shell(
        mnemonic = "PackageFiles",
        command = command,
        inputs = files_to_package,
        outputs = [tarball],
    )

    return DefaultInfo(files = depset([tarball]))

package_files = rule(
    doc = "Package files using tar.",
    implementation = _package_files_impl,
    attrs = {
        "files": attr.label_list(
            doc = "Sources and binaries to package.",
            aspects = [
                collect_files_aspect,
            ],
            mandatory = True,
        ),
        "dev": attr.bool(
            doc = "For collect_files_aspect.",
            default = False,
            mandatory = False,
        ),
    },
    provides = [
        DefaultInfo,
    ],
)
