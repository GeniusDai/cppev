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

    output_files = []

    inputs = sorted(depset(transitive = run_files_transitive).to_list())
    outputs = [ctx.actions.declare_file("{}_run.tar.gz".format(ctx.label.name))]
    command = "/usr/bin/tar -h -zcvf {} {}".format(outputs[0].path, " ".join([file.path for file in inputs]))
    ctx.actions.run_shell(
        mnemonic = "PackageRunFiles",
        command = command,
        inputs = inputs,
        outputs = outputs,
    )
    output_files += outputs

    if ctx.attr.dev:
        inputs = sorted(depset(transitive = dev_files_transitive).to_list())
        outputs = [ctx.actions.declare_file("{}_dev.tar.gz".format(ctx.label.name))]
        command = "/usr/bin/tar -h -zcvf {} {}".format(outputs[0].path, " ".join([file.path for file in inputs]))
        ctx.actions.run_shell(
            mnemonic = "PackageDevFiles",
            command = command,
            inputs = inputs,
            outputs = outputs,
        )
        output_files += outputs

    return DefaultInfo(files = depset(output_files))

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
