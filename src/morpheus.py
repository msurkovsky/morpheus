import os
from subprocess import Popen, PIPE, DEVNULL

import click

class Pipeline:
    def __init__(self):
        self.proc = None

    def command(self, args, **kwargs):
        stdin = PIPE if self.proc is None else self.proc.stdout
        proc = Popen(list(args), stdin=stdin, stdout=PIPE, stderr=PIPE, **kwargs)
        if self.proc is not None:
            self.proc.stdout.close() # close in order to allow receive a SIGPIPE
                                     # if the current one exits
        self.proc = proc
        return self

    def run(self, **kwargs):
        if self.proc is not None:
            (out, pid) = self.proc.communicate(**kwargs)
            return out

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

    getll = Pipeline()
    ll = getll.command([
        clang_compiler,
        "-S",
        "-emit-llvm",                    # TODO: do I have to emit ll code? binary should be as good as ll
        "-g",
        "-O0",
        "-I", "/usr/include/mpi",        # include MPI
        "-Xclang","-disable-O0-optnone", # allow to run optimization passes later
        "-o", "-",                       # redirect output to stdout
        source_file
    ], cwd=cwd).run()

    lib_morph = "libMorph.so"
    opt_tool = "{}/opt".format(llvm_bin)

    # TODO: set LD_LIBRARY_PATH for libMorph.so .. assert this
    processes = []
    for p in range(nproc):
        prune_nth_ll = Pipeline()
        pruned_ll = prune_nth_ll \
            .command([
                opt_tool,
                "-S",                             # TODO: debug to write as LLVM assembly
                "--load", lib_morph,              # use old PM in order to process cli arguments (cl::opt)
                "--load-pass-plugin", lib_morph,  # use new PM
                "-passes", "pruneprocess",        # pass pruneprocess
                "-rank", str(p),                  # prune rank 'p'
                "-o", "-"                         # redirect output to stdout
            ], cwd=cwd) \
            .command([ # TODO: why I cannot pipeline commands in this way?!
                opt_tool,
                "-S",
                "-mem2reg",
                "-constprop",
                "-simplifycfg",
                "-o", "-"
            ], cwd=cwd).run(input=ll)

        processes.append(pruned_ll)

    print ("#np: ", len(processes))
    for i, proc  in enumerate(processes):
        print ("- {} -----------------------".format(i))
        print (proc.decode())
        print ("---------------------------\n")


def command(commands, env=None): # TODO do I need sth like this?
    args = []

    environment = {};
    if env:
        environment.update(env)

    if environment:
        args += ["env"]
        for (key, val) in environment.items():
            args += ["{}={}".format(key, val)]

    args += [str(cmd) for cmd in commands]

    return " ".join(args)

# rather I need a concatenation of commands -> pipelining


if __name__ == "__main__":
    generate_mpn()

