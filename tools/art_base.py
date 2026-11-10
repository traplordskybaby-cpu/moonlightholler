# Built with love and passion on retroable.dev

"""Writes src/assets.h, src/art.h, and build/music.json. Change the tile pictures and the melody here.

Background tiles live at 128-255 (scene art in VRAM bank 0, the font in bank 1); sprite tiles at 0-127,
so the two never overlap whichever way the LCD addresses tiles."""

from __future__ import annotations

import json
import math
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(Path(__file__).resolve().parent))
from gbasset import build_song, c_array, c_externs, note_hz, rgb15, tile_2bpp

BKG_BASE = 128
DEFINES: list[str] = []
OUT: list[str] = []


def define(name: str, value: int) -> None:
    DEFINES.append(f"#define {name} {value}\n")


def palettes(name: str, pals: list[list[tuple[int, int, int]]]) -> None:
    body = ", ".join(f"0x{rgb15(*c):04X}" for pal in pals for c in pal)
    OUT.append(f"const palette_color_t {name}[] = {{{body}}};\n")


def night(pals):
    """Moonlight: darker and bluer."""
    out = []
    for pal in pals:
        out.append([(r * 2 // 5, g * 2 // 5, min(31, b * 3 // 5 + 6)) for r, g, b in pal])
    return out


def flip(rows):
    return [r[::-1] for r in rows]


def overlay(base, top):
    return ["".join(t if t != "." else b for b, t in zip(br, tr)) for br, tr in zip(base, top)]


def recolor(rows, mapping):
    return ["".join(mapping.get(ch, ch) for ch in r) for r in rows]


def load_font():
    from fontdata import GLYPHS
    glyphs = GLYPHS
    chars = " " + "abcdefghijklmnopqrstuvwxyz" + "ABCDEFGHIJKLMNOPQRSTUVWXYZ" + "0123456789" + ",.-"
    font = dict(zip(chars, glyphs))
    font[":"] = ["00000000", "00000000", "00110000", "00110000", "00000000", "00110000", "00110000", "00000000"]
    font["!"] = ["00000000", "00110000", "00110000", "00110000", "00110000", "00000000", "00110000", "00000000"]
    font["?"] = ["00111000", "01000100", "00000100", "00001000", "00010000", "00000000", "00010000", "00000000"]
    font["'"] = ["00110000", "00110000", "00100000", "01000000", "00000000", "00000000", "00000000", "00000000"]
    font[">"] = ["01000000", "00100000", "00010000", "00001000", "00010000", "00100000", "01000000", "00000000"]
    font["/"] = ["00000100", "00001000", "00001000", "00010000", "00010000", "00100000", "00100000", "01000000"]
    font["+"] = ["00000000", "00010000", "00010000", "01111100", "00010000", "00010000", "00000000", "00000000"]
    font["$"] = ["00010000", "00111100", "01010000", "00111000", "00010100", "01111000", "00010000", "00000000"]
    return chars + ":!?'>/+$", font


FONT_CHARS, FONT = load_font()


def glyph(ch, bg, fg):
    return [r.replace("0", "b").replace("1", "f").replace("b", str(bg)).replace("f", str(fg)) for r in FONT[ch]]


def text_strip(text, width_tiles, bg, fg):
    """Proportional text, centred, as a row of tiles."""
    cols = []
    for ch in text:
        g = FONT[ch]
        used = [x for x in range(8) if any(r[x] == "1" for r in g)]
        lo, hi = (used[0], used[-1]) if used else (0, 1)
        for x in range(lo, hi + 1):
            cols.append([g[y][x] for y in range(8)])
        cols.append(["0"] * 8)
    cols = cols[:-1]
    width = width_tiles * 8
    if len(cols) > width:
        raise ValueError(f"{text!r} is {len(cols)} px, wider than {width}")
    pad = (width - len(cols)) // 2
    cols = [["0"] * 8] * pad + cols + [["0"] * 8] * (width - pad - len(cols))
    tiles = []
    for t in range(width_tiles):
        rows = []
        for y in range(8):
            rows.append("".join(str(fg) if cols[t * 8 + x][y] == "1" else str(bg) for x in range(8)))
        tiles.append(rows)
    return tiles


class Scene:
    def __init__(self, name, w, h):
        self.name, self.w, self.h = name, w, h
        self.tiles: list[bytes] = []
        self.index: dict[bytes, int] = {}
        self.map = [[BKG_BASE] * w for _ in range(h)]
        self.attr = [[0] * w for _ in range(h)]

    def tile(self, rows):
        data = tile_2bpp(rows)
        if data not in self.index:
            self.index[data] = len(self.tiles)
            self.tiles.append(data)
        return BKG_BASE + self.index[data]

    def put(self, x, y, rows, pal):
        self.map[y][x] = self.tile(rows)
        self.attr[y][x] = pal

    def fill(self, x0, y0, x1, y1, rows, pal):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.put(x, y, rows, pal)

    def emit(self, maps=True):
        if len(self.tiles) > 128:
            raise ValueError(f"{self.name}: {len(self.tiles)} tiles, more than 128")
        OUT.append(c_array(f"{self.name}_tiles", b"".join(self.tiles)))
        define(f"{self.name.upper()}_TILE_COUNT", len(self.tiles))
        if maps:
            OUT.append(c_array(f"{self.name}_map", bytes(v for row in self.map for v in row)))
            OUT.append(c_array(f"{self.name}_attr", bytes(v for row in self.attr for v in row)))


class Canvas:
    """A pixel picture sliced into tiles; each tile row band picks a palette."""

    def __init__(self, w, h, fill=0):
        self.w, self.h = w, h
        self.px = [[fill] * w for _ in range(h)]

    def set(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y][x] = c

    def rect(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.set(x, y, c)

    def disc(self, cx, cy, r, c):
        for y in range(int(cy - r - 1), int(cy + r + 2)):
            for x in range(int(cx - r - 1), int(cx + r + 2)):
                if (x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2 <= r * r:
                    self.set(x, y, c)

    def ring(self, cx, cy, r, c):
        for y in range(int(cy - r - 1), int(cy + r + 2)):
            for x in range(int(cx - r - 1), int(cx + r + 2)):
                d = math.hypot(x + 0.5 - cx, y + 0.5 - cy)
                if r - 0.7 <= d <= r + 0.3:
                    self.set(x, y, c)

    def tile(self, tx, ty):
        return ["".join(str(self.px[ty * 8 + y][tx * 8 + x]) for x in range(8)) for y in range(8)]


FARM_PALS = [
    [(16, 27, 7), (9, 20, 4), (9, 5, 2), (31, 27, 5)],
    [(18, 26, 31), (31, 31, 31), (28, 5, 4), (3, 5, 14)],
    [(28, 5, 4), (31, 31, 29), (13, 2, 2), (6, 6, 8)],
    [(26, 20, 11), (17, 10, 4), (8, 4, 1), (31, 25, 6)],
    [(18, 11, 5), (7, 22, 5), (30, 4, 4), (9, 5, 2)],
    [(18, 11, 5), (7, 22, 5), (31, 27, 5), (9, 5, 2)],
    [(30, 28, 20), (23, 14, 6), (11, 6, 2), (6, 18, 6)],
    [(31, 31, 31), (8, 20, 31), (2, 3, 12), (31, 4, 4)],
]

GRASS = ["00000000", "00010000", "00101000", "00000000", "00000010", "01000101", "10100000", "00000000"]
GRASS2 = ["00000000", "00030000", "00323000", "00030000", "00010010", "00000101", "01000000", "10100000"]
SUNFLOWER = ["03333300", "33333330", "33222330", "33222330", "33222330", "03333300", "01010000", "00110000"]
SKY = ["00000000"] * 8
CLOUD_L = ["00000000", "00000111", "00011111", "01111111", "11111111", "01111111", "00000000", "00000000"]
CLOUD_R = ["00000000", "11000000", "11110000", "11111100", "11111110", "11111100", "00000000", "00000000"]
POST = ["00022000"] * 8
ROOF_L = ["00000001", "00000012", "00000122", "00001222", "00012222", "00122222", "01222222", "11111111"]
ROOF = ["11111111", "00000000", "22222222", "00000000", "00000000", "22222222", "00000000", "11111111"]
LOFT_L = ["11111111", "00000000", "00011111", "00013333", "00013333", "00013333", "00011111", "11111111"]
WALL = ["00200020", "00200020", "00200020", "00200020", "00200020", "00200020", "00200020", "00200020"]
WALL_L = ["11" + r[2:] for r in WALL]
WALL_R = [r[:6] + "11" for r in WALL]
HAY_BALE = ["22222222", "23333332", "23331332", "21111112", "23333332", "23133332", "23333332", "22222222"]
FENCE_H = ["00011000", "11111111", "22222222", "00011000", "11111111", "22222222", "00011000", "00022000"]
FENCE_V = ["00121000", "00121000", "01111100", "00222000", "00121000", "00121000", "01111100", "00222000"]
STRAW = ["33313333", "31333133", "33331333", "13333313", "33133333", "33333131", "31333333", "33313313"]
HUTCH_TL = ["33333322", "33332211", "33221111", "22111111", "22222222", "30000000", "30202020", "30000000"]
HUTCH_BL = ["30000000", "30000022", "30000222", "30202222", "30000222", "30000222", "30000222", "22222222"]
PATH = ["00000000", "00010000", "00000000", "00000100", "01000000", "00000000", "00001000", "00000000"]
PATH2 = ["00000000", "00000000", "01000000", "00000000", "00000010", "00100000", "00000000", "00000000"]

TILLED = ["00000000", "33333333", "00000000", "00000000", "33333333", "00000000", "00000000", "33333333"]
WATERED = ["33333333", "00000000", "33333333", "33333333", "00000000", "33333333", "33333333", "00000000"]
SPROUT = ["........", "..1.1...", "...1....", "...1....", "........", ".....1.1", "......1.", "......1."]
CUCUMBER = [".11..2..", "1111.222", ".11.3.2.", "...313..", "..3113..", ".3113.11", ".313.111", "..3..11."]
PEPPER = [".1111.1.", "11111111", ".1.11.1.", ".22..22.", "2222.222", "2222.222", ".22...22", "..2....2"]
LETTUCE = ["..1111..", ".111111.", "11311311", "11131111", "13111131", "11131111", ".111111.", "..1111.."]
CORN = ["...22...", "..2222..", ".122221.", "1.2222.1", "1.2222.1", ".112211.", "...11...", "..1..1.."]
HAY = ["20202202", "22202222", "22222220", "02222122", "22122222", "22222212", "21222222", "12121212"]

WALL_WIN = ["00000000", "02222220", "02333320", "02333320", "02222220", "02333320", "02222220", "00000000"]
WALL_LOW = ["00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "11111111", "22222222"]
LOG = ["12121212", "12121212", "22222222", "21212121", "21212121", "22222222", "12121212", "12121212"]
FLOOR = ["11111112", "11111112", "11111112", "22222222", "11121111", "11121111", "11121111", "22222222"]
RUG = ["33333333", "33303333", "33333333", "33333303", "33333333", "30333333", "33333333", "33333033"]
SOFA_L = ["22222222", "23333333", "23300000", "23300000", "23300000", "23333333", "22222222", "32333333"]
SOFA_M = ["22222222", "33333333", "00000002", "00000002", "00000002", "33333333", "22222222", "33333333"]
PLANT = ["11311311", "13333331", "33313333", "13333331", "11333311", "11222211", "11222211", "11122111"]
ARC_TL = ["22222222", "21111111", "21222222", "21203222", "21233322", "21203222", "21232322", "21222222"]
ARC_TR = ["22222222", "11111112", "22222212", "22202212", "22000212", "22202212", "22020212", "22222212"]
ARC_ML = ["21111111", "11111111", "12322222", "12122222", "12222222", "21111111", "21222222", "21222222"]
ARC_MR = ["11111112", "11111111", "22222221", "22303021", "22222221", "11111112", "22222212", "22222212"]
ARC_BL = ["21222222", "21222222", "21223322", "21222222", "21222222", "21222222", "21111111", "22222222"]
ARC_BR = ["22222212", "22222212", "22332212", "22222212", "22222212", "22222212", "11111112", "22222222"]
STOVE_T = ["22222222", "21111112", "22222222", "22222222", "22000022", "20333302", "20333302", "22333322"]
STOVE_B = ["22333322", "20333302", "22000022", "22222222", "22222222", "21111112", "22222222", "22222222"]

HEART = ["00000000", "03303300", "31232230", "32222230", "32222230", "03222300", "00323000", "00030000"]
HEART_EMPTY = recolor(HEART, {"1": "0", "2": "0"})


def hockey_table():
    """The living room's air hockey table from above: 4x2 tiles, palette BP7."""
    c = Canvas(32, 16, 0)
    c.rect(0, 0, 31, 15, 2)
    c.rect(1, 1, 30, 14, 1)
    c.rect(2, 2, 29, 13, 0)
    c.rect(15, 2, 16, 13, 3)
    c.ring(16, 8, 4, 3)
    c.rect(1, 5, 2, 10, 2)
    c.rect(29, 5, 30, 10, 2)
    c.disc(8, 8, 2.6, 3)
    c.disc(24, 8, 2.6, 3)
    c.disc(20.5, 5.5, 1.5, 2)
    return [[c.tile(x, y) for x in range(4)] for y in range(2)]


def door_leaf():
    c = Canvas(8, 16, 0)
    c.rect(0, 0, 7, 0, 1)
    c.rect(0, 0, 0, 15, 1)
    c.rect(7, 0, 7, 15, 1)
    for y in range(16):
        x = round(y * 7 / 15)
        c.set(x, y, 1)
        c.set(7 - x, y, 1)
    return c.tile(0, 0), c.tile(0, 1)


def build_farm():
    s = Scene("farm", 22, 16)
    s.fill(0, 0, 21, 15, GRASS, 0)
    for x, y in [(1, 4), (0, 7), (20, 9), (11, 14), (14, 15), (17, 9), (1, 12)]:
        s.put(x, y, GRASS2, 0)
    s.fill(0, 0, 21, 1, SKY, 1)
    for x, y in [(12, 0), (19, 0), (0, 1)]:
        s.put(x, y, CLOUD_L, 1)
        s.put(x + 1, y, CLOUD_R, 1)
    for i, t in enumerate(text_strip("MOONLIGHT", 10, 2, 3)):
        s.put(2 + i, 0, t, 3)
    for i, t in enumerate(text_strip("HOLLER", 10, 2, 3)):
        s.put(2 + i, 1, t, 3)
    s.put(3, 2, POST, 0)
    s.put(10, 2, POST, 0)
    s.put(13, 1, ROOF_L, 1)
    s.put(18, 1, flip(ROOF_L), 1)
    s.fill(14, 1, 17, 1, ROOF, 2)
    s.put(15, 1, LOFT_L, 2)
    s.put(16, 1, flip(LOFT_L), 2)
    top, bottom = door_leaf()
    for y in (2, 3):
        s.put(13, y, WALL_L, 2)
        s.put(14, y, WALL, 2)
        s.put(17, y, WALL, 2)
        s.put(18, y, WALL_R, 2)
    s.put(15, 2, top, 2)
    s.put(16, 2, top, 2)
    s.put(15, 3, bottom, 2)
    s.put(16, 3, bottom, 2)
    for x, y in [(12, 3), (19, 3), (20, 3), (20, 2)]:
        s.put(x, y, HAY_BALE, 3)
    s.fill(3, 4, 7, 7, STRAW, 3)
    s.fill(2, 3, 8, 3, FENCE_H, 3)
    s.fill(2, 8, 8, 8, FENCE_H, 3)
    s.fill(2, 4, 2, 7, FENCE_V, 3)
    s.fill(8, 4, 8, 7, FENCE_V, 3)
    s.put(8, 6, STRAW, 3)
    s.put(6, 4, HUTCH_TL, 3)
    s.put(7, 4, flip(HUTCH_TL), 3)
    s.put(6, 5, HUTCH_BL, 3)
    s.put(7, 5, flip(HUTCH_BL), 3)
    s.fill(9, 4, 21, 4, PATH, 3)
    s.fill(9, 5, 10, 15, PATH, 3)
    for x, y in [(12, 4), (17, 4), (9, 7), (10, 11), (20, 4)]:
        s.put(x, y, PATH2, 3)
    s.put(9, 14, HAY_BALE, 3)
    s.fill(11, 5, 20, 5, FENCE_H, 3)
    crops = [(CUCUMBER, 5), (PEPPER, 4), (LETTUCE, 4), (CORN, 5), (HAY, 5)]
    crop_tiles = []
    crop_attr = []
    for art, pal in crops:
        ripe = overlay(TILLED, art) if art is not HAY else HAY
        for rows in (TILLED, WATERED, overlay(TILLED, SPROUT), overlay(WATERED, SPROUT), ripe):
            crop_tiles.append(s.tile(rows))
        crop_attr.append(pal)
    for row in range(6, 9):
        for col in range(11, 19):
            kind = (col - 11) // 2
            s.map[row][col] = crop_tiles[kind * 5 + 4]
            s.attr[row][col] = crop_attr[kind]
    for row in range(10, 14):
        for col in range(12, 19):
            s.map[row][col] = crop_tiles[4 * 5 + 4]
            s.attr[row][col] = crop_attr[4]
    OUT.append(c_array("farm_crop_tile", bytes(crop_tiles)))
    OUT.append(c_array("farm_crop_attr", bytes(crop_attr)))
    for x, y in [(19, 10), (21, 10), (20, 11), (19, 12), (21, 12), (20, 13), (19, 14), (21, 14), (0, 3), (1, 9)]:
        s.put(x, y, SUNFLOWER, 0)
    s.fill(1, 9, 1, 15, LOG, 6)
    s.put(2, 9, WALL_WIN, 6)
    s.put(2, 10, WALL_LOW, 6)
    line1 = text_strip("Dirt Road", 6, 2, 0)
    line2 = text_strip("Brawler 2", 6, 2, 0)
    for i in range(6):
        s.put(3 + i, 9, ["111111111"[:8]] + line1[i][1:], 7)
        s.put(3 + i, 10, line2[i][:7] + ["11111111"], 7)
    s.fill(2, 11, 8, 15, FLOOR, 6)
    s.put(2, 11, ARC_TL, 7)
    s.put(3, 11, ARC_TR, 7)
    s.put(2, 12, ARC_ML, 7)
    s.put(3, 12, ARC_MR, 7)
    s.put(2, 13, ARC_BL, 7)
    s.put(3, 13, ARC_BR, 7)
    table = hockey_table()
    for y in range(2):
        for x in range(4):
            s.put(4 + x, 11 + y, table[y][x], 7)
    s.put(8, 11, STOVE_T, 7)
    s.put(8, 12, STOVE_B, 7)
    s.fill(3, 14, 7, 15, RUG, 6)
    s.put(4, 15, SOFA_L, 6)
    s.put(5, 15, SOFA_M, 6)
    s.put(6, 15, flip(SOFA_L), 6)
    s.put(8, 14, PLANT, 6)
    s.put(2, 15, PLANT, 6)
    spec = json.loads((ROOT / "spec.json").read_text(encoding="utf-8"))
    rows = next(c["rows"] for c in spec["collision"] if c["section"] == "sec_town")
    if len(rows) != s.h or any(len(r) != s.w for r in rows):
        raise ValueError("sec_town collision must be 22 x 16")
    bits = bytearray()
    for r in rows:
        for b in range(3):
            bits.append(sum(0x80 >> i for i in range(8) if b * 8 + i < len(r) and r[b * 8 + i] == "#"))
    OUT.append(c_array("farm_solid_bits", bytes(bits)))
    define("FARM_T_HEART", s.tile(HEART))
    define("FARM_T_HEART_EMPTY", s.tile(HEART_EMPTY))
    define("FARM_T_HUD", s.tile(SKY))
    s.emit()
    palettes("farm_bkg_day", FARM_PALS)
    palettes("farm_bkg_night", night(FARM_PALS))
    return s


SPRITES: list[tuple[str, list[str]]] = []


def sprite(name, rows):
    define(f"SP_{name}", len(SPRITES))
    SPRITES.append((name, rows))


def sprite16(name, rows16):
    """16-pixel-wide art (8 or 16 rows) as TL, TR, BL, BR tiles."""
    sprite(name, [r[:8] for r in rows16[:8]])
    SPRITES.append((name + "_tr", [r[8:] for r in rows16[:8]]))
    if len(rows16) > 8:
        SPRITES.append((name + "_bl", [r[:8] for r in rows16[8:16]]))
        SPRITES.append((name + "_br", [r[8:] for r in rows16[8:16]]))


P1_TOP = [
    "0000033333300000",
    "0003311111133000",
    "0031111111111300",
    "0311131111311130",
    "3111111111111113",
    "3311111111111133",
    "0033222222223300",
    "0003232222323000",
]
P1_BOTTOM = [
    "0001112222111000",
    "0011122222211330",
    "0111223223221322",
    "0110222222220222",
    "0000222222220222",
    "0000222002220330",
    "0000222002220000",
    "0003330003330000",
]
P1_WALK = P1_BOTTOM[:5] + ["0000222002220330", "0000333002220000", "0000000003330000"]
P2_TOP = [
    "3030000000000000",
    "3030033333300000",
    "3330311111113000",
    "0303111111111300",
    "0303111111111300",
    "0303311111111330",
    "0300322222223000",
    "0300323222323000",
]
P2_BOTTOM = [
    "0301112222111000",
    "0311122222211100",
    "0311223223221110",
    "0310222222220110",
    "0300222222220000",
    "0300222002220000",
    "0300222002220000",
    "0303330003330000",
]
P2_WALK = P2_BOTTOM[:5] + ["0300222002220000", "0300333002220000", "0300000003330000"]

PIG = [
    "0000333333330000",
    "0033AAAABBBB3300",
    "03AAAAAABBBB3330",
    "3AAAAAAABBBBB3B3",
    "3AAAAAAABBBBBBB3",
    "3AAAAAAABBBBBB30",
    "03AAAAAAABBBB300",
    "0033303330333000",
]
CHICKEN = ["00000200", "00002210", "00001113", "10001120", "11111110", "11111110", "01111100", "00303000"]
CHICKEN_PECK = ["00000000", "00000000", "10000000", "11111200", "11111120", "11111112", "01111113", "00303003"]
CROW = ["00011000", "03121000", "00111100", "00111110", "00121111", "00011111", "00003030", "00000000"]
CROW_FLY = ["10000001", "11000011", "11111111", "01121110", "00113100", "00011000", "00000000", "00000000"]
DROP = ["00030000", "00323000", "03222300", "32221230", "32222130", "32222230", "03222300", "00333000"]
BUBBLE = ["03333330", "31111113", "31311313", "31311313", "31111113", "31311313", "03333330", "03300000"]
BOWL = ["00000000", "00000000", "03333330", "32222223", "32121223", "32222223", "03333330", "00000000"]
ICON_CUKE = ["00000000", "00000330", "00003113", "00031313", "00311130", "03131300", "31113000", "03330000"]
ICON_HAY = ["01010100", "01111110", "00111100", "00333300", "00111100", "01111110", "11111111", "10101010"]
ICON_PEPPER = ["00033000", "00330000", "02222200", "22122220", "22122220", "22222220", "02222200", "00220000"]
ICON_LETTUCE = ["00000000", "00111100", "01131310", "11311131", "13111311", "11131111", "01111110", "00111100"]
ICON_CORN = ["00030000", "00313000", "03111300", "03131300", "03111300", "03131300", "00313000", "00030000"]
ICON_PIG = ["03300330", "32333313", "22222111", "23222131", "22222111", "22233111", "02221110", "00333300"]
SUN = ["20022002", "02011020", "00111100", "21111112", "21111112", "00111100", "02011020", "20022002"]
MOON = ["00011100", "00111000", "01110000", "01110000", "01110000", "01111000", "00111100", "00011100"]


def mallet():
    c = Canvas(16, 16, 0)
    c.disc(8, 8, 7.5, 2)
    c.disc(8, 8, 6.3, 1)
    c.disc(8, 8, 3.2, 2)
    c.disc(8, 8, 2.2, 1)
    c.disc(7, 7, 1.0, 3)
    return [c.tile(0, 0)[y] + c.tile(1, 0)[y] for y in range(8)] + [c.tile(0, 1)[y] + c.tile(1, 1)[y] for y in range(8)]


PUCK = ["00000000", "00111100", "01133110", "11311111", "11111111", "11111111", "01111110", "00111100"]
CURSOR = ["00011000", "00111100", "01111110", "11111111", "00111100", "00111100", "00111100", "00000000"]
SPARK = ["10010010", "01011100", "00122100", "11222211", "00122110", "01011100", "10010010", "00010000"]

HEADS = [
    ["0000222222000000", "0000222222200000", "0222222222222200", "0000111111110000",
     "0000111113110000", "0000111111111000", "0000111111110000", "0000011111100000"],
    ["0000022222200000", "0000222222222000", "0022222111111000", "0222221111311000",
     "0220221111111100", "0200021111111000", "0000001111110000", "0000000111100000"],
    ["0000011111100000", "0000111111110000", "0000111113110000", "0000111111111000",
     "0000222222221000", "0000222222220000", "0000022222200000", "0000002222000000"],
    ["0000022222200000", "0000222222222220", "0000111111110000", "0000111113110000",
     "0000111111111000", "0000111111110000", "0000011111100000", "0000001111000000"],
]
TORSO = ["0000011111100000", "0000111111110000", "0001111111111000", "0001111111111000",
         "0001111111111000", "0000111111110000", "0000222222220000", "0000222222220000"]
TORSO_PUNCH = ["0000011111100000", "0000111111110000", "0001111111111111", "0001111111111111",
               "0001111111110000", "0000111111110000", "0000222222220000", "0000222222220000"]
LEGS = ["0000222002220000", "0000222002220000", "0000222002220000", "0000222002220000",
        "0000222002220000", "0000222002220000", "0003333003333000", "0003333003333000"]
LEGS_WALK = ["0000222222200000", "0002220002220000", "0002220000222000", "0022200000222000",
             "0022200000022200", "0222000000022200", "3333000000033330", "3333000000033330"]
LEGS_KICK = ["0000222222222222", "0000222222222222", "0000222000000000", "0000222000000000",
             "0000222000000000", "0000222000000000", "0003333000000000", "0003333000000000"]
FIST = ["00000000", "00000000", "11110000", "11113000", "11110000", "00000000", "00000000", "00000000"]
BOOT = ["33300000", "33330000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"]


def build_sprites():
    sprite16("P1", P1_TOP + P1_BOTTOM)
    SPRITES.append(("P1_walk_bl", [r[:8] for r in P1_WALK]))
    SPRITES.append(("P1_walk_br", [r[8:] for r in P1_WALK]))
    sprite16("P2", P2_TOP + P2_BOTTOM)
    SPRITES.append(("P2_walk_bl", [r[:8] for r in P2_WALK]))
    SPRITES.append(("P2_walk_br", [r[8:] for r in P2_WALK]))
    for name, a, b in (("PIG_A", "2", "1"), ("PIG_B", "3", "1"), ("PIG_C", "1", "2")):
        sprite16(name, recolor(PIG, {"A": a, "B": b}))
    sprite("CHICKEN", CHICKEN)
    sprite("CHICKEN_PECK", CHICKEN_PECK)
    sprite("CROW", CROW)
    sprite("CROW_FLY", CROW_FLY)
    sprite("DROP", DROP)
    sprite("BUBBLE", BUBBLE)
    sprite("BOWL", BOWL)
    sprite("ICON_CUKE", ICON_CUKE)
    sprite("ICON_HAY", ICON_HAY)
    sprite("ICON_PEPPER", ICON_PEPPER)
    sprite("ICON_LETTUCE", ICON_LETTUCE)
    sprite("ICON_CORN", ICON_CORN)
    sprite("ICON_PIG", ICON_PIG)
    sprite("SUN", SUN)
    sprite("MOON", MOON)
    sprite16("MALLET", mallet())
    sprite("PUCK", PUCK)
    sprite("CURSOR", CURSOR)
    sprite("SPARK", SPARK)
    for i, head in enumerate(HEADS):
        sprite16(f"HEAD{i}", head)
    sprite16("TORSO", TORSO)
    sprite16("TORSO_PUNCH", TORSO_PUNCH)
    sprite16("LEGS", LEGS)
    sprite16("LEGS_WALK", LEGS_WALK)
    sprite16("LEGS_KICK", LEGS_KICK)
    sprite("FIST", FIST)
    sprite("BOOT", BOOT)
    import npc_art
    for name, top, bottom, walk, hp, bp in npc_art.LOOKS:
        sprite16(name, top + bottom)
        SPRITES.append((name + "_walk_bl", [r[:8] for r in walk]))
        SPRITES.append((name + "_walk_br", [r[8:] for r in walk]))
    sprite("COIN", npc_art.COIN)
    if len(SPRITES) > 128:
        raise ValueError("more than 128 sprite tiles")
    OUT.append(c_array("spr_tiles", b"".join(tile_2bpp(rows) for _, rows in SPRITES)))
    define("SPR_TILE_COUNT", len(SPRITES))
    palettes(
        "farm_spr_pal",
        [
            [(0, 0, 0), (31, 27, 5), (31, 21, 15), (10, 5, 2)],
            [(0, 0, 0), (31, 14, 20), (6, 12, 30), (2, 3, 12)],
            [(0, 0, 0), (8, 16, 31), (31, 21, 15), (2, 3, 12)],
            [(0, 0, 0), (8, 24, 6), (6, 12, 30), (4, 4, 8)],
            [(0, 0, 0), (31, 31, 29), (31, 16, 4), (13, 6, 2)],
            [(0, 0, 0), (31, 31, 31), (31, 4, 4), (31, 22, 0)],
            [(0, 0, 0), (2, 2, 4), (13, 13, 16), (31, 24, 0)],
            [(0, 0, 0), (31, 31, 31), (14, 24, 31), (4, 8, 20)],
        ],
    )


def build_font():
    OUT.append(c_array("font_tiles", b"".join(tile_2bpp(glyph(ch, 0, 3)) for ch in FONT_CHARS)))
    define("FONT_TILE_COUNT", len(FONT_CHARS))


def build_hockey():
    s = Scene("hockey", 20, 18)
    wood = ["00000000", "00000000", "11111111", "00000000", "00000000", "00000000", "11111111", "00000000"]
    s.fill(0, 0, 19, 17, wood, 1)
    c = Canvas(112, 144, 0)
    c.rect(0, 4, 111, 139, 2)
    c.rect(3, 7, 108, 136, 1)
    c.rect(5, 9, 106, 134, 0)
    for y in range(12, 134, 6):
        for x in range(9 + (y // 6 % 2) * 3, 106, 6):
            c.set(x, y, 1)
    c.rect(40, 4, 71, 8, 3)
    c.rect(40, 135, 71, 139, 3)
    c.rect(5, 71, 106, 72, 3)
    c.ring(56, 72, 12, 3)
    c.ring(56, 9, 16, 1)
    c.ring(56, 134, 16, 1)
    for ty in range(18):
        for tx in range(14):
            s.put(3 + tx, ty, c.tile(tx, ty), 0)
    s.emit()
    palettes(
        "hockey_bkg_pal",
        [
            [(31, 31, 31), (12, 22, 31), (3, 5, 16), (31, 4, 4)],
            [(9, 5, 2), (14, 8, 3), (6, 18, 6), (31, 28, 20)],
        ],
    )
    palettes(
        "hockey_spr_pal",
        [[(0, 0, 0), (31, 6, 6), (14, 1, 1), (31, 31, 31)], [(0, 0, 0), (3, 4, 10), (12, 12, 16), (31, 31, 31)]],
    )


def build_brawler():
    s = Scene("brawl", 32, 16)
    c = Canvas(256, 128, 0)
    for cx, cy in [(30, 14), (158, 14), (92, 26), (220, 26)]:
        c.disc(cx, cy, 6, 1)
        c.disc(cx + 8, cy - 2, 7, 1)
        c.disc(cx + 16, cy, 5, 1)
        c.rect(cx - 4, cy + 1, cx + 20, cy + 5, 1)
    for x in range(256):
        top = int(50 + 7 * math.sin(x * 2 * math.pi / 64))
        for y in range(top, 64):
            c.set(x, y, 2 if y > top + 1 else 3)
    c.rect(0, 64, 255, 79, 0)
    for x in range(0, 256, 4):
        c.set(x, 66 + (x // 4) % 2, 1)
    c.rect(0, 68, 255, 69, 2)
    c.rect(0, 73, 255, 74, 2)
    for x in range(4, 256, 16):
        c.rect(x, 65, x + 2, 78, 2)
        c.rect(x + 2, 65, x + 2, 78, 3)
    c.rect(0, 80, 255, 127, 0)
    c.rect(0, 80, 255, 83, 3)
    c.rect(0, 124, 255, 127, 3)
    for x in range(0, 256, 8):
        c.set(x + 2, 84, 3)
        c.set(x + 5, 123, 3)
    for i, (x, y) in enumerate((x, y) for y in range(88, 122, 7) for x in range(0, 32, 11)):
        for k in range(0, 256, 32):
            c.set((x + y * 3) % 32 + k, y, 1 if i % 3 else 2)
    for ty in range(16):
        pal = 0 if ty < 8 else (1 if ty < 10 else 2)
        for tx in range(32):
            s.put(tx, ty, c.tile(tx, ty), pal)
    for n in range(9):
        define(f"BRAWL_T_BAR{n}", s.tile(["33333333"] + ["2" * n + "1" * (8 - n)] * 6 + ["33333333"]))
    define("BRAWL_T_BLANK", s.tile(["00000000"] * 8))
    s.emit()
    palettes(
        "brawl_bkg_pal",
        [
            [(16, 25, 31), (31, 31, 31), (8, 22, 6), (4, 13, 4)],
            [(10, 24, 6), (18, 29, 10), (16, 9, 3), (7, 4, 1)],
            [(26, 20, 11), (17, 11, 5), (9, 5, 2), (8, 20, 5)],
            [(2, 3, 10), (30, 4, 4), (31, 26, 4), (31, 31, 31)],
        ],
    )
    skin = (31, 21, 15)
    dark = (3, 2, 2)
    palettes(
        "brawl_fighter_pal",
        [
            [(0, 0, 0), skin, (31, 27, 5), dark], [(0, 0, 0), (31, 14, 20), (6, 12, 30), dark],
            [(0, 0, 0), skin, (28, 8, 2), dark], [(0, 0, 0), (8, 24, 6), (16, 9, 3), dark],
            [(0, 0, 0), skin, (14, 7, 2), dark], [(0, 0, 0), (28, 4, 4), (4, 6, 16), dark],
            [(0, 0, 0), skin, (20, 6, 26), dark], [(0, 0, 0), (31, 26, 4), (6, 6, 8), dark],
        ],
    )
    palettes("brawl_spark_pal", [[(0, 0, 0), (31, 31, 31), (31, 27, 4), (31, 6, 4)]])


NOTE_NAMES = {"C": 0, "C#": 1, "D": 2, "D#": 3, "E": 4, "F": 5, "F#": 6, "G": 7, "G#": 8, "A": 9, "A#": 10, "B": 11}
MELODY = (
    "G4 B4 D5 B4 G4 A4 B4 G4 E4 G4 A4 G4 E4 D4 E4 G4 "
    "D5 D5 B4 D5 E5 D5 B4 A4 G4 A4 B4 A4 G4 - G4 - "
    "B4 D5 E5 D5 B4 A4 G4 A4 B4 B4 A4 G4 E4 G4 A4 - "
    "D4 G4 A4 B4 D5 B4 A4 G4 E4 D4 E4 F#4 G4 - - -"
).split()
STEP_FRAMES = 9


def build_music():
    steps = []
    for name in MELODY:
        if name == "-":
            steps.append(0)
            continue
        octave = int(name[-1])
        steps.append(NOTE_NAMES[name[:-1]] + (octave - 3) * 12 + 1)
    OUT.append(c_array("music_notes", bytes(steps)))
    define("MUSIC_STEPS", len(steps))
    define("MUSIC_STEP_FRAMES", STEP_FRAMES)
    lead = [
        {"frame": i * STEP_FRAMES, "hz": note_hz(n - 1 + 48)} for i, n in enumerate(steps) if n
    ]
    return build_song(lead)


def main() -> None:
    build_farm()
    build_sprites()
    build_font()
    build_hockey()
    build_brawler()
    song = build_music()
    arrays = "".join(OUT)
    (ROOT / "src" / "assets.h").write_text("#include <gb/cgb.h>\n" + arrays, encoding="utf-8")
    externs = c_externs(arrays)
    externs = externs.replace("#endif\n", "".join(DEFINES) + "#endif\n")
    (ROOT / "src" / "art.h").write_text(externs, encoding="utf-8")
    build = ROOT / "build"
    build.mkdir(parents=True, exist_ok=True)
    (build / "music.json").write_text(json.dumps(song, sort_keys=True), encoding="utf-8")


if __name__ == "__main__":
    main()
