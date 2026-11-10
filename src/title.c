#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>

#include "art.h"
#include "bank.h"
#include "rt/rt.h"
#include "shared/sound.h"
#include "shared/style.h"
#include "title.h"

/*
 * The cover is 20 x 16 tiles of picture (320 tiles, so it spills into the second VRAM bank) with a two row
 * band underneath for the "PRESS START" prompt.  Data layout inside the COVER blob (see tools/make_assets.py):
 *   0     8 palettes (64 bytes)      64    tile numbers (320)
 *   384   attributes (320)           704   tile graphics (320 x 16)
 */
#define COVER_MAP 64
#define COVER_ATTR 384
#define COVER_TILES 704

static void prompt(uint8_t on) {
    uint8_t x;
    if (on) {
        style_text(4, 16, "PRESS START", COVER_TEXT_PAL, 0);
    } else {
        for (x = 4; x < 15; x++) {
            style_cell(x, 16, 128, (uint8_t)(COVER_TEXT_PAL | 0x08), 0);
        }
    }
}

void title_run(void) {
    static palette_color_t pal[32];
    static uint8_t row[20];
    uint8_t y, x, keys, prev = 0xFF, tick = 0, lcdc;

    DISPLAY_OFF;
    HIDE_SPRITES;
    HIDE_WIN;
    lcdc = LCDC_REG;
    LCDC_REG |= LCDCF_BG8000; /* tiles 0-255 straight from 0x8000 in both banks */

    bk_copy(pal, COVER_BANK, COVER_ADDR, 64);
    set_bkg_palette(0, 8, pal);

    VBK_REG = 0;
    bk_vram((uint8_t *)0x8000, COVER_BANK, COVER_ADDR + COVER_TILES, 256 * 16);
    VBK_REG = 1;
    bk_vram((uint8_t *)0x8000, COVER_BANK, COVER_ADDR + COVER_TILES + 256 * 16, 64 * 16);
    set_bkg_data(128, FONT_TILE_COUNT, font_tiles); /* the prompt's letters, in bank 1 */
    VBK_REG = 0;

    for (y = 0; y < 16; y++) {
        bk_copy(row, COVER_BANK, (uint16_t)(COVER_ADDR + COVER_MAP + y * 20), 20);
        VBK_REG = 0;
        set_bkg_tiles(0, y, 20, 1, row);
        bk_copy(row, COVER_BANK, (uint16_t)(COVER_ADDR + COVER_ATTR + y * 20), 20);
        VBK_REG = 1;
        set_bkg_tiles(0, y, 20, 1, row);
    }
    VBK_REG = 0;
    for (y = 16; y < 18; y++) {
        for (x = 0; x < 20; x++) {
            style_cell(x, y, 128, (uint8_t)(COVER_TEXT_PAL | 0x08), 0);
        }
    }
    move_bkg(0, 0);
    SHOW_BKG;
    DISPLAY_ON;
    prompt(1);

    while (1) {
        keys = joypad();
        if ((keys & (J_START | J_A)) && !(prev & (J_START | J_A))) {
            break;
        }
        prev = keys;
        sound_music(tick);
        tick++;
        if ((tick & 31) == 0) {
            prompt((uint8_t)((tick & 32) == 0));
        }
        vsync();
    }
    rt_seed((uint8_t)(tick ^ (uint8_t)sys_time));
    sound_effect();
    DISPLAY_OFF;
    LCDC_REG = (uint8_t)((LCDC_REG & ~LCDCF_BG8000) | (lcdc & LCDCF_BG8000));
}
