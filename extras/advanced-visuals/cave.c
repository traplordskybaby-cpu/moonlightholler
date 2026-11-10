#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>
#include "scenes.h"
#include "shared/advanced_visuals.h"
#include "shared/style.h"

#define TILE_CEILING 120
#define TILE_FLOOR 121
#define TILE_WALL_A 122
#define TILE_WALL_B 123
#define TILE_WALL_DARK 124
#define TILE_SKY 125
#define TILE_MOUNTAIN 126
#define TILE_FOREGROUND 127

static const uint8_t cave_tiles[8 * 16] = {
    /* 120: near-black ceiling */
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    /* 121: rough floor */
    0x00,0x00,0x18,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x42,0x00,0x00,0x00,0x18,0x00,
    /* 122: stone blocks */
    0x7E,0x00,0x42,0x3C,0x42,0x3C,0x7E,0x00,0x3C,0x42,0x24,0x5A,0x24,0x5A,0x3C,0x42,
    /* 123: mossy stone */
    0x7E,0x00,0x52,0x2C,0x46,0x38,0x7E,0x00,0x3C,0x42,0x2A,0x54,0x24,0x5A,0x3C,0x42,
    /* 124: distant dark stone */
    0x00,0x00,0x18,0x18,0x00,0x00,0x24,0x24,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,
    /* 125: sky gradient pattern */
    0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x24,0x00,0x00,0x00,0x42,0x00,0x00,0x00,
    /* 126: mountain silhouette */
    0x00,0x00,0x10,0x10,0x38,0x38,0x7C,0x7C,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,
    /* 127: foreground ridge */
    0x00,0x00,0x00,0x00,0x18,0x18,0x3C,0x3C,0x7E,0x7E,0x7E,0x7E,0xFF,0xFF,0xFF,0xFF
};

static const uint8_t crystal_tiles[4 * 16] = {
    0x00,0x00,0x10,0x10,0x38,0x28,0x7C,0x54,0xFE,0xAA,0x7C,0x54,0x38,0x28,0x10,0x10,
    0x00,0x00,0x08,0x08,0x1C,0x14,0x3E,0x2A,0x7F,0x55,0x3E,0x2A,0x1C,0x14,0x08,0x08,
    0x10,0x10,0x38,0x28,0x7C,0x54,0xFE,0xAA,0x7C,0x54,0x38,0x28,0x10,0x10,0x00,0x00,
    0x08,0x08,0x1C,0x14,0x3E,0x2A,0x7F,0x55,0x3E,0x2A,0x1C,0x14,0x08,0x08,0x00,0x00
};
static const palette_color_t crystal_palette[] = {
    RGB(0, 0, 0), RGB(5, 12, 24), RGB(12, 24, 31), RGB(28, 31, 31)
};

static const palette_color_t cave_glow_a[] = {
    RGB(0, 3, 10), RGB(5, 12, 24), RGB(12, 24, 31), RGB(28, 31, 31)
};
static const palette_color_t cave_glow_b[] = {
    RGB(0, 5, 8), RGB(3, 16, 18), RGB(9, 25, 23), RGB(25, 31, 27)
};

static const palette_color_t cave_palette[] = {
    RGB(2, 3, 8), RGB(7, 10, 17), RGB(15, 18, 21), RGB(26, 25, 19),
    RGB(1, 4, 13), RGB(4, 10, 20), RGB(9, 18, 25), RGB(20, 25, 27),
    RGB(2, 5, 7), RGB(7, 12, 12), RGB(13, 19, 15), RGB(24, 25, 17),
    RGB(0, 2, 8), RGB(4, 7, 14), RGB(9, 13, 19), RGB(18, 21, 24)
};

