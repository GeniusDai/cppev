CollectedFileInfo = provider(
    doc = "Collect executables and dynamic libraries.",
    fields = {
        "files": "depset[File]; The collected files.",
    },
)

ATTR_ASPECTS = [
    "data",
    "srcs",
    "deps",
    "dynamic_deps",
]

PACKAGE_RULES = [
    "cc_binary",
    "cc_shared_library",
    "py_binary",
    "py_library",
]

def _collect_files_aspect_impl(target, ctx):
    print("Aspect for file collection in {}({})".format(ctx.rule.kind, target.label))

    transitive = []
    if ctx.rule.kind in PACKAGE_RULES:
        if DefaultInfo not in target:
            fail("{} doesn't have DefaultInfo!".format(target.label))
        transitive.append(target[DefaultInfo].files)

    for attr in ATTR_ASPECTS:
        if not hasattr(ctx.rule.attr, attr):
            continue
        for dep in getattr(ctx.rule.attr, attr):
            if CollectedFileInfo in dep:
                transitive.append(dep[CollectedFileInfo].files)

    return CollectedFileInfo(files = depset(transitive = transitive))

collect_files_aspect = aspect(
    doc = "Collect file info.",
    implementation = _collect_files_aspect_impl,
    attr_aspects = ATTR_ASPECTS,
    provides = [
        CollectedFileInfo,
    ],
)

def _package_files_impl(ctx):
    transitive = []
    for file in ctx.attr.files:
        if CollectedFileInfo not in file:
            fail("{} doesn't have CollectedFileInfo!".format(file.label))
        transitive.append(file[CollectedFileInfo].files)

    inputs = depset(transitive = transitive).to_list()

    outputs = [ctx.actions.declare_file("{}.tar.gz".format(ctx.label.name))]

    command = "/usr/bin/tar -h -zcvf {} {}".format(outputs[0].path, " ".join([file.path for file in inputs]))

    print("Package command: {}".format(command))

    ctx.actions.run_shell(
        mnemonic = "PackageFiles",
        command = command,
        inputs = inputs,
        outputs = outputs,
    )

    return DefaultInfo(files = depset(outputs))

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
    },
    provides = [
        DefaultInfo,
    ],
)
