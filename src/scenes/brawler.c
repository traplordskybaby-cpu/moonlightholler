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

#define FIGHTERS 4
#define FULL_HP 72
#define GROUND 112
#define IDLE 0
#define PUNCH 1
#define KICK 2
#define HURT 3

typedef struct {
    uint8_t x, state, timer, hp, left, kind, walk;
} fighter_t;

static const char *const names[FIGHTERS] = {"Duke", "Mae", "Bubba", "Lulu"};
static const uint8_t name_col[FIGHTERS] = {1, 6, 10, 15};
static const uint8_t bar_tile[9] = {BRAWL_T_BAR0, BRAWL_T_BAR1, BRAWL_T_BAR2, BRAWL_T_BAR3, BRAWL_T_BAR4,
                                    BRAWL_T_BAR5, BRAWL_T_BAR6, BRAWL_T_BAR7, BRAWL_T_BAR8};
static fighter_t you, rival;
static uint8_t phase, pick, frame, cam, wait, shots, kills, spark_x, spark_t;

static void blank(void) {
    uint8_t x, y;
    for (y = 0; y < 18; y++) {
        for (x = 0; x < 20; x++) {
            style_cell(x, y, BRAWL_T_BLANK, 3, 0);
        }
    }
}

void brawler_enter(void) {
    uint8_t i;
    DISPLAY_OFF;
    rt_scene_enter();
    HIDE_WIN;
    style_scene(brawl_tiles, BRAWL_TILE_COUNT);
    set_bkg_palette(0, 4, brawl_bkg_pal);
    set_sprite_palette(0, 8, brawl_fighter_pal);
    blank();
    style_text(5, 1, "Dirt Road", 3, 0);
    style_text(5, 2, "Brawler 2", 3, 0);
    style_text(3, 5, "Pick a fighter", 3, 0);
    for (i = 0; i < FIGHTERS; i++) {
        style_text(name_col[i], 12, names[i], 3, 0);
    }
    style_text(2, 15, "A punch  B kick", 3, 0);
    phase = 0;
    pick = 0;
    frame = 0;
    wait = 0;
    shots = 0;
    kills = 0;
    SHOW_SPRITES;
    SHOW_BKG;
    DISPLAY_ON;
}

static void bars(void) {
    uint8_t i, a, b;
    for (i = 0; i < 9; i++) {
        a = you.hp > i * 8 ? (uint8_t)(you.hp - i * 8) : 0;
        b = rival.hp > i * 8 ? (uint8_t)(rival.hp - i * 8) : 0;
        style_cell(i, 1, bar_tile[a > 8 ? 8 : a], 3, 1);
        style_cell((uint8_t)(19 - i), 1, bar_tile[b > 8 ? 8 : b], 0x23, 1);
    }
}

static void fight_start(void) {
    uint8_t x;
    DISPLAY_OFF;
    style_map(0, 0, 32, 16, brawl_map, brawl_attr);
    for (x = 0; x < 20; x++) {
        style_cell(x, 0, BRAWL_T_BLANK, 3, 1);
        style_cell(x, 1, BRAWL_T_BLANK, 3, 1);
    }
    you.kind = pick;
    rival.kind = (uint8_t)((pick + 2) & 3);
    style_text(0, 0, names[you.kind], 3, 1);
    style_text(9, 0, "VS", 3, 1);
    style_text((uint8_t)(20 - (rival.kind == 2 ? 5 : (rival.kind == 1 ? 3 : 4))), 0, names[rival.kind], 3, 1);
    set_sprite_palette(0, 2, &brawl_fighter_pal[you.kind * 8]);
    set_sprite_palette(2, 2, &brawl_fighter_pal[rival.kind * 8]);
    set_sprite_palette(4, 1, brawl_spark_pal);
    you.x = 104;
    rival.x = 152;
    you.hp = rival.hp = FULL_HP;
    you.state = rival.state = IDLE;
    you.timer = rival.timer = 0;
    rival.left = 1;
    you.left = 0;
    bars();
    move_win(7, 128);
    SHOW_WIN;
    phase = 1;
    DISPLAY_ON;
}

static uint8_t gap(void) {
    return you.x < rival.x ? (uint8_t)(rival.x - you.x) : (uint8_t)(you.x - rival.x);
}

static void attack(fighter_t *f, uint8_t kind) {
    f->state = kind;
    f->timer = kind == PUNCH ? 14 : 20;
    if (f == &you && shots < 255) {
        shots++;
    }
}

static void land(fighter_t *f, fighter_t *target) {
    uint8_t reach = f->state == PUNCH ? 24 : 28;
    uint8_t facing = f->left ? target->x < f->x : target->x > f->x;
    if (!facing || gap() > reach || target->state == HURT) {
        return;
    }
    target->hp = target->hp > (f->state == PUNCH ? 6 : 9) ? (uint8_t)(target->hp - (f->state == PUNCH ? 6 : 9)) : 0;
    target->state = HURT;
    target->timer = 12;
    spark_x = f->left ? (uint8_t)(f->x - 6) : (uint8_t)(f->x + 14);
    spark_t = 8;
    sound_hit();
    bars();
}

static void step(fighter_t *f, int8_t dir) {
    uint8_t to = (uint8_t)(f->x + dir);
    fighter_t *other = f == &you ? &rival : &you;
    uint8_t apart = to < other->x ? (uint8_t)(other->x - to) : (uint8_t)(to - other->x);
    if (to < 8 || to > 232 || apart < 14 || apart > 140) {
        return;
    }
    f->x = to;
    f->walk++;
}

static void tick(fighter_t *f, fighter_t *target) {
    if (!f->timer) {
        f->state = IDLE;
        return;
    }
    f->timer--;
    if (f->state == HURT) {
        if (f->timer > 6) {
            step(f, f->left ? 2 : -2);
        }
    } else if (f->timer == (f->state == PUNCH ? 8 : 12)) {
        land(f, target);
    }
}

