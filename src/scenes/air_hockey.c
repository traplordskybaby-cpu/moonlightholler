/* Built with love and passion on retroable.dev */

#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>

#include "art.h"
#include "probe.h"
#include "rt/rt.h"
#include "scenes/scenes.h"
#include "shared/sound.h"
#include "shared/style.h"

#define ICE_L 29
#define ICE_R 123
#define ICE_T 9
#define ICE_B 127
#define WIN_SCORE 5

static int16_t px, py;
static int8_t vx, vy;
static uint8_t mx, my, cx, cy;
static uint8_t you, cpu, two, over, frame, shots, kills;

static void scores(void) {
    style_number(1, 13, you, 1, 1, 0);
    style_number(18, 5, cpu, 1, 1, 0);
    style_text(17, 4, two ? " P2" : "CPU", 1, 0);
}

static void serve(int8_t dir) {
    px = 76 * 16;
    py = 64 * 16;
    vx = (int8_t)((rt_random() & 15) - 8);
    vy = (int8_t)(dir * 20);
}

void air_hockey_enter(void) {
    DISPLAY_OFF;
    rt_scene_enter();
    HIDE_WIN;
    style_scene(hockey_tiles, HOCKEY_TILE_COUNT);
    style_map(0, 0, 20, 18, hockey_map, hockey_attr);
    set_bkg_palette(0, 2, hockey_bkg_pal);
    set_sprite_palette(0, 2, hockey_spr_pal);
    style_text(0, 12, "YOU", 1, 0);
    style_text(17, 16, "SEL", 1, 0);
    style_text(17, 17, " 2P", 1, 0);
    you = cpu = two = over = frame = shots = kills = 0;
    mx = 72;
    my = 104;
    cx = 72;
    cy = 16;
    scores();
    serve(1);
    SHOW_SPRITES;
    SHOW_BKG;
    DISPLAY_ON;
}

static uint8_t toward(uint8_t at, uint8_t to, uint8_t step) {
    if (at + step < to) return (uint8_t)(at + step);
    if (at > to + step) return (uint8_t)(at - step);
    return to;
}

static uint8_t strike(uint8_t x, uint8_t y) {
    int8_t dx = (int8_t)((px >> 4) + 4 - (x + 8));
    int8_t dy = (int8_t)((py >> 4) + 4 - (y + 8));
    if (dx <= -11 || dx >= 11 || dy <= -11 || dy >= 11) {
        return 0;
    }
    vx = (int8_t)(dx * 5);
    vy = (int8_t)(dy * 5);
    if (vy > -16 && vy < 16) {
        vy = (int8_t)(y > 64 ? -24 : 24);
    }
    px += vx;
    py += vy;
    sound_hit();
    return 1;
}

static void puck(void) {
    uint8_t centre;
    px += vx;
    py += vy;
    if ((frame & 15) == 0) {
        vx = (int8_t)(vx - (vx >> 4));
        vy = (int8_t)(vy - (vy >> 4));
    }
    if (px < ICE_L * 16) { px = ICE_L * 16; vx = (int8_t)-vx; }
    if (px > ICE_R * 16) { px = ICE_R * 16; vx = (int8_t)-vx; }
    centre = (uint8_t)((px >> 4) + 4);
    if (py < ICE_T * 16) {
        if (centre >= 64 && centre < 96) {
            you++;
            if (kills < 255) kills++;
            sound_effect();
            scores();
            serve(-1);
        } else {
            py = ICE_T * 16;
            vy = (int8_t)-vy;
        }
    }
    if (py > ICE_B * 16) {
        if (centre >= 64 && centre < 96) {
            cpu++;
            sound_effect();
            scores();
            serve(1);
        } else {
            py = ICE_B * 16;
            vy = (int8_t)-vy;
        }
    }
}

uint8_t air_hockey_frame(uint8_t keys, uint8_t prev) {
    uint8_t slot, pressed = (uint8_t)(keys & ~prev);
    uint8_t pcx, pcy;
    frame++;
    if (pressed & J_START) {
        return SECTION_TOWN; /* leave early */
    }
    if (over) {
        if (--over == 1) {
            return SECTION_TOWN;
        }
    } else {
        if (pressed & J_SELECT) {
            two ^= 1;
            scores();
        }
        if ((keys & J_LEFT) && mx > ICE_L) mx -= 2;
        if ((keys & J_RIGHT) && mx < ICE_R - 8) mx += 2;
        if ((keys & J_UP) && my > 72) my -= 2;
        if ((keys & J_DOWN) && my < ICE_B - 8) my += 2;
        pcx = (uint8_t)((px >> 4) + 4);
        pcy = (uint8_t)((py >> 4) + 4);
        if (two) {
            if ((keys & J_B) && cx > ICE_L) cx -= 2;
            if ((keys & J_A) && cx < ICE_R - 8) cx += 2;
        } else if (frame & 1 || pcy < 72) {
            cx = toward(cx, pcx > 8 ? (uint8_t)(pcx - 8) : 0, 1);
        }
        cy = toward(cy, pcy < 72 ? (uint8_t)(pcy > 12 ? pcy - 12 : 0) : 14, 1);
        if (cx < ICE_L) cx = ICE_L;
        if (cx > ICE_R - 8) cx = ICE_R - 8;
        if (cy < ICE_T) cy = ICE_T;
        if (cy > 56) cy = 56;
        if (strike(mx, my) && shots < 255) {
            shots++;
        }
        strike(cx, cy);
        puck();
        if (you >= WIN_SCORE || cpu >= WIN_SCORE) {
            style_text(6, 9, you >= WIN_SCORE ? "YOU WIN!" : (two ? " P2 WINS" : "CPU WINS"), 1, 0);
            over = 150;
        }
    }
    slot = style_wide(0, (uint8_t)(mx + 8), (uint8_t)(my + 16), SP_MALLET, SP_MALLET + 1, 0);
    slot = style_wide(slot, (uint8_t)(mx + 8), (uint8_t)(my + 24), SP_MALLET + 2, SP_MALLET + 3, 0);
    slot = style_wide(slot, (uint8_t)(cx + 8), (uint8_t)(cy + 16), SP_MALLET, SP_MALLET + 1, 0);
    slot = style_wide(slot, (uint8_t)(cx + 8), (uint8_t)(cy + 24), SP_MALLET + 2, SP_MALLET + 3, 0);
    style_sprite(slot, (uint8_t)((px >> 4) + 8), (uint8_t)((py >> 4) + 16), SP_PUCK, 1);
    sound_music(frame);
    forge_tick = frame;
    forge_px = mx;
    forge_py = my;
    forge_enemies = 1;
    forge_shots = shots;
    forge_kills = kills;
    forge_health = (uint8_t)(WIN_SCORE - cpu);
    forge_objective = you;
    return SECTION_AIR_HOCKEY;
}
