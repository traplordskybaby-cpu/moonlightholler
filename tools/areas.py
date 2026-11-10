"""Moonlight Holler's places.  Each builder returns an Area (see worldkit.py)."""
from __future__ import annotations

import json
from pathlib import Path

import art_base as b
from art_base import Canvas, flip, overlay, recolor
from worldkit import (Area, FARM_PALS, HOUSE_PALS, STORE_PALS, VILLAGE_PALS, CREEK_PALS, HS_ARCADE, HS_BED,
                      HS_HOCKEY, HS_SHOP, HS_TALK, R_DAISY, R_GUS, R_PEARL, R_SHOP, R_WENDELL, FLAG_INDOOR)

ROOT = Path(__file__).resolve().parents[1]

AREA_FARM, AREA_TRAILER, AREA_VILLAGE, AREA_STORE, AREA_LAKE = 0, 1, 2, 3, 4
DIR_DOWN, DIR_UP, DIR_LEFT, DIR_RIGHT = 0, 1, 2, 3

# ---- shared tile art ---------------------------------------------------------------------------
ROOFM_TOP = ["11111111", "00100100", "00100100", "00100100", "00100100", "00100100", "00100100", "00100100"]
ROOFM_EAVE = ["00100100", "00100100", "00100100", "00100100", "00100100", "00100100", "11111111", "22222222"]
AC_UNIT = ["00100100", "01111110", "01222210", "01232210", "01222210", "01111110", "00100100", "00100100"]
SIDING = ["00000000", "00000000", "11111111", "00000000", "00000000", "11111111", "00000000", "00000000"]
SKIRT = ["11111111", "20202020", "02020202", "20202020", "02020202", "20202020", "02020202", "22222222"]
TRAILER_WIN = ["33333333", "30000003", "30011003", "30011003", "30000003", "33333333", "00000000", "00000000"]
TRAILER_DOOR_T = ["22222222", "21111112", "21000012", "21000012", "21111112", "21111112", "21111112", "21111112"]
TRAILER_DOOR_B = ["21111112", "21111112", "21111312", "21111112", "21111112", "21111112", "21111112", "21111112"]
PORCH = ["22222222", "11111111", "11111111", "22222222", "11111111", "11111111", "22222222", "00000000"]
SHINGLE = ["11111111", "11111111", "22222222", "11111111", "11111111", "22222222", "11111111", "22222222"]
EAVE = ["11111111", "11111111", "22222222", "11111111", "22222222", "22222222", "33333333", "00000000"]
CLAP = ["00000000", "00000000", "00000000", "11111111", "00000000", "00000000", "00000000", "11111111"]
CLAP_BASE = ["00000000", "00000000", "11111111", "00000000", "11111111", "22222222", "22222222", "22222222"]
WINDOW = ["22222222", "21110112", "21110112", "20000002", "21110112", "21110112", "21110112", "22222222"]
DOOR_TOP = ["22222222", "21111112", "21222212", "21222212", "21111112", "21222212", "21222212", "21111112"]
DOOR_BOT = ["21222212", "21222212", "21111112", "21222312", "21222212", "21222212", "21111112", "22222222"]
STEP = ["00000000", "11111111", "12222221", "12222221", "11111111", "00000000", "00000000", "00000000"]
SIGN_POST = ["00000000", "11111111", "12221221", "12212221", "11111111", "00022000", "00022000", "00022000"]
MAT = ["33333333", "30000003", "30333303", "30333303", "30333303", "30333303", "30000003", "33333333"]
STRIPE = ["00100100", "00100100", "00100100", "00100100", "00100100", "00100100", "00100100", "00100100"]
WAINSCOT = ["22222222", "11111111", "12111211", "11111111", "12111211", "11111111", "12111211", "22222222"]
TRIM = ["22222222", "11111111", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"]
SIDE_L = ["22111111", "22111111", "22112111", "22111111", "22111111", "22111211", "22111111", "22111111"]
SIDE_R = flip(SIDE_L)
BASEBOARD = ["00000000", "00000000", "00000000", "00000000", "00000000", "11111111", "22222222", "22222222"]
CURTAIN_L = ["30000000", "33000000", "33000000", "33000000", "33000000", "33000000", "33000000", "30000000"]
FRAME = ["00000000", "02222220", "02111120", "02131120", "02111120", "02111120", "02222220", "00000000"]
SHELF_BOOKS = ["22222222", "21213121", "21213121", "21213121", "22222222", "21312131", "21312131", "22222222"]


def table_tiles():
    c = Canvas(16, 8, 0)
    c.rect(0, 1, 15, 7, 2)
    c.rect(1, 2, 14, 6, 1)
    c.rect(2, 3, 13, 5, 3)
    return [c.tile(0, 0), c.tile(1, 0)]


def bed_tiles():
    c = Canvas(16, 16, 0)
    c.rect(0, 0, 15, 15, 2)
    c.rect(1, 1, 14, 14, 0)
    c.rect(2, 2, 13, 6, 1)
    c.rect(2, 7, 13, 7, 2)
    for y in range(9, 14, 2):
        c.rect(2, y, 13, y, 3)
    return [[c.tile(x, y) for x in range(2)] for y in range(2)]


# the farm as originally generated: "#" = solid. The trailer is carved out of it below.
FARM_COLLISION = [
    "######################",
    "######################",
    "...#......#..######.#.",
    "..#######...#########.",
    "..#...###.............",
    "..#...###..##########.",
    "..#...................",
    "..#.....#.............",
    "..#######.............",
    ".########.............",
    ".########.............",
    ".########.............",
    ".########.............",
    ".###..................",
    ".#......##............",
    ".##.###...............",
]


def _farm_rows():
    return [list(r) for r in FARM_COLLISION]


# ---- FARM --------------------------------------------------------------------------------------
def build_farm():
    s = Area("farm", 22, 16, FARM_PALS)
    G, G2 = b.GRASS, b.GRASS2
    s.fill(0, 0, 21, 15, G, 0)
    for x, y in [(1, 4), (0, 7), (20, 9), (11, 14), (14, 15), (17, 9), (1, 12)]:
        s.put(x, y, G2, 0)
    s.fill(0, 0, 21, 1, b.SKY, 1)
    for x, y in [(12, 0), (19, 0), (0, 1)]:
        s.put(x, y, b.CLOUD_L, 1)
        s.put(x + 1, y, b.CLOUD_R, 1)
    for i, t in enumerate(b.text_strip("MOONLIGHT", 10, 2, 3)):
        s.put(2 + i, 0, t, 3)
    for i, t in enumerate(b.text_strip("HOLLER", 10, 2, 3)):
        s.put(2 + i, 1, t, 3)
    s.put(3, 2, b.POST, 0)
    s.put(10, 2, b.POST, 0)
    # barn
    s.put(13, 1, b.ROOF_L, 1)
    s.put(18, 1, flip(b.ROOF_L), 1)
    s.fill(14, 1, 17, 1, b.ROOF, 2)
    s.put(15, 1, b.LOFT_L, 2)
    s.put(16, 1, flip(b.LOFT_L), 2)
    top, bottom = b.door_leaf()
    for y in (2, 3):
        s.put(13, y, b.WALL_L, 2)
        s.put(14, y, b.WALL, 2)
        s.put(17, y, b.WALL, 2)
        s.put(18, y, b.WALL_R, 2)
    s.put(15, 2, top, 2)
    s.put(16, 2, top, 2)
    s.put(15, 3, bottom, 2)
    s.put(16, 3, bottom, 2)
    for x, y in [(12, 3), (19, 3), (20, 3), (20, 2)]:
        s.put(x, y, b.HAY_BALE, 3)
    # guinea pig pen
    s.fill(3, 4, 7, 7, b.STRAW, 3)
    s.fill(2, 3, 8, 3, b.FENCE_H, 3)
    s.fill(2, 8, 8, 8, b.FENCE_H, 3)
    s.fill(2, 4, 2, 7, b.FENCE_V, 3)
    s.fill(8, 4, 8, 7, b.FENCE_V, 3)
    s.put(8, 6, b.STRAW, 3)
    s.put(6, 4, b.HUTCH_TL, 3)
    s.put(7, 4, flip(b.HUTCH_TL), 3)
    s.put(6, 5, b.HUTCH_BL, 3)
    s.put(7, 5, flip(b.HUTCH_BL), 3)
    # trails
    s.fill(9, 4, 21, 4, b.PATH, 3)
    s.fill(9, 5, 10, 15, b.PATH, 3)
    for x, y in [(12, 4), (17, 4), (9, 7), (10, 11), (20, 4)]:
        s.put(x, y, b.PATH2, 3)
    s.fill(11, 5, 20, 5, b.FENCE_H, 3)
    # crops
    crops = [(b.CUCUMBER, 5), (b.PEPPER, 4), (b.LETTUCE, 4), (b.CORN, 5), (b.HAY, 5)]
    crop_tiles, crop_attr = [], []
    for art, pal in crops:
        ripe = overlay(b.TILLED, art) if art is not b.HAY else b.HAY
        for rows in (b.TILLED, b.WATERED, overlay(b.TILLED, b.SPROUT), overlay(b.WATERED, b.SPROUT), ripe):
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
    s.crop_tiles, s.crop_attr = bytes(crop_tiles), bytes(crop_attr)
    for x, y in [(19, 10), (21, 10), (20, 11), (19, 12), (21, 12), (20, 13), (19, 14), (21, 14), (0, 3), (1, 9), (0, 12), (1, 15)]:
        s.put(x, y, b.SUNFLOWER, 0)

    # ---- the trailer (cols 2-8, rows 9-13), front door at (5,12) ----
    s.fill(2, 9, 8, 9, ROOFM_TOP, 7)
    s.fill(2, 10, 8, 10, ROOFM_EAVE, 7)
    s.put(7, 9, AC_UNIT, 7)
    s.fill(2, 11, 8, 12, SIDING, 6)
    s.put(3, 11, TRAILER_WIN, 1)
    s.put(4, 11, TRAILER_WIN, 1)
    s.put(7, 11, TRAILER_WIN, 1)
    s.put(8, 11, TRAILER_WIN, 1)
    s.put(5, 11, TRAILER_DOOR_T, 7)
    s.put(5, 12, TRAILER_DOOR_B, 7)
    s.fill(2, 13, 8, 13, SKIRT, 6)
    s.put(5, 13, PORCH, 3)
    s.put(4, 14, b.PATH2, 3)
    s.put(5, 14, b.PATH2, 3)
    s.put(6, 14, b.PATH2, 3)
    s.fill(2, 14, 3, 14, G, 0)
    s.put(2, 14, b.SUNFLOWER, 0)
    s.put(8, 14, b.SUNFLOWER, 0)
    s.put(1, 11, b.SUNFLOWER, 0)
    # trail sign at the fork
    s.put(11, 3, SIGN_POST, 3)

    # ---- hearts / HUD tile constants come from Area ----
    # ---- collision: the original plan with the old arcade nook replaced by the house ----
    rows = _farm_rows()
    solid = [[1 if ch == "#" else 0 for ch in r] for r in rows]
    for y in range(9, 16):
        for x in range(0, 10):
            solid[y][x] = 0
    for y in range(9, 14):
        for x in range(2, 9):
            solid[y][x] = 1
    solid[13][5] = 0  # porch steps
    solid[12][5] = 0  # the door
    solid[3][11] = 1  # sign
    for y in range(16):
        for x in range(22):
            s.solid[y][x] = solid[y][x]

    # ---- links ----
    s.exit(5, 12, 5, 12, AREA_TRAILER, 9, 14, DIR_UP)
    s.exit(21, 4, 21, 4, AREA_VILLAGE, 1, 8, DIR_RIGHT)
    s.exit(9, 15, 10, 15, AREA_LAKE, 9, 1, DIR_DOWN)
    s.hot(11, 3, 11, 3, HS_TALK, "SIGN_FARM")
    return s


# ---- TRAILER INTERIOR ------------------------------------------------------------------------
def build_trailer():
    s = Area("trailer", 20, 16, HOUSE_PALS, FLAG_INDOOR)
    s.fill(0, 0, 19, 15, b.FLOOR, 6)
    # back wall (rows 0-1)
    s.fill(0, 0, 19, 0, STRIPE, 2)
    s.fill(0, 1, 19, 1, WAINSCOT, 0)
    for x in (4, 5):
        s.put(x, 0, WINDOW, 4)
    s.put(3, 0, CURTAIN_L, 3)
    s.put(6, 0, flip(CURTAIN_L), 3)
    s.put(9, 0, FRAME, 2)
    s.put(13, 0, FRAME, 2)
    s.fill(0, 1, 19, 1, WAINSCOT, 0)
    # side walls
    for y in range(2, 16):
        s.put(0, y, SIDE_L, 0)
        s.put(19, y, SIDE_R, 0)
    s.fill(0, 15, 19, 15, BASEBOARD, 2)
    s.fill(0, 15, 19, 15, WAINSCOT, 0)
    s.put(9, 15, MAT, 3)
    s.put(10, 15, MAT, 3)
    s.block(0, 0, 19, 1)
    s.block(0, 2, 0, 15)
    s.block(19, 2, 19, 15)
    s.block(0, 15, 19, 15)
    s.block(9, 15, 10, 15, False)
    # arcade cabinet
    for (x, y, t) in [(2, 2, b.ARC_TL), (3, 2, b.ARC_TR), (2, 3, b.ARC_ML), (3, 3, b.ARC_MR), (2, 4, b.ARC_BL), (3, 4, b.ARC_BR)]:
        s.put(x, y, t, 4, True)
    s.hot(2, 2, 3, 4, HS_ARCADE)
    # air hockey table
    tbl = b.hockey_table()
    for y in range(2):
        for x in range(4):
            s.put(6 + x, 7 + y, tbl[y][x], 4, True)
    s.hot(6, 7, 9, 8, HS_HOCKEY)
    # rug and sofa
    s.fill(11, 8, 15, 10, b.RUG, 6)
    s.put(11, 12, b.SOFA_L, 6, True)
    s.put(12, 12, b.SOFA_M, 6, True)
    s.put(13, 12, flip(b.SOFA_L), 6, True)
    # kitchen corner
    s.put(16, 2, b.STOVE_T, 4, True)
    s.put(16, 3, b.STOVE_B, 4, True)
    tl, tr = table_tiles()
    s.put(13, 4, tl, 0, True)
    s.put(14, 4, tr, 0, True)
    # bed
    bt = bed_tiles()
    for y in range(2):
        for x in range(2):
            s.put(17 + x - 1, 11 + y, bt[y][x], 3, True)
    s.hot(16, 11, 17, 12, HS_BED)
    # plants and pictures
    s.put(1, 13, b.PLANT, 6, True)
    s.put(18, 3, b.PLANT, 6, True)
    s.put(1, 7, b.PLANT, 6, True)
    # bookshelf with books by the door side
    s.put(7, 2, SHELF_BOOKS, 0, True)
    s.put(8, 2, SHELF_BOOKS, 0, True)
    s.hot(7, 2, 8, 2, HS_TALK, "BOOKS")
    # people
    s.npc(12, 6, 0, R_PEARL, 0, (11, 5, 15, 7))
    # links
    s.exit(9, 15, 10, 15, AREA_FARM, 5, 14, DIR_DOWN)
    return s


# ---- painters ----------------------------------------------------------------------------------
def ellipse(c, cx, cy, rx, ry, col):
    for y in range(int(cy - ry - 1), int(cy + ry + 2)):
        for x in range(int(cx - rx - 1), int(cx + rx + 2)):
            if ((x + 0.5 - cx) / rx) ** 2 + ((y + 0.5 - cy) / ry) ** 2 <= 1.0:
                c.set(x, y, col)


def tree_canvas(seed=0):
    """16x16 round tree on a grass background; palette: 0 grass, 1 leaves, 2 shadow, 3 trunk."""
    c = Canvas(16, 16, 0)
    c.disc(8, 7.6, 7.5, 2)
    c.disc(7.4, 6.8, 6.3, 1)
    c.disc(5.2, 4.6, 2.4, 0)
    c.disc(10.5, 9.5, 1.2, 2)
    c.disc(4.5, 9.5, 1.0, 2)
    c.rect(7, 13, 8, 15, 3)
    c.set(6, 15, 3)
    c.set(9, 15, 3)
    for (x, y) in ([(9, 3), (11, 6), (6, 8)] if seed % 2 == 0 else [(8, 4), (5, 7), (10, 8)]):
        c.set(x, y, 0)
    return c


def bush_canvas():
    c = Canvas(8, 8, 0)
    c.disc(4, 4.8, 3.7, 2)
    c.disc(3.6, 4.2, 2.8, 1)
    c.set(2, 3, 0)
    c.set(3, 3, 0)
    return c


def barrel_canvas():
    c = Canvas(8, 8, 0)
    ellipse(c, 4, 4, 3.6, 3.9, 1)
    c.rect(1, 2, 6, 2, 2)
    c.rect(1, 5, 6, 5, 2)
    c.rect(3, 1, 3, 6, 3)
    return c


CRATE = ["22222222", "21111112", "21211212", "21121112", "21121112", "21211212", "21111112", "22222222"]
COBBLE_A = ["11111111", "00010000", "00010020", "11111111", "00000100", "02000100", "00000100", "11111111"]
COBBLE_B = ["11111111", "00000100", "00000100", "11111111", "00010000", "00010020", "00010000", "11111111"]
RED_ROOF = ["00000000", "00000000", "22222222", "00000000", "00000000", "22222222", "00000000", "11111111"]
RED_EAVE = ["00000000", "22222222", "00000000", "00000000", "22222222", "22222222", "33333333", "00000000"]
AWNING = ["00110011", "00110011", "00110011", "00110011", "00110011", "22222222", "33333333", "00000000"]
BENCH = ["00000000", "00000000", "11111111", "12222221", "11111111", "10000001", "10000001", "00000000"]
BOARD = ["22222222", "21111112", "21301312", "21111112", "21013012", "21111112", "21301312", "22222222"]
LOCKED_NOTE = ["00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000", "00000000"]
PLANK_H = ["11111111", "12222221", "11111111", "11111111", "12222221", "11111111", "11111111", "12222221"]
BRIDGE = ["22222222", "11111111", "12222221", "11111111", "22222222", "11111111", "12222221", "11111111"]
CABIN_ROOF = ["11111111", "12121212", "22222222", "11111111", "12121212", "22222222", "11111111", "22222222"]
CABIN_LOG = ["12121212", "11111111", "22222222", "21212121", "11111111", "22222222", "12121212", "11111111"]


def house_front(s, x0, y0, width, door_x, roof_rows, roof_pal, wall_pal, win_pal, windows, sign=None):
    """A storefront/cottage: two roof rows, three wall rows, door at door_x (bottom row y0+4)."""
    x1 = x0 + width - 1
    if roof_pal == 3:
        s.fill(x0, y0, x1, y0, SHINGLE, 3, True)
        s.fill(x0, y0 + 1, x1, y0 + 1, EAVE, 3, True)
    else:
        s.fill(x0, y0, x1, y0, RED_ROOF, roof_pal, True)
        s.fill(x0, y0 + 1, x1, y0 + 1, RED_EAVE, roof_pal, True)
    s.fill(x0, y0 + 2, x1, y0 + 3, CLAP, wall_pal, True)
    s.fill(x0, y0 + 4, x1, y0 + 4, CLAP_BASE, wall_pal, True)
    for wx in windows:
        s.put(wx, y0 + 3, WINDOW, win_pal, True)
    s.put(door_x, y0 + 3, DOOR_TOP, 3, True)
    s.put(door_x, y0 + 4, DOOR_BOT, 3, False)


def scatter(rng, n, x0, y0, x1, y1, avoid):
    out = []
    tries = 0
    while len(out) < n and tries < 500:
        tries += 1
        x, y = rng.randint(x0, x1), rng.randint(y0, y1)
        if (x, y) not in avoid and (x, y) not in out:
            out.append((x, y))
    return out


# ---- VILLAGE -----------------------------------------------------------------------------------
def build_village():
    import random

    rng = random.Random(11)
    s = Area("village", 32, 16, VILLAGE_PALS)
    G, G2 = b.GRASS, b.GRASS2
    s.fill(0, 0, 31, 15, G, 0, False)
    for (x, y) in scatter(rng, 26, 0, 2, 31, 15, set()):
        s.put(x, y, G2, 0, False)
    # trail from the farm, cobbled plaza
    s.fill(0, 8, 31, 9, b.PATH, 3, False)
    for (x, y) in [(2, 9), (5, 8), (9, 9), (24, 8), (28, 9), (30, 8)]:
        s.put(x, y, b.PATH2, 3, False)
    for y in range(7, 12):
        for x in range(11, 21):
            s.put(x, y, COBBLE_A if (x + y) % 2 == 0 else COBBLE_B, 4, False)
    # north tree line
    trees = [(0, 0), (3, 0), (6, 0), (9, 0), (12, 0), (15, 0), (18, 0), (21, 0), (24, 0), (27, 0), (29, 0)]
    for k, (x, y) in enumerate(trees):
        s.paint(tree_canvas(k), x, y, 2, True)
    for x in (1, 5, 8, 11, 14, 17, 20, 23, 26, 31):
        s.put(x, 2, b.SUNFLOWER if x % 2 else G2, 0, False)
    # general store: x 4..10, door at 7
    house_front(s, 4, 3, 7, 7, 2, 5, 6, 7, (5, 9))
    s.text_tiles(5, 5, "STORE", 5, 7, 2, 0)
    s.put(3, 7, CRATE, 3, True)
    s.paint(barrel_canvas(), 11, 7, 3, True)
    s.exit(7, 7, 7, 7, AREA_STORE, 9, 14, DIR_UP)
    # cottage: x 18..24, door at 21 (locked)
    house_front(s, 18, 3, 7, 21, 2, 3, 6, 7, (19, 23))
    s.hot(21, 7, 21, 7, HS_TALK, "LOCKED")
    s.put(17, 7, b.SUNFLOWER, 0, False)
    s.put(25, 7, b.SUNFLOWER, 0, False)
    # notice board and well
    s.put(13, 6, SIGN_POST, 3, True)
    s.hot(13, 6, 13, 6, HS_TALK, "BOARD")
    well = Canvas(16, 16, 0)
    well.disc(8, 8, 7.6, 2)
    well.disc(8, 8, 6.2, 1)
    well.disc(8, 8, 4.0, 3)
    well.disc(8, 8, 3.0, 2)
    well.rect(7, 1, 8, 4, 3)
    s.paint(well, 15, 10, 4, True)
    s.hot(15, 10, 16, 11, HS_TALK, "WELL")
    # south side: trees, bushes, benches, flowers
    for k, (x, y) in enumerate([(1, 11), (4, 13), (8, 12), (23, 11), (27, 13), (29, 11)]):
        s.paint(tree_canvas(k + 1), x, y, 2, True)
    for (x, y) in [(6, 11), (11, 14), (21, 13), (25, 12), (13, 13), (18, 14), (3, 10), (30, 14)]:
        s.paint(bush_canvas(), x, y, 2, True)
    for (x, y) in [(12, 12), (19, 12), (9, 14), (22, 15), (15, 14)]:
        s.put(x, y, b.SUNFLOWER, 0, False)
    s.put(18, 11, BENCH, 3, True)
    s.put(19, 11, BENCH, 3, True)
    # east end: road closed
    s.fill(27, 8, 27, 9, b.FENCE_V, 3, True)
    s.put(26, 7, SIGN_POST, 3, True)
    s.hot(26, 7, 26, 7, HS_TALK, "CLOSED")
    for k, y in enumerate((2, 4, 6, 10, 12, 14)):
        s.paint(tree_canvas(k), 29 if y < 8 else 29, y, 2, True)
    s.paint(tree_canvas(0), 29, 6, 2, True)
    s.block(28, 8, 31, 9, True)
    # borders
    s.block(0, 0, 31, 1)
    s.block(0, 15, 31, 15)
    # links
    s.exit(0, 8, 0, 9, AREA_FARM, 20, 4, DIR_LEFT)
    s.npc(23, 9, 2, R_DAISY, 0, (21, 8, 25, 9))
    s.npc(13, 9, 3, R_WENDELL, 0, (11, 8, 17, 9))
    return s


# ---- GENERAL STORE -----------------------------------------------------------------------------
def build_store():
    s = Area("store", 20, 16, STORE_PALS, FLAG_INDOOR)
    s.fill(0, 0, 19, 15, b.FLOOR, 6)
    s.fill(0, 0, 19, 0, STRIPE, 2)
    s.fill(0, 1, 19, 1, WAINSCOT, 0)
    for y in range(2, 16):
        s.put(0, y, SIDE_L, 0)
        s.put(19, y, SIDE_R, 0)
    s.fill(0, 15, 19, 15, WAINSCOT, 0)
    s.put(9, 15, MAT, 3)
    s.put(10, 15, MAT, 3)
    s.block(0, 0, 19, 1)
    s.block(0, 2, 0, 15)
    s.block(19, 2, 19, 15)
    s.block(0, 15, 19, 15)
    s.block(9, 15, 10, 15, False)
    # shelves on the back wall
    SH_A = ["22222222", "20302032", "20302032", "22222222", "20302032", "20302032", "20302032", "22222222"]
    SH_B = ["22222222", "23102313", "23102313", "22222222", "21302131", "21302131", "21302131", "22222222"]
    for x in range(2, 18):
        if x in (9, 10):
            s.put(x, 0, FRAME, 2, True)
            continue
        s.put(x, 0, SH_A if x % 2 == 0 else SH_B, 4 if x % 2 == 0 else 5, True)
    s.put(9, 0, FRAME, 2, True)
    s.put(10, 0, FRAME, 2, True)
    # counter
    CT_L = ["00000000", "11111111", "12222221", "12222221", "11111111", "22222222", "22222222", "22222222"]
    CT_M = ["00000000", "11111111", "22222222", "22222222", "11111111", "22222222", "21212121", "22222222"]
    s.put(5, 5, CT_L, 6, True)
    for x in range(6, 14):
        s.put(x, 5, CT_M, 6, True)
    s.put(14, 5, CT_L, 6, True)
    s.hot(5, 5, 14, 5, HS_SHOP)
    s.npc(9, 4, 1, R_SHOP, 0, (9, 4, 9, 4))
    # stock around the room
    s.paint(barrel_canvas(), 1, 2, 0, True)
    s.paint(barrel_canvas(), 1, 3, 0, True)
    s.put(2, 3, CRATE, 0, True)
    s.put(18, 2, CRATE, 0, True)
    s.put(18, 3, CRATE, 0, True)
    s.paint(barrel_canvas(), 17, 2, 0, True) if False else None
    s.put(1, 13, b.PLANT, 6, True)
    s.put(18, 13, b.PLANT, 6, True)
    s.fill(7, 9, 12, 11, b.RUG, 6, False)
    s.put(3, 8, CRATE, 0, True)
    s.put(4, 8, CRATE, 0, True)
    s.put(3, 9, CRATE, 0, True)
    s.put(15, 8, b.HAY_BALE, 3, True)
    s.put(16, 8, b.HAY_BALE, 3, True)
    s.put(15, 9, b.HAY_BALE, 3, True)
    s.exit(9, 15, 10, 15, AREA_VILLAGE, 7, 8, DIR_DOWN)
    return s


# ---- CREEK -------------------------------------------------------------------------------------
def build_lake():
    import random

    rng = random.Random(5)
    s = Area("lake", 22, 24, CREEK_PALS)
    G, G2 = b.GRASS, b.GRASS2
    s.fill(0, 0, 21, 23, G, 0, False)
    for (x, y) in scatter(rng, 30, 0, 1, 21, 22, set()):
        s.put(x, y, G2, 0, False)
    # trail down from the farm
    s.fill(9, 0, 10, 10, b.PATH, 3, False)
    s.fill(9, 14, 10, 19, b.PATH, 3, False)
    s.fill(5, 16, 10, 16, b.PATH, 3, False)
    for (x, y) in [(9, 3), (10, 7), (9, 17), (7, 16)]:
        s.put(x, y, b.PATH2, 3, False)
    # the creek: rows 11-13 across the whole map
    c = Canvas(176, 24, 0)
    c.rect(0, 3, 175, 20, 1)
    c.rect(0, 2, 175, 2, 2)
    c.rect(0, 21, 175, 21, 2)
    for x in range(0, 176, 7):
        c.set(x + (x // 7 % 3), 7 + (x // 7) % 5, 3)
        c.set(x + 3, 15 + (x // 7) % 4, 3)
    for x in range(176):
        if (x * 7) % 11 == 0:
            c.set(x, 9, 2)
            c.set(x, 17, 2)
    s.paint(c, 0, 11, 5, True)
    for y in (11, 12, 13):
        s.fill(9, y, 10, y, BRIDGE, 3, False)
    # pond with a dock
    pond = Canvas(80, 40, 0)
    ellipse(pond, 40, 20, 39, 19.5, 2)
    ellipse(pond, 40, 20, 37, 17.5, 1)
    for (x, y) in [(22, 14), (28, 26), (50, 12), (58, 22), (36, 30), (46, 28), (30, 18)]:
        pond.set(x, y, 3)
        pond.set(x + 1, y, 3)
    for (x, y) in [(18, 22), (62, 16), (40, 8), (44, 33)]:
        pond.set(x, y, 2)
        pond.set(x + 1, y, 2)
    s.paint(pond, 11, 16, 5, True)
    s.fill(11, 19, 12, 19, PLANK_H, 3, False)
    s.fill(10, 19, 10, 19, PLANK_H, 3, False)
    # cabin
    s.fill(2, 17, 7, 17, CABIN_ROOF, 3, True)
    s.fill(2, 18, 7, 18, CABIN_ROOF, 3, True)
    s.fill(2, 19, 7, 20, CABIN_LOG, 6, True)
    s.put(3, 20, WINDOW, 7, True)
    s.put(6, 20, WINDOW, 7, True)
    s.put(4, 19, DOOR_TOP, 3, True)
    s.put(4, 20, DOOR_BOT, 3, True)
    s.hot(4, 20, 4, 20, HS_TALK, "CABIN")
    # trees, rocks, bushes, flowers
    for k, (x, y) in enumerate([(0, 0), (2, 0), (5, 0), (13, 0), (16, 0), (19, 0), (0, 3), (19, 3), (1, 6), (18, 6), (3, 9), (16, 9),
                                (0, 21), (9, 21), (12, 21), (15, 21), (18, 21), (20, 21), (1, 14), (7, 13), (20, 17), (0, 17)]):
        if (x, y) in ((1, 14), (7, 13), (20, 17), (0, 17)) and False:
            continue
        if 11 <= y <= 13 and True:
            continue
        s.paint(tree_canvas(k), x, y, 2, True)
    for (x, y) in [(4, 4), (14, 5), (6, 8), (13, 8), (5, 14), (14, 14), (17, 15), (2, 15), (20, 11)]:
        if 11 <= y <= 13:
            continue
        s.paint(bush_canvas(), x, y, 2, True)
    rock = Canvas(8, 8, 0)
    ellipse(rock, 4, 4.8, 3.6, 3.0, 2)
    ellipse(rock, 3.7, 4.2, 2.8, 2.2, 1)
    rock.set(2, 3, 3)
    rock.set(3, 3, 3)
    for (x, y) in [(7, 5), (12, 4), (4, 7), (15, 15), (7, 22)]:
        if 11 <= y <= 13:
            continue
        s.paint(rock, x, y, 4, True)
    for (x, y) in [(3, 5), (12, 6), (15, 7), (6, 6), (12, 15), (8, 20), (2, 11 + 5)]:
        s.put(x, y, b.SUNFLOWER, 0, False)
    s.put(11, 2, SIGN_POST, 3, True)
    s.hot(11, 2, 11, 2, HS_TALK, "SIGN_LAKE")
    s.put(8, 15, SIGN_POST, 3, True)
    s.hot(8, 15, 8, 15, HS_TALK, "POND")
    # borders
    s.block(0, 0, 21, 0)
    s.block(0, 23, 21, 23)
    s.block(0, 0, 0, 23)
    s.block(21, 0, 21, 23)
    s.block(9, 0, 10, 0, False)
    s.exit(9, 0, 10, 0, AREA_FARM, 10, 14, DIR_UP)
    s.npc(10, 18, 4, R_GUS, 0, (8, 17, 10, 19))
    return s


ALL_AREAS = [build_farm, build_trailer, build_village, build_store, build_lake]
