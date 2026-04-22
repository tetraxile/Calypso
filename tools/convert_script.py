#!/usr/bin/env python3

import argparse
import os
from pathlib import Path
import sys

from util import BinaryReader


STAS_COMMAND_NAMES = {
    0: "FRAME",
    1: "CONTROLLER",
    2: "MOTION",
    3: "AMIIBO",
    4: "TOUCH"
}

STAS_BUTTON_NAMES = [
    "A",
    "B",
    "X",
    "Y",
    "LSTICK",
    "RSTICK",
    "L",
    "R",
    "ZL",
    "ZR",
    "PLUS",
    "MINUS",
    "DLEFT",
    "DUP",
    "DRIGHT",
    "DDOWN"
]


def parse_stas(path: Path):
    with open(path, "rb") as f:
        reader = BinaryReader(f.read())

    reader.read_signature(4, "STAS")
    stas_version = reader.read_u16()
    game_version = reader.read_u16()
    editor_version = reader.read_u16()
    reader.align(4)
    title_id = reader.read_u64()

    if stas_version not in (0,):
        print(f"unsupported STAS version: {stas_version}", file=sys.stderr)
        sys.exit(1)


    print(f"header:")
    print(f"  STAS version: {stas_version:04x}")
    print(f"  game version: {game_version:04x}")
    print(f"  editor version: {editor_version:04x}")
    print(f"  title ID: {title_id:016x}")
    print()

    command_count = reader.read_u32()
    seconds_editing = reader.read_u32()
    player_count = reader.read_u8()
    reader.align(4)
    controller_types = reader.read_u8s(4)
    author_name_len = reader.read_u16()
    author_name = reader.read_string("utf-8", size=author_name_len)
    reader.align(4)

    print(f"script header:")
    print(f"  command count: {command_count}")
    print(f"  seconds spent editing: {seconds_editing}")
    print(f"  num players: {player_count}")
    print(f"  controller types: {controller_types}")
    print(f"  author: {author_name}")
    print()

    # return
    for i in range(min(command_count, 200)):
    # for i in range(command_count):
        command_start = reader.position

        command_type = reader.read_u16()
        command_size = reader.read_u16()

        # print(f"command {i}: {STAS_COMMAND_NAMES[command_type]} (offset: {command_start:#x})")

        if command_type == 0:
            assert(command_size == 0x4)
            frame_id = reader.read_u32()
            print(f"  frame: {frame_id}")
        elif command_type == 1:
            assert(command_size == 0x1c)
            player_id = reader.read_u8()
            reader.align(4)
            buttons = reader.read_u64()
            left_stick = reader.read_s32s(2)
            right_stick = reader.read_s32s(2)
            # print(f"  player: {player_id}")
            print(f"  buttons: ", end="")
            is_empty = True
            for i in range(len(STAS_BUTTON_NAMES)):
                if (1 << i) & buttons:
                    if is_empty:
                        is_empty = False
                    else:
                        print(", ", end="")
                    print(STAS_BUTTON_NAMES[i], end="")
            print()
            print(f"  left stick:  {left_stick}")
            # print(f"  right stick: {right_stick}")
        else:
            reader.read_bytes(command_size)

        # print()


def parse_lunakit(path: Path):
    with open(path, "rb") as f:
        reader = BinaryReader(f.read())
    
    reader.read_signature(4, "BOOB")
    print(reader.read_u32())
    print(reader.read_bool())
    reader.seek_rel(3)
    print(reader.read_s32())

    pass


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("infile")
    parser.add_argument("outfile", nargs="?")
    parser.add_argument("-f", "--format", choices=("stas", "lunakit", "tsv"))

    args = parser.parse_args()



    in_path = Path(args.infile)
    if not in_path.is_file():
        print("error: input file does not exist", file=sys.stderr)
        return 1
    
    _, in_ext = os.path.splitext(in_path)

    if args.format is not None:
        format_ = args.format
    else:
        format_ = in_ext

    if format_ == "stas":
        parse_stas(in_path)
    elif format_ == "lunakit":
        parse_lunakit(in_path)
    elif format_ == "tsv":
        print("error: TSV-TAS format not supported", file=sys.stderr)
        return 1
    else:
        print("error: unsupported/unknown script format", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())