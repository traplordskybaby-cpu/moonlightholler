#!/usr/bin/env python3
"""Appends the data banks (cover art, maps, text, minigame art) to the compiled ROM and fixes the header.

usage: pack_rom.py build/game.gb build/databank.bin build/moonlight-holler.gb
"""
import json
import sys
from pathlib import Path

BANK = 0x4000
SIZE_CODE = {2: 0x00, 4: 0x01, 8: 0x02, 16: 0x03, 32: 0x04, 64: 0x05, 128: 0x06}
MBC5 = 0x19


def header_checksum(rom):
    x = 0
    for b in rom[0x134:0x14D]:
        x = (x - b - 1) & 0xFF
    return x


def fix_header(rom, banks):
    rom[0x147] = MBC5
    rom[0x148] = SIZE_CODE[banks]
    rom[0x149] = 0x00
    rom[0x14D] = header_checksum(rom)
    rom[0x14E] = rom[0x14F] = 0
    total = (sum(rom) - rom[0x14E] - rom[0x14F]) & 0xFFFF
    rom[0x14E], rom[0x14F] = total >> 8, total & 0xFF


def main():
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    game, data_path, out = (Path(a) for a in sys.argv[1:])
    info = json.loads((game.parent / "rom_info.json").read_text())
    banks = info["rom_banks"]
    rom = bytearray(game.read_bytes())
    data = data_path.read_bytes()
    spill = [b for b in rom[0x8000:] if b not in (0x00, 0xFF)]
    if spill:
        sys.exit("The compiled code is larger than 32 KB (banks 0-1), so it would collide with the data banks.\n"
                 "See README.md, section 'If the build says the code is too big'.")
    code_end = max((i for i, b in enumerate(rom[:0x8000]) if b not in (0x00, 0xFF)), default=0) + 1
    rom = rom[:0x8000].ljust(0x8000, b"\xff")
    rom += data
    rom = rom.ljust(banks * BANK, b"\xff")
    if len(rom) != banks * BANK:
        sys.exit(f"data is {len(data)} bytes but the ROM only has {banks} banks")
    fix_header(rom, banks)
    out.write_bytes(rom)
    print(f"packed {out}: {len(rom) // 1024} KB, code uses {code_end} of 32768 bytes in banks 0-1, "
          f"{len(data)} bytes of data in banks 2-{banks - 1}")


if __name__ == "__main__":
    main()
