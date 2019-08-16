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
def run(source_file, nproc, output_dir):
    src_path = os.path.abspath(source_file)
    src_dir_path = os.path.dirname(source_file)
    src_file_basename = os.path.basename(source_file)
    if output_dir is None:
        output_dir = os.getcwd()
    dest_path = os.path.abspath(output_dir)

    user_id = pwd.getpwnam(os.environ["USER"]).pw_uid
    cmd = local["docker"][
        "run",
        "--user", user_id,
        "-v", "{}:{}".format(src_dir_path, "/input"),
        "--user", user_id,
        "-v", "{}:{}".format(dest_path, "/output"),
        "morpheus",
        "-np", nproc,
        os.path.join("/input", src_file_basename),
        "-o", "/output"
    ]

    cmd();

if __name__ == "__main__":
    run()
