#!/usr/bin/python3

import os
import io
import pwd

import click
from plumbum import local

@click.command()
@click.argument("source-file")
@click.option("-np", "--nproc", default=1, help="Number of processes.")
@click.option("-o", "--output-dir", default=None, type=str, help="Output directory")
@click.option("-I", "--includes", default=None, type=str, multiple=True, help="Add directory to include search path")
def run(source_file, nproc, output_dir, includes):
    src_path = os.path.abspath(source_file)
    src_dir_path = os.path.abspath(os.path.dirname(source_file))
    src_file_basename = os.path.basename(source_file)
    if output_dir is None:
        output_dir = os.getcwd()
    dest_path = os.path.abspath(output_dir)

    # prepare includes
    incls = [] if includes is None else includes[:]
    n = iter(range(len(incls)))
    get_remote = lambda: "/remote{}".format(next(n))
    paths_to_remotes = [(os.path.abspath(path), "{}".format(get_remote()))
                        for path in incls]

    # put together all remotes
    remotes = [(src_dir_path, "/input"), (dest_path, "/output")] + paths_to_remotes

    flatten = lambda l: [item for sublist in l for item in sublist]

    user_id = pwd.getpwnam(os.environ["USER"]).pw_uid
    cmd = local["docker"][
        "run",
        flatten(("--user", user_id, "-v", remote)
                for remote in map(lambda x: "{}:{}".format(*x), remotes)),
        "morpheus",
        "-np", nproc,
        os.path.join("/input", src_file_basename),
        "-o", "/output",
        flatten(("-I", path_to_remote[1]) for path_to_remote in paths_to_remotes)
    ]

    cmd()

if __name__ == "__main__":
    run()