static const uint8_t intro0[] = { 255,TILE_CEILING, 105,TILE_CEILING };
static const uint8_t intro1[] = { 55,TILE_CEILING, 250,TILE_WALL_DARK, 55,TILE_CEILING };
static const uint8_t intro2[] = { 35,TILE_CEILING, 255,TILE_WALL_A, 35,TILE_WALL_A, 35,TILE_CEILING };
static const uint8_t * const intro_frames[] = { intro0, intro1, intro2 };
static const uint8_t intro_sizes[] = { sizeof(intro0), sizeof(intro1), sizeof(intro2) };
static uint8_t cave_map[AV_VIEW_W * AV_VIEW_H];
static uint8_t cave_attrs[AV_VIEW_W * AV_VIEW_H];
static uint8_t cave_window_attrs[AV_VIEW_W * 9];
static uint8_t view_mode;
static uint8_t facing;
static uint8_t anim;
static uint8_t intro_step;
static int16_t player_x, player_y;
static uint8_t mountain_offset;
static const uint8_t cave_grid[AV_MAP_W * AV_MAP_H] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,1,1,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,1,0,0,0,0,1,1,0,0,1,
    1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,
    1,0,0,1,1,0,0,0,0,0,0,1,0,0,0,1,
    1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,1,0,0,0,0,1,1,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

static uint8_t cave_blocked(int16_t x, int16_t y) {
    int16_t tx = x >> 8;
    int16_t ty = y >> 8;
    if (tx < 0 || ty < 0 || tx >= AV_MAP_W || ty >= AV_MAP_H) return 1;
    return cave_grid[(uint8_t)ty * AV_MAP_W + (uint8_t)tx];
}

static void draw_intro(void) {
    uint16_t decoded;
    decoded = av_rle_decode(intro_frames[intro_step], intro_sizes[intro_step],
                            cave_map, AV_VIEW_W * AV_VIEW_H);
    if (decoded == AV_VIEW_W * AV_VIEW_H) set_bkg_tiles(0, 0, AV_VIEW_W, AV_VIEW_H, cave_map);
}

static void draw_raycast(void) {
    av_ray_column_t rays[AV_VIEW_W];
    uint8_t x, y;
    av_raycast(cave_grid, player_x, player_y, facing, rays);
    for (x = 0; x < AV_VIEW_W; ++x) {
        uint8_t top = rays[x].top;
        uint8_t bottom = rays[x].bottom;
        uint8_t wall = (uint8_t)(TILE_WALL_A + (rays[x].texture & 1));
        if (rays[x].distance > 7) wall = TILE_WALL_DARK;
        if (rays[x].side) wall = TILE_WALL_B;
        for (y = 0; y < AV_VIEW_H; ++y) {
            if (y < top) cave_map[(uint16_t)y * AV_VIEW_W + x] = TILE_CEILING;
            else if (y <= bottom) cave_map[(uint16_t)y * AV_VIEW_W + x] = wall;
            else cave_map[(uint16_t)y * AV_VIEW_W + x] = TILE_FLOOR;
        }
    }
    set_bkg_tiles(0, 0, AV_VIEW_W, AV_VIEW_H, cave_map);
    style_text(0, 17, "B: BACK  SEL: VIEW", 1, 0);
}

static void draw_panorama(void) {
    uint8_t heights[AV_VIEW_W];
    uint8_t layer[AV_VIEW_W * 9];
    uint8_t sky[AV_VIEW_W * AV_VIEW_H];
    uint8_t x, y;
    static const uint8_t ridge[32] = {
        2,3,5,4,6,7,5,3,2,4,6,7,6,4,3,2,
        3,5,7,6,4,3,2,4,5,7,6,5,3,2,3,4
    };
    av_heightfield(ridge, mountain_offset, heights);
    for (y = 0; y < AV_VIEW_H; ++y)
        for (x = 0; x < AV_VIEW_W; ++x)
            sky[(uint16_t)y * AV_VIEW_W + x] = TILE_SKY;
    for (y = 0; y < 9; ++y) {
        for (x = 0; x < AV_VIEW_W; ++x) {
            uint8_t h = heights[x];
            layer[(uint16_t)y * AV_VIEW_W + x] = y < (uint8_t)(8 - h)
                ? TILE_SKY : (y == (uint8_t)(8 - h) ? TILE_MOUNTAIN : TILE_FOREGROUND);
        }
    }
    set_bkg_tiles(0, 0, AV_VIEW_W, AV_VIEW_H, sky);
    set_win_tiles(0, 0, AV_VIEW_W, 9, layer);
    move_win(7, 72);
    style_text(0, 8, "SEL: CAVE  B: BACK", 1, 1);
    SHOW_WIN;
    SCX_REG = (uint8_t)(mountain_offset >> 1);
}

