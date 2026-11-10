#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>

#include "art.h"
#include "shared/style.h"

static const palette_color_t bkg_palettes[] = {
    RGB(28, 31, 26), RGB(17, 24, 14), RGB(6, 13, 10), RGB(1, 3, 4),
    RGB(8, 16, 31), RGB(4, 8, 24), RGB(2, 4, 16), RGB(0, 0, 6)
};

static const palette_color_t spr_palettes[] = {
    RGB(31, 28, 16), RGB(31, 8, 4), RGB(20, 4, 2), RGB(0, 0, 0),
    RGB(16, 31, 28), RGB(4, 20, 18), RGB(2, 10, 12), RGB(0, 0, 0)
};

void style_boot(void) {
    BGP_REG = 0xE4;
    OBP0_REG = 0xE4;
    OBP1_REG = 0xD0;
    set_bkg_palette(0, 2, bkg_palettes);
    set_sprite_palette(0, 2, spr_palettes);
}

void style_paint(void) {
    uint8_t row[20];
    uint8_t line;
    uint8_t column;
    VBK_REG = 1;
    for (line = 0; line < 18; line++) {
        for (column = 0; column < 20; column++) {
            row[column] = (uint8_t)(line >= 9);
        }
        set_bkg_tiles(0, line, 20, 1, row);
    }
    set_sprite_prop(0, 1);
    VBK_REG = 0;
}

void style_fill(uint8_t tile) {
    uint8_t row[20];
    uint8_t line;
    uint8_t column;
    for (column = 0; column < 20; column++) {
        row[column] = tile;
    }
    for (line = 0; line < 18; line++) {
        set_bkg_tiles(0, line, 20, 1, row);
    }
}

void style_scene(const uint8_t *tiles, uint8_t count) {
    set_bkg_data(128, count, tiles);
    style_common();
}

/* font (VRAM bank 1), sprite art, and a cleared sprite table: what every scene needs */
void style_common(void) {
    uint8_t i;
    VBK_REG = 1;
    set_bkg_data(128, FONT_TILE_COUNT, font_tiles);
    VBK_REG = 0;
    set_sprite_data(0, SPR_TILE_COUNT, spr_tiles);
    for (i = 0; i < 40; i++) {
        shadow_OAM[i].y = 0;
        shadow_OAM[i].prop = 0;
    }
}

void style_map(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *map, const uint8_t *attr) {
    VBK_REG = 1;
    set_bkg_tiles(x, y, w, h, attr);
    VBK_REG = 0;
    set_bkg_tiles(x, y, w, h, map);
}

void style_cell(uint8_t x, uint8_t y, uint8_t tile, uint8_t attr, uint8_t win) {
    VBK_REG = 1;
    if (win) {
        set_win_tile_xy(x, y, attr);
    } else {
        set_bkg_tile_xy(x, y, attr);
    }
    VBK_REG = 0;
    if (win) {
        set_win_tile_xy(x, y, tile);
    } else {
        set_bkg_tile_xy(x, y, tile);
    }
}

uint8_t style_glyph(char c) {
    if (c >= 'a' && c <= 'z') return (uint8_t)(1 + c - 'a');
    if (c >= 'A' && c <= 'Z') return (uint8_t)(27 + c - 'A');
    if (c >= '0' && c <= '9') return (uint8_t)(53 + c - '0');
    if (c == ',') return 63;
    if (c == '.') return 64;
    if (c == '-') return 65;
    if (c == ':') return 66;
    if (c == '!') return 67;
    if (c == '?') return 68;
    if (c == '\'') return 69;
    if (c == '>') return 70;
    if (c == '/') return 71;
    if (c == '+') return 72;
    if (c == '$') return 73;
    return 0;
}

void style_text(uint8_t x, uint8_t y, const char *text, uint8_t pal, uint8_t win) {
    while (*text) {
        style_cell(x, y, (uint8_t)(128 + style_glyph(*text)), (uint8_t)(pal | 0x08), win);
        text++;
        x++;
    }
}

void style_text_n(uint8_t x, uint8_t y, const char *text, uint8_t n, uint8_t pal, uint8_t win) {
    while (n--) {
        style_cell(x, y, (uint8_t)(128 + style_glyph(*text)), (uint8_t)(pal | 0x08), win);
        text++;
        x++;
    }
}

void style_number(uint8_t x, uint8_t y, uint8_t value, uint8_t digits, uint8_t pal, uint8_t win) {
    x = (uint8_t)(x + digits - 1);
    while (digits--) {
        style_cell(x, y, (uint8_t)(128 + 53 + value % 10), (uint8_t)(pal | 0x08), win);
        value /= 10;
        x--;
    }
}

uint8_t style_sprite(uint8_t slot, uint8_t x, uint8_t y, uint8_t tile, uint8_t prop) {
    OAM_item_t *o = &shadow_OAM[slot];
    o->y = y;
    o->x = x;
    o->tile = tile;
    o->prop = prop;
    return (uint8_t)(slot + 1);
}

uint8_t style_wide(uint8_t slot, uint8_t x, uint8_t y, uint8_t left, uint8_t right, uint8_t prop) {
    if (prop & S_FLIPX) {
        slot = style_sprite(slot, x, y, right, prop);
        return style_sprite(slot, (uint8_t)(x + 8), y, left, prop);
    }
    slot = style_sprite(slot, x, y, left, prop);
    return style_sprite(slot, (uint8_t)(x + 8), y, right, prop);
}

void style_hide(uint8_t slot) {
    for (; slot < 40; slot++) {
        shadow_OAM[slot].y = 0;
    }
}
