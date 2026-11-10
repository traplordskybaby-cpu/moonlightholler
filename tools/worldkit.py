"""Area-building toolkit for Moonlight Holler.

An Area is a tile map with its own tile set (max 128 tiles), eight background palettes, solid flags,
exits (doors / trail ends), hotspots (signs, machines, bed) and NPC spots.  It serialises into one
binary blob that the game copies out of a ROM bank when the area is entered.
"""
from __future__ import annotations

import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import art_base as base
from art_base import Canvas, night, overlay, flip, recolor
from gbasset import rgb15, tile_2bpp

BKG = 128

# hotspot kinds (keep in step with src/world.h)
HS_TALK, HS_ARCADE, HS_HOCKEY, HS_BED, HS_SHOP = 1, 2, 3, 4, 5
# npc roles
R_PEARL, R_SHOP, R_DAISY, R_WENDELL, R_GUS = 1, 2, 3, 4, 5
FLAG_INDOOR = 1

# ---- palettes ----------------------------------------------------------------------------------
HUD = [(18, 26, 31), (31, 31, 31), (28, 5, 4), (3, 5, 14)]  # slot 1 in every area: HUD + dialogue


def pal(*rows):
    return [list(r) for r in rows]


FARM_PALS = [list(map(tuple, p)) for p in base.FARM_PALS]
FARM_PALS[6] = [(27, 29, 30), (19, 23, 27), (10, 13, 20), (31, 25, 6)]  # trailer siding
FARM_PALS[7] = [(24, 24, 26), (15, 15, 19), (8, 8, 12), (28, 5, 4)]  # trailer roof / door

VILLAGE_PALS = [
    [(16, 27, 7), (9, 20, 4), (9, 5, 2), (31, 27, 5)],  # 0 grass
    HUD,  # 1
    [(16, 27, 7), (6, 18, 5), (3, 10, 3), (12, 7, 2)],  # 2 trees on grass
    [(26, 20, 11), (17, 10, 4), (8, 4, 1), (31, 25, 6)],  # 3 dirt / wood
    [(23, 23, 23), (15, 15, 17), (8, 8, 11), (29, 29, 29)],  # 4 cobble
    [(28, 5, 4), (31, 31, 29), (13, 2, 2), (6, 6, 8)],  # 5 red roof / awning
    [(30, 28, 20), (23, 14, 6), (11, 6, 2), (6, 18, 6)],  # 6 cream wall / planks
    [(31, 31, 31), (8, 20, 31), (2, 3, 12), (31, 4, 4)],  # 7 windows / signs
]

CREEK_PALS = [
    [(16, 27, 7), (9, 20, 4), (9, 5, 2), (31, 27, 5)],  # 0 grass
    HUD,  # 1
    [(16, 27, 7), (6, 18, 5), (3, 10, 3), (12, 7, 2)],  # 2 trees on grass
    [(26, 20, 11), (17, 10, 4), (8, 4, 1), (31, 25, 6)],  # 3 dirt / wood
    [(16, 27, 7), (22, 22, 24), (11, 11, 15), (30, 30, 30)],  # 4 rocks on grass
    [(16, 27, 7), (6, 16, 30), (2, 8, 22), (28, 31, 31)],  # 5 water / shore
    [(30, 28, 20), (23, 14, 6), (11, 6, 2), (6, 18, 6)],  # 6 cabin wall
    [(31, 31, 31), (8, 20, 31), (2, 3, 12), (31, 4, 4)],  # 7 signs
]

HOUSE_PALS = [
    [(26, 20, 11), (17, 10, 4), (8, 4, 1), (31, 25, 6)],  # 0 wood floor
    HUD,  # 1
    [(29, 25, 18), (24, 18, 12), (14, 8, 5), (22, 8, 8)],  # 2 wallpaper
    [(28, 5, 4), (31, 31, 29), (13, 2, 2), (6, 6, 8)],  # 3 red sofa
    [(31, 31, 31), (8, 20, 31), (2, 3, 12), (31, 4, 4)],  # 4 arcade / hockey blue
    [(26, 20, 11), (7, 22, 5), (31, 27, 5), (9, 5, 2)],  # 5 plant / green
    [(30, 28, 20), (23, 14, 6), (11, 6, 2), (6, 18, 6)],  # 6 rug / bed wood
    [(22, 22, 22), (14, 14, 16), (8, 8, 10), (28, 28, 28)],  # 7 stone / counter
]

