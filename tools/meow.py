import struct
import argparse


parser = argparse.ArgumentParser()
parser.add_argument("infile")

args = parser.parse_args()

with open(args.infile, "rb") as f:
    sticks = []
    while (data := f.read(8)):
        stick = struct.unpack("<ii", data)
        sticks.append(stick)
    
    print(sticks)
