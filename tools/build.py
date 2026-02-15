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
STD_DIR          = CLIENT_DIR / "lib" / "std"


# ~~~~~ UTILS ~~~~~

def run_command(args: list[str]) -> int:
    print(" ".join(args))

    try:
        subprocess.run(args, check=True)
        return 0
    except subprocess.CalledProcessError:
        return 1


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
    shutil.copytree(src, dst, dirs_exist_ok=True)


def copy_file(src: Path, dst: Path):
    assert isinstance(src, Path)
    assert isinstance(dst, Path)

    print(f"cp {src} {dst}")
    shutil.copy(src, dst)


def create_dir(path: Path):
    assert isinstance(path, Path)

    if not path.is_dir:
        print(f"mkdir {path}")
        os.mkdir(path)
    
def change_dir(path: Path):
    assert isinstance(path, Path)

    print(f"cd {path}")
    os.chdir(CLIENT_DIR)



# ~~~~~ BUILD STEPS ~~~~~


def create_std_dir() -> int:
    change_dir(CLIENT_DIR)
    return run_command([ "sys/tools/setup_libcxx_prepackaged.py" ])


def build_client(job_count: int) -> int:
    if (r := run_command([ "cmake", "-B", str(CLIENT_BUILD_DIR), "-S", str(CLIENT_DIR) ])): return r
    if (r := run_command([ "cmake", "--build", str(CLIENT_BUILD_DIR), "--", "-j", str(job_count) ])): return r

    copy_dir(CLIENT_BUILD_DIR / "sd", OUTPUT_DIR / "sd")
    copy_file(CLIENT_BUILD_DIR / "Calypso.nss", OUTPUT_DIR / "Calypso.nss")
    return 0


def build_server(job_count: int) -> int:
    if (r := run_command([ "cmake", "-B", str(SERVER_BUILD_DIR), "-S", str(SERVER_DIR) ])): return r
    if (r := run_command([ "cmake", "--build", str(SERVER_BUILD_DIR), "--", "-j", str(job_count) ])): return r

    copy_file(SERVER_BUILD_DIR / "CalypsoServer", OUTPUT_DIR / "CalypsoServer")
    return 0


def main() -> int:
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
        create_dir(OUTPUT_DIR)
        if not STD_DIR.is_dir():
            if (r := create_std_dir()): return r

        if "client" in targets:
            if (r := build_client(args.jobs)): return r
        if "server" in targets:
            if (r := build_server(args.jobs)): return r
    
    return 0


if __name__ == "__main__":
    sys.exit(main())