static uint8_t draw_fighter(uint8_t slot, fighter_t *f, uint8_t pal) {
    uint8_t sx = (uint8_t)(f->x - cam + 8);
    uint8_t flip = f->left ? S_FLIPX : 0;
    uint8_t torso = f->state == PUNCH && f->timer <= 10 && f->timer >= 4 ? SP_TORSO_PUNCH : SP_TORSO;
    uint8_t legs = f->state == KICK && f->timer <= 16 && f->timer >= 6 ? SP_LEGS_KICK
                   : ((f->walk & 8) ? SP_LEGS_WALK : SP_LEGS);
    uint8_t head = (uint8_t)(SP_HEAD0 + f->kind * 2);
    uint8_t ex = f->left ? (uint8_t)(sx - 8) : (uint8_t)(sx + 16);
    slot = style_wide(slot, sx, GROUND - 8, head, (uint8_t)(head + 1), (uint8_t)(pal | flip));
    slot = style_wide(slot, sx, GROUND, torso, (uint8_t)(torso + 1), (uint8_t)(pal + 1 | flip));
    slot = style_wide(slot, sx, GROUND + 8, legs, (uint8_t)(legs + 1), (uint8_t)(pal + 1 | flip));
    if (torso == SP_TORSO_PUNCH) {
        return style_sprite(slot, ex, GROUND, SP_FIST, (uint8_t)(pal | flip));
    }
    if (legs == SP_LEGS_KICK) {
        return style_sprite(slot, ex, GROUND + 8, SP_BOOT, (uint8_t)(pal + 1 | flip));
    }
    return style_sprite(slot, 0, 0, SP_FIST, 0);
}

static void select_frame(uint8_t pressed) {
    uint8_t i, slot = 0;
    uint8_t x;
    if ((pressed & J_LEFT) && pick) pick--;
    if ((pressed & J_RIGHT) && pick < FIGHTERS - 1) pick++;
    for (i = 0; i < FIGHTERS; i++) {
        uint8_t head = (uint8_t)(SP_HEAD0 + i * 2);
        x = (uint8_t)(20 + i * 38);
        slot = style_wide(slot, x, 72, head, (uint8_t)(head + 1), (uint8_t)(i * 2));
        slot = style_wide(slot, x, 80, SP_TORSO, SP_TORSO + 1, (uint8_t)(i * 2 + 1));
        slot = style_wide(slot, x, 88, SP_LEGS, SP_LEGS + 1, (uint8_t)(i * 2 + 1));
    }
    x = (uint8_t)(24 + pick * 38);
    style_sprite(slot, x, (uint8_t)(104 + ((frame >> 3) & 1)), SP_CURSOR, 0);
    forge_px = x;
    forge_py = 104;
    if (++wait >= 120 || (pressed & (J_A | J_START))) {
        sound_effect();
        style_hide(0);
        fight_start();
    }
}

uint8_t brawler_frame(uint8_t keys, uint8_t prev) {
    uint8_t slot, pressed = (uint8_t)(keys & ~prev);
    uint8_t mid;
    frame++;
    if (pressed & J_SELECT) {
        return SECTION_TOWN; /* leave early */
    }
    if (phase == 0) {
        select_frame(pressed);
    } else {
        if (phase == 1) {
            if (you.state == IDLE) {
                if (keys & J_LEFT) step(&you, -1);
                if (keys & J_RIGHT) step(&you, 1);
                if (pressed & J_A) attack(&you, PUNCH);
                else if (pressed & J_B) attack(&you, KICK);
            }
            if (rival.state == IDLE) {
                uint8_t roll = rt_random();
                if (gap() > 22) {
                    if (frame & 3) step(&rival, rival.x > you.x ? -1 : 1);
                } else if (roll < 10) {
                    attack(&rival, PUNCH);
                } else if (roll < 16) {
                    attack(&rival, KICK);
                } else if (roll < 22) {
                    step(&rival, rival.x > you.x ? 2 : -2);
                }
            }
            tick(&you, &rival);
            tick(&rival, &you);
            you.left = rival.x < you.x;
            rival.left = you.x < rival.x;
            if (!you.hp || !rival.hp) {
                if (!rival.hp && kills < 255) kills++;
                style_text((uint8_t)(((cam >> 3) + 7) & 31), 4, "K.O.!", 3, 0);
                style_text((uint8_t)(((cam >> 3) + 5) & 31), 6, rival.hp ? "YOU LOSE" : "YOU WIN!", 3, 0);
                phase = 2;
                wait = 150;
            }
        } else if (--wait == 0) {
            return SECTION_TOWN;
        }
        mid = (uint8_t)((you.x >> 1) + (rival.x >> 1));
        cam = mid < 80 ? 0 : (mid > 176 ? 96 : (uint8_t)(mid - 80));
        move_bkg(cam, 0);
        slot = draw_fighter(0, &you, 0);
        slot = draw_fighter(slot, &rival, 2);
        if (spark_t) {
            spark_t--;
            style_sprite(slot, (uint8_t)(spark_x - cam + 8), GROUND + 2, SP_SPARK, 4);
        } else {
            style_sprite(slot, 0, 0, SP_SPARK, 4);
        }
        forge_px = (uint8_t)(you.x - cam + 8);
        forge_py = GROUND;
    }
    sound_music(frame);
    forge_tick = frame;
    forge_enemies = phase ? 1 : 0;
    forge_shots = shots;
    forge_kills = kills;
    forge_health = (uint8_t)(you.hp >> 3);
    forge_objective = kills;
    return SECTION_BRAWLER;
}
