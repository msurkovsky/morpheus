import os
import io

import click
from plumbum import local

@click.command()
@click.argument("source-file")
@click.option("-np", "--nproc", default=1, help="Number of processes.")
@click.option("-o", "--output-dir", default=None, type=str, help="Output directory")
@click.option("-I", "--includes", default=None, type=str, multiple=True, help="Add directory to include search path")
def generate_mpn(source_file, nproc, output_dir, includes):
    cwd = os.path.abspath(os.getcwd())

    clang_compiler = "clang++"

    # prepare all includes
    incls = ["/usr/include/mpi"] + list([] if includes is None else includes[:])

    flatten = lambda l: [item for sublist in l for item in sublist]
    ll = local[clang_compiler][
        "-S",
        "-emit-llvm",                    # TODO: do I have to emit ll code? binary should be as good as ll
        "-g",
        "-O0",
        flatten(("-I", incl) for incl in incls),
        "-Xclang","-disable-O0-optnone",     # allow to run optimization passes later
        "-o", "-",                           # redirect output to stdout
        source_file
    ]

    lib_morph = "libMorph.so"
    # assert any(path in )

    assert ("LD_LIBRARY_PATH" in os.environ), "LD_LIBRARY_PATH is not set."
    assert any(
        os.path.isfile(os.path.join(path, lib_morph))
        for path in os.environ["LD_LIBRARY_PATH"].split(os.pathsep)
    ), "Could not find libMorph.so. Check is LD_LIBRARY_PATH is set properly."

    opt_tool = "opt"

    processes = []
    for p in range(nproc):
        with local.cwd(cwd):
            a = local[opt_tool][
                "-S",
                "--load", lib_morph,              # use old PM in order to process cli arguments (cl::opt)
                "--load-pass-plugin", lib_morph,  # use new PM
                "-passes", "substituterank",      # pass pruneprocess
                "-rank", str(p),                  # prune rank 'p'
                "-o", "-"                         # redirect output to stdout
            ]
            b = local[opt_tool][
                "-S",
                "-mem2reg",
                "-constprop",
                "-simplifycfg",
                "-o", "-"
            ]
            c = local[opt_tool][
                "-disable-output",
                "--load-pass-plugin", lib_morph,
                "-passes", "generate-mpn",
                "-o", "-"
            ]

            cmd = ll | a | b | c
            output = cmd();
            buf = io.StringIO(output)
            fname = buf.readline()
            with open(os.path.join(output_dir, fname.rstrip()), "w") as file:
                file.write(buf.read());

if __name__ == "__main__":
    generate_mpn()
