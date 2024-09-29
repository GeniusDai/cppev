CollectedFileInfo = provider(
    doc = "Collect executables and dynamic libraries.",
    fields = {
        "files": "List[File]; The collected files.",
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
    "py_library",
    "py_binary",
]

def _collect_files_aspect_impl(target, ctx):
    print("Aspect for file collection in {}({})".format(ctx.rule.kind, target.label))

    direct = []
    if ctx.rule.kind in PACKAGE_RULES:
        for file in target[DefaultInfo].files.to_list():
            direct.append(file)

    transitive = []
    for attr in ATTR_ASPECTS:
        if not hasattr(ctx.rule.attr, attr):
            continue
        for dep in getattr(ctx.rule.attr, attr):
            if CollectedFileInfo in dep:
                transitive += dep[CollectedFileInfo].files

    return CollectedFileInfo(files = direct + transitive)

collect_files_aspect = aspect(
    implementation = _collect_files_aspect_impl,
    attr_aspects = ATTR_ASPECTS,
)

def _package_files_impl(ctx):
    inputs = []
    for file in ctx.attr.files:
        inputs += file[CollectedFileInfo].files
    inputs = depset(inputs).to_list()

    outputs = [ctx.actions.declare_file("{}.tar.gz".format(ctx.label.name))]

    command = "/usr/bin/tar -h -zcvf {} {}".format(outputs[0].path, " ".join([f.path for f in inputs]))

    print("Package command: {}".format(command))

    ctx.actions.run_shell(
        mnemonic = "PackageFiles",
        command = command,
        inputs = inputs,
        outputs = outputs,
    )

    return DefaultInfo(files = depset(outputs))

package_files = rule(
    implementation = _package_files_impl,
    attrs = {
        "files": attr.label_list(
            doc = "Sources and binaries to package.",
            aspects = [
                collect_files_aspect,
            ],
        ),
    },
)
