# Built with love and passion on retroable.dev

"""Tile and music helpers the game's make_assets.py may import. The agent cannot edit this file."""

from __future__ import annotations


def tile_2bpp(rows: list[str]) -> bytes:
    """Eight strings of eight digits (0-3) become one 16-byte Game Boy tile."""
    if len(rows) != 8 or any(len(row) != 8 for row in rows):
        raise ValueError("a tile is 8 rows of 8 pixels")
    out = bytearray()
    for row in rows:
        low = high = 0
        for ch in row:
            if ch not in "0123":
                raise ValueError("pixels are 0, 1, 2 or 3")
            shade = int(ch)
            low = (low << 1) | (shade & 1)
            high = (high << 1) | ((shade >> 1) & 1)
        out.append(low)
        out.append(high)
    return bytes(out)


def c_array(name: str, data: bytes) -> str:
    if not name.isidentifier():
        raise ValueError("array name must be a C identifier")
    body = ", ".join(f"0x{byte:02X}" for byte in data)
    return f"const unsigned char {name}[] = {{{body}}};\n"


def note_hz(midi: int) -> float:
    """Equal temperament, A4 = 440 Hz."""
    return 440.0 * (2.0 ** ((midi - 69) / 12.0))


def build_song(lead: list[dict[str, float | int]]) -> dict[str, object]:
    notes = [{"frame": int(item["frame"]), "hz": float(item["hz"])} for item in lead]
    return {"sample_rate": 48000, "lead": notes}


def rgb15(red: int, green: int, blue: int) -> int:
    """One Game Boy Color color. Each channel is 0 to 31."""
    if not all(isinstance(channel, int) and 0 <= channel <= 31 for channel in (red, green, blue)):
        raise ValueError("a Color channel is 0 to 31")
    return red | (green << 5) | (blue << 10)


def c_palette(name: str, colors: list[tuple[int, int, int]]) -> str:
    """Four colors become a palette_color_t array for set_bkg_palette or set_sprite_palette."""
    if len(colors) != 4 or not name.isidentifier():
        raise ValueError("a palette is 4 colors and a C name")
    body = ", ".join(f"0x{rgb15(*color):04X}" for color in colors)
    return f"const palette_color_t {name}[] = {{{body}}};\n"


def c_externs(arrays: str) -> str:
    """A header declaring every array in c_array output, so several C files can share src/assets.h."""
    lines = []
    for line in arrays.splitlines():
        if line.startswith("const ") and "[] = {" in line:
            kind, name = line[len("const ") :].split("[", 1)[0].rsplit(None, 1)
            lines.append(f"extern const {kind} {name}[];\n")
    color = "#include <gb/cgb.h>\n" if any("palette_color_t" in line for line in lines) else ""
    return f"#ifndef FORGE_ART_H\n#define FORGE_ART_H\n{color}{''.join(lines)}#endif\n"
