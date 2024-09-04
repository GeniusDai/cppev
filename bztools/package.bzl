CollectedFileInfo = provider(
    doc = "",
    fields = {
        "files": "List[File]; The collected files.",
    },
)

def _collect_files_aspect_impl(target, ctx):
    print("Collect file aspect in {}({})".format(ctx.rule.kind, target.label))

    # Collect the binaries except .a.
    # If you don't want to include the .so produced by cc_library, please use linkstatic=True.
    # If you want to include .so in srcs, please add it explicitly.
    direct = []
    for file in target[DefaultInfo].files.to_list():
        if file.extension != "a":
            direct.append(file)

    transitive = []
    if hasattr(ctx.rule.attr, "deps"):
        for dep in ctx.rule.attr.deps:
            transitive += dep[CollectedFileInfo].files
    if hasattr(ctx.rule.attr, "dynamic_deps"):
        for dep in ctx.rule.attr.dynamic_deps:
            transitive += dep[CollectedFileInfo].files
    
    all_without_dup = []

    dict = {}
    for file in direct+transitive:
        if dict.get(file.path) == None:
            all_without_dup.append(file)
            dict[file.path] = ""

    return CollectedFileInfo(files=all_without_dup)

collect_files_aspect = aspect(
    implementation = _collect_files_aspect_impl,
    attr_aspects = [
        "deps",
        "dynamic_deps",
    ],
)

def _package_files_impl(ctx):
    inputs = []
    for dep in ctx.attr.files:
        inputs += dep[CollectedFileInfo].files
    inputs = depset(inputs).to_list()

    outputs = [ ctx.actions.declare_file("{}.tar.gz".format(ctx.label.name)) ]

    command = "/usr/bin/tar -h -zcvf {} {}".format(outputs[0].path, " ".join([ f.path for f in inputs]))

    print("Package command: {}".format(command))

    ctx.actions.run_shell(
        mnemonic = "SystemPackage",
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
