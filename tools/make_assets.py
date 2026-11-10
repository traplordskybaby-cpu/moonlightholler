#!/usr/bin/env python3
"""Builds all art, maps and text for Moonlight Holler.

Outputs
  src/assets.h        small tables that stay in the always-visible ROM (sprites, font, music)
  src/art.h           declarations, constants, and the address of every banked blob
  build/databank.bin  ROM banks 2.. (cover art, areas, dialogue, hockey and brawler art)
  build/rom_info.json what tools/pack_rom.py needs

Run from the project root:   python3 tools/make_assets.py
"""
from __future__ import annotations

import json
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import art_base as base  # noqa: E402
import areas  # noqa: E402
import dialog  # noqa: E402
import npc_art  # noqa: E402
import worldkit  # noqa: E402
from gbasset import rgb15  # noqa: E402

BANK_SIZE = 0x4000
FIRST_DATA_BANK = 2
RESIDENT = {"spr_tiles", "font_tiles", "farm_spr_pal"}


class BankPacker:
    def __init__(self):
        self.banks = [bytearray()]
        self.items = {}

    def add(self, name, data):
        data = bytes(data)
        if len(data) > BANK_SIZE:
            raise ValueError(f"{name} is {len(data)} bytes: larger than a bank")
        cur = self.banks[-1]
        if len(cur) + len(data) > BANK_SIZE:
            self.banks.append(bytearray())
            cur = self.banks[-1]
        bank = FIRST_DATA_BANK + len(self.banks) - 1
        addr = 0x4000 + len(cur)
        cur += data
        self.items[name] = (bank, addr, len(data))
        return bank, addr, len(data)

    def image(self):
        out = bytearray()
        for b in self.banks:
            out += bytes(b).ljust(BANK_SIZE, b"\xff")
        return bytes(out)


def parse_arrays(text):
    arrays = {}
    for m in re.finditer(r"const (unsigned char|palette_color_t) (\w+)\[\] = \{(.*?)\};", text, re.S):
        kind, name, body = m.groups()
        vals = [int(v, 16) for v in re.findall(r"0x([0-9A-Fa-f]+)", body)]
        data = bytes(vals) if kind == "unsigned char" else b"".join(struct.pack("<H", v) for v in vals)
        arrays[name] = (kind, data)
    return arrays


def c_bytes(name, data, kind="unsigned char"):
    if kind == "unsigned char":
        return f"const unsigned char {name}[] = {{{', '.join(f'0x{v:02X}' for v in data)}}};\n"
    vals = struct.unpack(f"<{len(data) // 2}H", data)
    return f"const palette_color_t {name}[] = {{{', '.join(f'0x{v:04X}' for v in vals)}}};\n"


def is_resident(name):
    return name in RESIDENT or name.startswith("music") or name.startswith("farm_crop")


def build_cover():
    data = json.loads((ROOT / "art" / "cover.json").read_text())
    pal = b"".join(struct.pack("<4H", *[rgb15(*c) for c in p]) for p in data["palettes"])
    n = data["width_tiles"] * data["height_tiles"]
    cmap, cattr, tiles = bytearray(), bytearray(), bytearray()
    for i in range(n):
        bank = 1 if i >= 256 else 0
        cmap.append(i - 256 if bank else i)
        cattr.append(data["tile_palette"][i] | (bank << 3))
        tiles += bytes.fromhex(data["tiles"][i])
    assert len(pal) == 64 and len(cmap) == 320 and len(tiles) == 5120
    return bytes(pal + cmap + cattr + tiles), data["text_palette"]


