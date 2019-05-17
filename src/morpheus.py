import os
import subprocess

import click

# TODO: pyenv path to correct version of LLVM

def compile_cpp():
    pass

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
    input_file = "testing_file.cpp" # TODO: cli
    workdir = os.getcwd()
    workdir_path = os.path.abspath(workdir)

    cmd_args = ["/bin/bash"]
    # process = subprocess.Popen(cmd_args, cwd=workdir_path,
    #                            stdin=subprocess.PIPE,
    #                            stdout=subprocess.PIPE,
    #                            stderr=subprocess.PIPE)

    # cmd = command(["ls"])
    # TODO: I should use Popen.communicate in a case I need pipes
    ret_cod = subprocess.call(["ls"], stdin=subprocess.PIPE,
                                      stdout=subprocess.PIPE,
                                      stderr=subprocess.PIPE)
    print (ret_cod)
    """
    # out, err = process.communicate(cmd.encode())
    pid = out.strip()

    if not pid:
        print("STDERR: {}".format(err))

    process.terminate()
    files = out.decode().split("\n")
    for file in files:
        cmd_args = ["/bin/bash"]
        process = subprocess.Popen(cmd_args, cwd=workdir_path,
                                stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE)
        print ("-- OUT FOR FILE: {} -----".format(file))
        cmd = command(["cat", file])
        out, err = process.communicate(cmd.encode())
        pid = out.strip()
        if not pid:
            print ("STDERR: {}".format(err))
        print (out.decode())
        print ("---------")

    # print (out.decode()) # TODO: that's the result I want to pass to a next command

    # return (int(pid), command)
    """