void cave_enter(void) {
    DISPLAY_OFF;
    view_mode = 0;
    facing = 0;
    anim = 0;
    intro_step = 0;
    player_x = 3 * 256 + 128;
    player_y = 3 * 256 + 128;
    mountain_offset = 0;
    set_bkg_data(TILE_CEILING, 8, cave_tiles);
    set_bkg_palette(0, 4, cave_palette);
    memset(cave_attrs, 0, sizeof(cave_attrs));
    memset(cave_window_attrs, 0, sizeof(cave_window_attrs));
    VBK_REG = 1;
    set_bkg_tiles(0, 0, AV_VIEW_W, AV_VIEW_H, cave_attrs);
    set_win_tiles(0, 0, AV_VIEW_W, 9, cave_window_attrs);
    VBK_REG = 0;
    set_sprite_data(120, 4, crystal_tiles);
    set_sprite_palette(0, 1, crystal_palette);
    HIDE_SPRITES;
    HIDE_WIN;
    draw_intro();
    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;
}

uint8_t cave_frame(uint8_t keys, uint8_t prev) {
    uint8_t pressed = (uint8_t)(keys & ~prev);
    if (pressed & J_B) {
        HIDE_WIN;
        SCX_REG = 0;
        return SECTION_TOWN;
    }
    /* Three compressed keyframes play as a short tile-streamed entrance film. */
    if (intro_step < 3) {
        if (++anim >= 8) {
            anim = 0;
            ++intro_step;
            if (intro_step < 3) draw_intro();
            else draw_raycast();
        }
        return SECTION_CAVE;
    }
    if (pressed & J_SELECT) {
        view_mode ^= 1;
        if (view_mode) draw_panorama();
        else { HIDE_WIN; SCX_REG = 0; draw_raycast(); }
    }
    if ((anim & 15) == 0) set_bkg_palette(0, 1, (anim & 16) ? cave_glow_a : cave_glow_b);
    if (view_mode) {
        style_hide(0);
        if (keys & J_LEFT) mountain_offset--;
        if (keys & J_RIGHT) mountain_offset++;
        if ((anim & 3) == 0) draw_panorama();
    } else {
        if (pressed & J_LEFT) facing = (uint8_t)((facing + 3) & 3);
        if (pressed & J_RIGHT) facing = (uint8_t)((facing + 1) & 3);
        if ((keys & J_UP) && (anim & 3) == 0) {
            int16_t nx = (int16_t)(player_x + ((facing == 1) ? 64 : (facing == 3) ? -64 : 0));
            int16_t ny = (int16_t)(player_y + ((facing == 2) ? 64 : (facing == 0) ? -64 : 0));
            if (!cave_blocked(nx, ny)) {
                player_x = nx; player_y = ny;
            }
        }
        if ((keys & J_DOWN) && (anim & 3) == 0) {
            int16_t nx = (int16_t)(player_x - ((facing == 1) ? 64 : (facing == 3) ? -64 : 0));
            int16_t ny = (int16_t)(player_y - ((facing == 2) ? 64 : (facing == 0) ? -64 : 0));
            if (!cave_blocked(nx, ny)) {
                player_x = nx; player_y = ny;
            }
        }
        if ((anim & 1) == 0) draw_raycast();
        style_quad(0, 136, 104, 120, 121, 122, 123, 0);
    }
    ++anim;
    return SECTION_CAVE;
}
