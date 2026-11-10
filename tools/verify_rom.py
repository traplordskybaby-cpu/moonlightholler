#!/usr/bin/env python3
"""Sanity-checks the finished ROM: header fields, checksums, and that the data banks are intact."""
import json
import sys
from pathlib import Path

LOGO = bytes.fromhex("CEED6666CC0D000B03730083000C000D0008111F8889000EDCCC6EE6DDDDD999BBBB67636E0EECCCDDDC999FBBB9333E")


def main():
    path = Path(sys.argv[1] if len(sys.argv) > 1 else "build/moonlight-holler.gb")
    rom = path.read_bytes()
    info = json.loads((path.parent / "rom_info.json").read_text())
    data = (path.parent / "databank.bin").read_bytes()
    problems = []
    if rom[0x104:0x134] != LOGO:
        problems.append("Nintendo logo is missing or damaged")
    title = rom[0x134:0x143].rstrip(b"\x00").decode("ascii", "replace")
    if title != "MOONLIGHTHOLLER":
        problems.append(f"title is {title!r}")
    if rom[0x143] != 0x80:
        problems.append("not flagged as Game Boy Color compatible")
    if rom[0x147] != 0x19:
        problems.append(f"cartridge type is {rom[0x147]:#x}, expected MBC5 (0x19)")
    expected = info["rom_banks"] * 0x4000
    if len(rom) != expected:
        problems.append(f"size is {len(rom)}, expected {expected}")
    x = 0
    for b in rom[0x134:0x14D]:
        x = (x - b - 1) & 0xFF
    if x != rom[0x14D]:
        problems.append("header checksum is wrong")
    total = (sum(rom) - rom[0x14E] - rom[0x14F]) & 0xFFFF
    if total != (rom[0x14E] << 8 | rom[0x14F]):
        problems.append("global checksum is wrong")
    if rom[0x8000:0x8000 + len(data)] != data:
        problems.append("data banks do not match build/databank.bin")
    if problems:
        print("ROM CHECK FAILED:")
        for p in problems:
            print("  -", p)
        sys.exit(1)
    print(f"ROM check passed: {path.name}, {len(rom) // 1024} KB, MBC5, CGB, title {title}")


if __name__ == "__main__":
    main()
