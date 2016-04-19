#!/usr/bin/env python
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("patchfile", help="name of patch file in YDL format")
parser.add_argument("-n", "--numpatches", type=int, default=100,
                    help="number of patches to convert")
args = parser.parse_args()

with open(args.patchfile, 'rb') as f:
    patches = f.read(13 + 261 * min(args.numpatches, 100))

if patches:
    print("const uint8_t patches[] PROGMEM = {")
    for b in patches:
        print("0x%02x, " % ord(b))
    print("};")
