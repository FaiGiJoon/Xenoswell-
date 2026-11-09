"""Converts a raw 15-bit PCSX-Redux screenshot to PNG: raw2png.py in.raw W H out.png"""
import struct
import sys
import zlib


def chunk(tag, data):
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data))


path, w, h, out = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
raw = open(path, "rb").read()
rows = bytearray()
for y in range(h):
    rows.append(0)
    for x in range(w):
        v = raw[(y * w + x) * 2] | raw[(y * w + x) * 2 + 1] << 8
        rows += bytes(((v & 31) << 3, (v >> 5 & 31) << 3, (v >> 10 & 31) << 3))
png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
open(out, "wb").write(png + chunk(b"IDAT", zlib.compress(bytes(rows))) + chunk(b"IEND", b""))