def main():
    # 1. legacy builders: sprites (now with villagers), font, hockey, brawler, music
    base.build_sprites()
    base.build_font()
    base.build_hockey()
    base.build_brawler()
    base.build_music()
    arrays = parse_arrays("".join(base.OUT))
    defines = list(base.DEFINES)

    # 2. places
    built = [fn() for fn in areas.ALL_AREAS]
    farm = built[areas.AREA_FARM]
    arrays["farm_crop_tile"] = ("unsigned char", farm.crop_tiles)
    arrays["farm_crop_attr"] = ("unsigned char", farm.crop_attr)

    # 3. dialogue
    dialog.check(base.FONT_CHARS)
    dnames, dblob = dialog.build()

    # 4. NPC look tables
    bases = []
    for name, *_rest in npc_art.LOOKS:
        line = next(x for x in defines if x.startswith(f"#define SP_{name} "))
        bases.append(int(line.split()[-1]))
    arrays["npc_base"] = ("unsigned char", bytes(bases))
    arrays["npc_head_pal"] = ("unsigned char", bytes(l[4] for l in npc_art.LOOKS))
    arrays["npc_body_pal"] = ("unsigned char", bytes(l[5] for l in npc_art.LOOKS))

    # 5. pack into banks
    packer = BankPacker()
    cover_blob, text_pal = build_cover()
    packer.add("cover", cover_blob)
    for a in built:
        packer.add("area_" + a.name, a.blob(dnames))
    packer.add("dialog", dblob)
    resident_text, decl, banked_defs = [], [], []
    for name, (kind, data) in sorted(arrays.items()):
        if is_resident(name) or name.startswith("npc_"):
            resident_text.append(c_bytes(name, data, kind))
            ctype = "unsigned char" if kind == "unsigned char" else "palette_color_t"
            decl.append(f"extern const {ctype} {name}[];\n")
        else:
            bank, addr, size = packer.add(name, data)
            ctype = "const unsigned char *" if kind == "unsigned char" else "const palette_color_t *"
            banked_defs.append(f"#define {name} (({ctype})bk_get({bank}, 0x{addr:04X}, {size}))\n")

    ab = [packer.items["area_" + a.name][0] for a in built]
    aa = [packer.items["area_" + a.name][1] for a in built]
    resident_text.append(c_bytes("area_bank", bytes(ab)))
    resident_text.append("const unsigned int area_addr[] = {" + ", ".join(f"0x{v:04X}" for v in aa) + "};\n")
    decl.append("extern const unsigned char area_bank[];\nextern const unsigned int area_addr[];\n")

    # 6. constants
    consts = []
    for i, a in enumerate(built):
        consts.append(f"#define AREA_{a.name.upper()} {i}\n")
    consts.append(f"#define AREA_COUNT {len(built)}\n")
    for k, v in (("HS_TALK", worldkit.HS_TALK), ("HS_ARCADE", worldkit.HS_ARCADE), ("HS_HOCKEY", worldkit.HS_HOCKEY),
                 ("HS_BED", worldkit.HS_BED), ("HS_SHOP", worldkit.HS_SHOP), ("R_PEARL", worldkit.R_PEARL),
                 ("R_SHOP", worldkit.R_SHOP), ("R_DAISY", worldkit.R_DAISY), ("R_WENDELL", worldkit.R_WENDELL),
                 ("R_GUS", worldkit.R_GUS), ("FLAG_INDOOR", worldkit.FLAG_INDOOR),
                 ("T_HUD", 128), ("T_HEART", 129), ("T_HEART_EMPTY", 130), ("T_BOX", 131)):
        consts.append(f"#define {k} {v}\n")
    for i, n in enumerate(dnames):
        consts.append(f"#define DLG_{n} {i}\n")
    consts.append(f"#define DLG_COUNT {len(dnames)}\n")
    for key in ("cover", "dialog"):
        bank, addr, _size = packer.items[key]
        consts.append(f"#define {key.upper()}_BANK {bank}\n#define {key.upper()}_ADDR 0x{addr:04X}\n")
    consts.append(f"#define COVER_TEXT_PAL {text_pal}\n")

    image = packer.image()
    total_banks = FIRST_DATA_BANK + len(packer.banks)
    rom_banks = 4
    while rom_banks < total_banks:
        rom_banks *= 2
    consts.append(f"#define ROM_BANKS {rom_banks}\n")

    (ROOT / "src" / "assets.h").write_text("#include <gb/cgb.h>\n" + "".join(resident_text), encoding="utf-8")
    header = ["#ifndef ART_H\n#define ART_H\n#include <gb/cgb.h>\n#include \"bank.h\"\n"] + decl + banked_defs + defines + consts + ["#endif\n"]
    (ROOT / "src" / "art.h").write_text("".join(header), encoding="utf-8")

    build = ROOT / "build"
    build.mkdir(parents=True, exist_ok=True)
    (build / "databank.bin").write_bytes(image)
    (build / "rom_info.json").write_text(json.dumps({"rom_banks": rom_banks, "data_banks": len(packer.banks),
                                                      "items": packer.items}, indent=1))
    prev = ROOT / "art" / "preview"
    prev.mkdir(parents=True, exist_ok=True)
    for a in built:
        worldkit.render(a, scale=3).save(prev / f"{a.name}.png")

    used = sum(len(b) for b in packer.banks)
    print("areas:", ", ".join(f"{a.name}({len(a.tiles)} tiles)" for a in built))
    print(f"banked data: {used} bytes in {len(packer.banks)} bank(s); ROM will be {rom_banks} banks = {rom_banks * 16} KB")
    for k, (bank, addr, size) in sorted(packer.items.items(), key=lambda kv: (kv[1][0], kv[1][1])):
        print(f"  bank {bank} 0x{addr:04X} {size:6d}  {k}")


if __name__ == "__main__":
    main()
