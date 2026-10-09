"""Reads Data/Local/ServerList.bmd and, with --rename OLD NEW, renames a server group in place."""
import struct
import sys

KEY = (0xFC, 0xCF, 0xAB)
FMT = "<H32sBB15sh"  # index, name, pos, sequence, non-pvp flags, description length
SIZE = struct.calcsize(FMT)


def bux(data: bytes) -> bytes:
    return bytes(b ^ KEY[i % 3] for i, b in enumerate(data))


def read(path):
    data = open(path, "rb").read()
    records, pos = [], 0
    while pos + SIZE <= len(data):
        fields = list(struct.unpack(FMT, bux(data[pos:pos + SIZE])))
        pos += SIZE
        desc = bux(data[pos:pos + fields[5]])
        pos += fields[5]
        records.append((fields, desc))
    assert pos == len(data), f"trailing bytes: {len(data) - pos}"
    return records


def write(path, records):
    out = bytearray()
    for fields, desc in records:
        out += bux(struct.pack(FMT, *fields)) + bux(desc)
    open(path, "wb").write(out)


path = sys.argv[1]
records = read(path)
if len(sys.argv) == 5 and sys.argv[2] == "--rename":
    old, new = sys.argv[3].encode(), sys.argv[4].encode()
    assert len(new) < 32
    for fields, _ in records:
        if fields[1].rstrip(b"\0") == old:
            fields[1] = new.ljust(32, b"\0")
    write(path, records)
    records = read(path)
for fields, desc in records:
    print(fields[0], fields[1].rstrip(b"\0").decode(), "pos", fields[2], "seq", fields[3], "nonpvp", fields[4].hex(), "desc", desc)
