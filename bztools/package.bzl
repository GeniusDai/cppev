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

def _collect_files_aspect_impl(target, ctx):
    print("Aspect for file collection in {}({})".format(ctx.rule.kind, target.label))

    direct = []

    if ctx.attr.dev and ctx.rule.kind == "cc_library":
        direct += ctx.rule.files.hdrs

    transitive = []

    rules_package = [
        "cc_binary",
        "cc_shared_library",
        "py_binary",
        "py_library",
    ]

    if ctx.attr.dev:
        rules_package.append("cc_library")

    if ctx.rule.kind in rules_package:
        if DefaultInfo not in target:
            fail("{} doesn't have DefaultInfo!".format(target.label))
        if ctx.rule.kind != "cc_library":
            transitive.append(target[DefaultInfo].files)
        else:
            for file in target[DefaultInfo].files.to_list():
                if file.extension in ["so", "dylib"] and file.basename[:3] == "lib":
                    continue
                direct.append(file)

    for attr in ATTR_ASPECTS:
        if not hasattr(ctx.rule.attr, attr):
            continue
        for dep in getattr(ctx.rule.attr, attr):
            if CollectedFileInfo in dep:
                transitive.append(dep[CollectedFileInfo].files)

    return CollectedFileInfo(files = depset(direct = direct, transitive = transitive))

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
    transitive = []
    for file in ctx.attr.files:
        if CollectedFileInfo not in file:
            fail("{} doesn't have CollectedFileInfo!".format(file.label))
        transitive.append(file[CollectedFileInfo].files)

    inputs = sorted(depset(transitive = transitive).to_list())

    outputs = [ctx.actions.declare_file("{}.tar.gz".format(ctx.label.name))]

    command = "/usr/bin/tar -h -zcvf {} {}".format(outputs[0].path, " ".join([file.path for file in inputs]))

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
