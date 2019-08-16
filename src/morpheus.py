import os
import sys

import click
from plumbum import local

@click.command()
@click.argument("source-file")
@click.option("-np", "--nproc", default=1, help="Number of processes.")
@click.option("-o", "--output-file", default=None, type=str, help="Output file")
def generate_mpn(source_file, nproc, output_file):
    assert ("LLVM_ROOT_PATH" in os.environ), "LLVM_ROOT_PATH is not specified."

    cwd = os.path.abspath(os.getcwd())

    llvm_root_path = os.path.abspath(os.environ["LLVM_ROOT_PATH"])
    llvm_bin = os.path.join(llvm_root_path, "build/bin")

    clang_compiler = "{}/clang++".format(llvm_bin)
    assert os.path.isfile(clang_compiler), "Wrong path to clang compiler."

    ll = local[clang_compiler][
        "-S",
        "-emit-llvm",                    # TODO: do I have to emit ll code? binary should be as good as ll
        "-g",
        "-O0",
        "-I", "/usr/include/mpi",        # include MPI
        "-Xclang","-disable-O0-optnone", # allow to run optimization passes later
        "-o", "-",                       # redirect output to stdout
        source_file
    ]

    lib_morph = "libMorph.so"
    # assert any(path in )

    assert ("LD_LIBRARY_PATH" in os.environ), "LD_LIBRARY_PATH is not set."
    assert any(
        os.path.isfile(os.path.join(path, lib_morph))
        for path in os.environ["LD_LIBRARY_PATH"].split(os.pathsep)
    ), "Could not find libMorph.so. Check is LD_LIBRARY_PATH is set properly."

    opt_tool = "{}/opt".format(llvm_bin)

    # TODO: set LD_LIBRARY_PATH for libMorph.so .. assert this
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
            ]

            cmd = ll | a | b | c
            print (cmd)
            cmd()


if __name__ == "__main__":
    generate_mpn()