STORE_PALS = [
    [(26, 20, 11), (17, 10, 4), (8, 4, 1), (31, 25, 6)],  # 0 wood floor
    HUD,  # 1
    [(29, 25, 18), (24, 18, 12), (14, 8, 5), (22, 8, 8)],  # 2 wallpaper
    [(28, 5, 4), (31, 31, 29), (13, 2, 2), (6, 6, 8)],  # 3 red / awning
    [(31, 31, 31), (8, 20, 31), (2, 3, 12), (31, 4, 4)],  # 4 blue tins
    [(26, 20, 11), (7, 22, 5), (31, 27, 5), (9, 5, 2)],  # 5 green goods
    [(30, 28, 20), (23, 14, 6), (11, 6, 2), (6, 18, 6)],  # 6 counter wood
    [(22, 22, 22), (14, 14, 16), (8, 8, 10), (28, 28, 28)],  # 7 stone
]


# ---- the Area ----------------------------------------------------------------------------------
class Area(base.Scene):
    def __init__(self, name, w, h, pals, flags=0):
        super().__init__(name, w, h)
        self.pals = pals
        self.flags = flags
        self.solid = [[0] * w for _ in range(h)]
        self.exits, self.hots, self.npcs = [], [], []
        # common tiles: indices fixed so the C code can name them
        self.T_HUD = self.tile(base.SKY)
        self.T_HEART = self.tile(base.HEART)
        self.T_HEART_EMPTY = self.tile(base.HEART_EMPTY)
        self.T_BOX = self.tile(["33333333", "33333333", "11111111", "00000000", "00000000", "00000000", "00000000", "00000000"])
        assert (self.T_HUD, self.T_HEART, self.T_HEART_EMPTY, self.T_BOX) == (128, 129, 130, 131)

    # -- placing
    def put(self, x, y, rows, pal, solid=None):
        if not (0 <= x < self.w and 0 <= y < self.h):
            raise ValueError(f"{self.name}: put out of range {x},{y}")
        super().put(x, y, rows, pal)
        if solid is not None:
            self.solid[y][x] = 1 if solid else 0

    def fill(self, x0, y0, x1, y1, rows, pal, solid=None):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.put(x, y, rows, pal, solid)

    def block(self, x0, y0, x1, y1, solid=True):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.solid[y][x] = 1 if solid else 0

    def paint(self, canvas, tx, ty, pal, solid=None, skip_blank=False):
        """Paste a pixel canvas whose size is a multiple of 8 at tile (tx, ty)."""
        for cy in range(canvas.h // 8):
            for cx in range(canvas.w // 8):
                self.put(tx + cx, ty + cy, canvas.tile(cx, cy), pal, solid)

    def paint_over(self, canvas, tx, ty, pal, solid=None):
        """Like paint, but pixels equal to 0 keep what is already in the cell (same palette only)."""
        for cy in range(canvas.h // 8):
            for cx in range(canvas.w // 8):
                x, y = tx + cx, ty + cy
                old = self._rows_of(self.map[y][x])
                top = canvas.tile(cx, cy)
                self.put(x, y, overlay(old, top) if old else top, pal, solid)

    def _rows_of(self, tile_number):
        data = None
        for k, v in self.index.items():
            if BKG + v == tile_number:
                data = k
                break
        if data is None:
            return None
        rows = []
        for y in range(8):
            lo, hi = data[y * 2], data[y * 2 + 1]
            rows.append("".join(str(((hi >> (7 - x)) & 1) << 1 | ((lo >> (7 - x)) & 1)) for x in range(8)))
        return rows

    def text_tiles(self, x, y, text, width, pal, bg, fg):
        for i, t in enumerate(base.text_strip(text, width, bg, fg)):
            self.put(x + i, y, t, pal)

    # -- gameplay data
    def exit(self, x0, y0, x1, y1, to_area, to_x, to_y, to_dir=0):
        self.exits.append((x0, y0, x1, y1, to_area, to_x, to_y, to_dir))

    def hot(self, x0, y0, x1, y1, kind, arg=0):
        self.hots.append((x0, y0, x1, y1, kind, arg))

    def npc(self, tx, ty, look, role, arg, rect):
        self.npcs.append((tx, ty, look, role, arg, *rect))

    def solid_bits(self):
        sb = (self.w + 7) // 8
        out = bytearray()
        for y in range(self.h):
            for b in range(sb):
                out.append(sum(0x80 >> i for i in range(8) if b * 8 + i < self.w and self.solid[y][b * 8 + i]))
        return bytes(out)

    def blob(self, dialogs=(), music=0):
        if len(self.tiles) > 128:
            raise ValueError(f"{self.name}: {len(self.tiles)} tiles (max 128)")
        w, h = self.w, self.h
        tiles_b = b"".join(self.tiles)
        pal_b = b"".join(struct.pack("<4H", *[rgb15(*c) for c in p]) for p in self.pals)
        night_b = b"".join(struct.pack("<4H", *[rgb15(*c) for c in p]) for p in night(self.pals))
        # keep the HUD palette readable at night too
        night_b = bytearray(night_b)
        night_b[8:16] = struct.pack("<4H", *[rgb15(*c) for c in HUD])
        night_b = bytes(night_b)
        sections = [
            ("map", bytes(v for row in self.map for v in row)),
            ("attr", bytes(v for row in self.attr for v in row)),
            ("solid", self.solid_bits()),
            ("pal", pal_b + night_b),
            ("tiles", tiles_b),
            ("exits", b"".join(struct.pack("<8B", *e) for e in self.exits)),
            ("hots", b"".join(struct.pack("<6B", *e[:5], list(dialogs).index(e[5]) if isinstance(e[5], str) else e[5]) for e in self.hots)),
            ("npcs", b"".join(struct.pack("<9B", *e) for e in self.npcs)),
        ]
        head_len = 8 + 2 * len(sections)
        offs, pos = [], head_len
        for _, data in sections:
            offs.append(pos)
            pos += len(data)
        head = struct.pack("<8B", w, h, self.flags, len(self.exits), len(self.hots), len(self.npcs), len(self.tiles), music)
        head += b"".join(struct.pack("<H", o) for o in offs)
        return head + b"".join(d for _, d in sections)


# ---- preview -----------------------------------------------------------------------------------
def render(area, night_pal=False, scale=3, grid=False, show_solid=False):
    from PIL import Image, ImageDraw

    pals = night(area.pals) if night_pal else area.pals
    im = Image.new("RGB", (area.w * 8, area.h * 8))
    px = im.load()
    tile_px = []
    for t in area.tiles:
        rows = []
        for y in range(8):
            lo, hi = t[y * 2], t[y * 2 + 1]
            rows.append([((hi >> (7 - x)) & 1) << 1 | ((lo >> (7 - x)) & 1) for x in range(8)])
        tile_px.append(rows)
    for y in range(area.h):
        for x in range(area.w):
            n = area.map[y][x] - BKG
            p = pals[area.attr[y][x] & 7]
            for yy in range(8):
                for xx in range(8):
                    c = p[tile_px[n][yy][xx]]
                    px[x * 8 + xx, y * 8 + yy] = (c[0] * 255 // 31, c[1] * 255 // 31, c[2] * 255 // 31)
    im = im.resize((area.w * 8 * scale, area.h * 8 * scale), Image.NEAREST).convert("RGBA")
    ov = Image.new("RGBA", im.size, (0, 0, 0, 0))
    d = ImageDraw.Draw(ov, "RGBA")
    if show_solid:
        for y in range(area.h):
            for x in range(area.w):
                if area.solid[y][x]:
                    d.rectangle([x * 8 * scale, y * 8 * scale, (x + 1) * 8 * scale - 1, (y + 1) * 8 * scale - 1], fill=(255, 0, 0, 70))
    for (x0, y0, x1, y1, *_r) in area.exits:
        d.rectangle([x0 * 8 * scale, y0 * 8 * scale, (x1 + 1) * 8 * scale - 1, (y1 + 1) * 8 * scale - 1], outline=(0, 255, 0, 255), width=2)
    for (x0, y0, x1, y1, *_r) in area.hots:
        d.rectangle([x0 * 8 * scale, y0 * 8 * scale, (x1 + 1) * 8 * scale - 1, (y1 + 1) * 8 * scale - 1], outline=(255, 255, 0, 255), width=1)
    for n in area.npcs:
        d.ellipse([n[0] * 8 * scale, n[1] * 8 * scale, n[0] * 8 * scale + 8 * scale, n[1] * 8 * scale + 8 * scale], outline=(255, 0, 255, 255), width=2)
    if grid:
        for x in range(area.w):
            d.line([x * 8 * scale, 0, x * 8 * scale, im.height], fill=(255, 255, 255, 40))
        for y in range(area.h):
            d.line([0, y * 8 * scale, im.width, y * 8 * scale], fill=(255, 255, 255, 40))
    return Image.alpha_composite(im, ov).convert("RGB")
