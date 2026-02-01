#!/usr/bin/env python3

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT_DIR = Path(__file__).parent.parent
CLIENT_DIR       = ROOT_DIR   / "client"
CLIENT_BUILD_DIR = CLIENT_DIR / "build"
SERVER_DIR       = ROOT_DIR   / "server"
SERVER_BUILD_DIR = SERVER_DIR / "build"
OUTPUT_DIR       = ROOT_DIR   / "build"


# ~~~~~ UTILS ~~~~~

def run_command(args: list[str]):
    print(" ".join(args))
    subprocess.run(args)


def remove_dir(path: Path):
    assert isinstance(path, Path)

    if not path.exists():
        return

    if not path.is_dir():
        print(f"error: `{path}` is not a directory", file=sys.stderr)
        sys.exit(1)

    print(f"rm -rf {path}")
    shutil.rmtree(path)


def remove_file(path: Path):
    assert isinstance(path, Path)

    if not path.exists():
        return

    if not path.is_file():
        print(f"error: `{path}` is not a file", file=sys.stderr)
        sys.exit(1)

    print(f"rm {path}")
    os.remove(path)


def copy_dir(src: Path, dst: Path):
    assert isinstance(src, Path)
    assert isinstance(dst, Path)

    print(f"cp -r {src} {dst}")
    shutil.copytree(src, dst)


def copy_file(src: Path, dst: Path):
    assert isinstance(src, Path)
    assert isinstance(dst, Path)

    print(f"cp {src} {dst}")
    shutil.copy(src, dst)



# ~~~~~ BUILD STEPS ~~~~~

def create_output_dir():
    if not OUTPUT_DIR.is_dir:
        print(f"mkdir {OUTPUT_DIR}")
        os.mkdir(OUTPUT_DIR)


def build_client(job_count: int):
    run_command([ "cmake", "-B", str(CLIENT_BUILD_DIR), "-S", str(CLIENT_DIR) ])
    run_command([ "cmake", "--build", str(CLIENT_BUILD_DIR) ])

    copy_dir(CLIENT_BUILD_DIR / "sd", OUTPUT_DIR / "sd")


def build_server(job_count: int):
    run_command([ "cmake", "-B", str(SERVER_BUILD_DIR), "-S", str(SERVER_DIR) ])
    run_command([ "cmake", "--build", str(SERVER_BUILD_DIR) ])

    copy_file(SERVER_BUILD_DIR / "CalypsoServer", OUTPUT_DIR / "CalypsoServer")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("target", choices=("client", "server", "all"), default="all", nargs="?")
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count())
    parser.add_argument("-c", "--clean", action="store_true")

    args = parser.parse_args()

    targets = ["client", "server"] if args.target == "all" else [args.target]

    if args.clean:
        if args.target == "all":
            remove_dir(OUTPUT_DIR)

        if "client" in targets:
            remove_dir(CLIENT_BUILD_DIR)
            remove_dir(OUTPUT_DIR / "sd")
        if "server" in targets:
            remove_dir(SERVER_BUILD_DIR)
            remove_file(OUTPUT_DIR / "CalypsoServer")
    else:
        create_output_dir()

        if "client" in targets:
            build_client(args.jobs)
        if "server" in targets:
            build_server(args.jobs)


if __name__ == "__main__":
    main()