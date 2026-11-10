/*
 * The Holler: the farm and everything you can walk to from it.
 *
 * The world is a set of "areas" (farm, trailer, village, store, lake).  Each area is a blob in ROM that is
 * copied out when you walk in through a door or off the end of a trail (see tools/areas.py).  The farm
 * systems (crops, crows, guinea pigs, hens, your farm buddy, the clock) are the originals; they only run
 * on the farm.  The clock, hearts and gold follow you everywhere.
 */

#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>

#include "art.h"
#include "bank.h"
#include "probe.h"
#include "rt/rt.h"
#include "scenes/scenes.h"
#include "shared/farm_animals.h"
#include "shared/sound.h"
#include "shared/style.h"

#define PLOTS 52
#define FIELD_PLOTS 24
#define RIPE 2
#define WET 4
#define FRAMES_PER_MINUTE 20
#define CROWS 3
#define DROPS 4
#define PIGS 3
#define HENS 3
#define CUKE 0
#define PEPPER 1
#define LETTUCE 2
#define CORN 3
#define HAY 4

#define DIR_DOWN 0
#define DIR_UP 1
#define DIR_LEFT 2
#define DIR_RIGHT 3

#define SLOT_HERO 0
#define SLOT_BUDDY 4
#define SLOT_HUD 26
#define MAX_NPCS 4
#define MAX_EXITS 6
#define MAX_HOTS 8
#define GOLD_MAX 99

#define M_PLAY 0
#define M_TALK 1
#define M_MENU 2
#define MENU_SHOP 0
#define MENU_BED 1

typedef struct {
    uint8_t x, y, tx, ty, state, timer;
} mover_t;

typedef struct {
    uint8_t x, y, tx, ty, timer, dir, step, look, role, x0, y0, x1, y1, moving;
} npc_t;

typedef struct {
    uint8_t x, y, life;
    int8_t dx, dy;
} drop_t;

/* ---- game state (kept while you visit the minigames) ---- */
static uint8_t ready;
static uint8_t frame;
static uint8_t plot[PLOTS];
static uint8_t crops[5];
static uint8_t gp_happy;
static uint8_t hen_happy;
static uint8_t gold;
static uint8_t day;
static uint8_t hour;
static uint8_t minute;
static uint8_t grow_left;
static uint8_t minute_tick;
static uint8_t night;
static uint8_t active;
static uint8_t harvested;
static uint8_t shots;
static uint8_t kills;
static uint8_t spray_wait;
static uint8_t quest[2];
static uint8_t pearl_talks;
static uint8_t gus_talks;

/* ---- where we are ---- */
static uint8_t area;
static uint8_t aw, ah, aflags;
static uint8_t solid[128];
static uint8_t nexit, nhot, nnpc;
static uint8_t exits[MAX_EXITS][8];
static uint8_t hots[MAX_HOTS][6];
static npc_t npcs[MAX_NPCS];
static palette_color_t pal_day[32];
static palette_color_t pal_night[32];
static uint8_t view_x, view_y;
static uint8_t hx, hy;
static uint8_t hdir, hstep, hmoving;

/* ---- farm creatures ---- */
static mover_t crows[CROWS];
static uint8_t crow_wait;
static mover_t pigs[PIGS];
static uint8_t hen_pal[HENS];
static uint8_t bubble_pig;
static uint8_t bubble_time;
static mover_t hens[HENS];
static mover_t buddy;
static uint8_t buddy_dir;
static uint8_t buddy_cool;
static uint8_t buddy_step;
static drop_t drops[DROPS];

/* ---- window box (dialogue / menus) ---- */
static uint8_t ui_mode;
static uint8_t hud_y;
static uint8_t dlg_first;
static uint8_t dlg_count;
static uint8_t dlg_page;
static uint8_t menu_kind;
static uint8_t menu_sel;
static uint8_t menu_msg;
static uint8_t after_dialog;

static const uint8_t hud_kind[5] = {CUKE, HAY, PEPPER, LETTUCE, CORN};
static const uint8_t hud_icon[5] = {SP_ICON_CUKE, SP_ICON_HAY, SP_ICON_PEPPER, SP_ICON_LETTUCE, SP_ICON_CORN};
static const uint8_t hud_pal[5] = {3, 0, 5, 3, 0};
static const uint8_t pig_tile[PIGS] = {SP_PIG_A, SP_PIG_B, SP_PIG_C};
static const uint8_t sell_price[5] = {2, 3, 2, 2, 1};

static void hud_sprites(void);
static void hud_count(uint8_t kind);
static uint8_t hud_dirty;
static void go(uint8_t id, uint8_t tx, uint8_t ty, uint8_t dir);
static void menu_open(uint8_t kind);

/* ====================================================================== crop plots */

static void plot_cell(uint8_t i, uint8_t *col, uint8_t *row, uint8_t *kind) {
    if (i < FIELD_PLOTS) {
        *col = (uint8_t)(11 + (i & 7));
        *row = (uint8_t)(6 + (i >> 3));
        *kind = (uint8_t)((i & 7) >> 1);
    } else {
        i -= FIELD_PLOTS;
        *col = (uint8_t)(12 + i % 7);
        *row = (uint8_t)(10 + i / 7);
        *kind = HAY;
    }
}

static uint8_t plot_at(uint8_t col, uint8_t row) {
    if (row >= 6 && row <= 8 && col >= 11 && col <= 18) {
        return (uint8_t)((row - 6) * 8 + col - 11);
    }
    if (row >= 10 && row <= 13 && col >= 12 && col <= 18) {
        return (uint8_t)(FIELD_PLOTS + (row - 10) * 7 + col - 12);
    }
    return 255;
}

static void plot_draw(uint8_t i) {
    uint8_t col, row, kind, look;
    uint8_t p = plot[i];
    if (area != AREA_FARM) {
        return;
    }
    plot_cell(i, &col, &row, &kind);
    look = (p & 3) == RIPE ? 4 : (uint8_t)(((p & 3) << 1) | ((p & WET) ? 1 : 0));
    style_cell(col, row, farm_crop_tile[kind * 5 + look], farm_crop_attr[kind], 0);
}

static uint8_t redraw[8];
static uint8_t redraws;

static void plot_mark(uint8_t i) {
    if (redraws < 8) {
        redraw[redraws++] = i;
    } else {
        plot_draw(i);
    }
}

static void water_at(uint8_t mx, uint8_t my) {
    uint8_t i = plot_at((uint8_t)(mx >> 3), (uint8_t)(my >> 3));
    if (i != 255 && (plot[i] & 3) != RIPE && !(plot[i] & WET)) {
        plot[i] |= WET;
        plot_mark(i);
    }
}

static uint8_t harvest_at(uint8_t mx, uint8_t my) {
    uint8_t col, row, kind;
    uint8_t i = plot_at((uint8_t)(mx >> 3), (uint8_t)(my >> 3));
    if (i == 255 || (plot[i] & 3) != RIPE) {
        return 0;
    }
    plot_cell(i, &col, &row, &kind);
    plot[i] = 0;
    plot_mark(i);
    if (crops[kind] < 9) {
        crops[kind]++;
        hud_dirty |= (uint8_t)(1 << kind);
    }
    if (harvested < 255) {
        harvested++;
    }
    return 1;
}

/* ====================================================================== HUD */

#define HY(y) ((uint8_t)(hud_y + (y)))

static void hud_hearts(void) {
    uint8_t i;
    for (i = 0; i < 5; i++) {
        style_cell((uint8_t)(1 + i), HY(0), i < gp_happy ? T_HEART : T_HEART_EMPTY, 1, 1);
        style_cell((uint8_t)(8 + i), HY(0), i < hen_happy ? T_HEART : T_HEART_EMPTY, 1, 1);
    }
}

static void hud_counts(void) {
    uint8_t i;
    for (i = 0; i < 5; i++) {
        style_number((uint8_t)(1 + i * 2), HY(1), crops[hud_kind[i]], 1, 1, 1);
    }
}

static void hud_count(uint8_t kind) {
    uint8_t i;
    for (i = 0; i < 5; i++) {
        if (hud_kind[i] == kind) {
            style_number((uint8_t)(1 + i * 2), HY(1), crops[kind], 1, 1, 1);
        }
    }
}

static void hud_gold(void) {
    style_number(11, HY(1), gold, 2, 1, 1);
}

static void hud_minutes(void) {
    style_number(17, HY(1), minute, 2, 1, 1);
}

static void hud_clock(void) {
    uint8_t h12 = (uint8_t)(hour % 12);
    if (h12 == 0) {
        h12 = 12;
    }
    style_text(15, HY(0), "Day", 1, 1);
    if (day < 10) {
        style_text(18, HY(0), " ", 1, 1);
        style_number(19, HY(0), day, 1, 1, 1);
    } else {
        style_number(18, HY(0), day, 2, 1, 1);
    }
    if (h12 < 10) {
        style_text(14, HY(1), " ", 1, 1);
        style_number(15, HY(1), h12, 1, 1, 1);
    } else {
        style_number(14, HY(1), h12, 2, 1, 1);
    }
    style_text(16, HY(1), ":", 1, 1);
    hud_minutes();
    style_text(19, HY(1), hour < 12 ? "a" : "p", 1, 1);
}

static void hud_draw(void) {
    uint8_t x, y;
    for (y = 0; y < 2; y++) {
        for (x = 0; x < 20; x++) {
            style_cell(x, HY(y), T_HUD, 1, 1);
        }
    }
    hud_hearts();
    hud_counts();
    hud_gold();
    hud_clock();
}

/* ====================================================================== light and fades */

static palette_color_t fade_b[32];
static palette_color_t fade_o[32];

static palette_color_t dim(palette_color_t c, uint8_t level) {
    uint8_t r = (uint8_t)(c & 31);
    uint8_t g = (uint8_t)((c >> 5) & 31);
    uint8_t b = (uint8_t)((c >> 10) & 31);
    r = (uint8_t)((uint8_t)(r * level) >> 3);
    g = (uint8_t)((uint8_t)(g * level) >> 3);
    b = (uint8_t)((uint8_t)(b * level) >> 3);
    return (palette_color_t)(r | ((uint16_t)g << 5) | ((uint16_t)b << 10));
}

static void light_calc(void) {
    night = (uint8_t)((hour >= 20 || hour < 6) && !(aflags & FLAG_INDOOR));
}

/* level 8 = normal, 0 = black */
static void fade_apply(uint8_t level) {
    uint8_t i;
    const palette_color_t *src = night ? pal_night : pal_day;
    for (i = 0; i < 32; i++) {
        fade_b[i] = dim(src[i], level);
        fade_o[i] = dim(farm_spr_pal[i], level);
    }
    set_bkg_palette(0, 8, fade_b);
    set_sprite_palette(0, 8, fade_o);
}

static void set_light(void) {
    light_calc();
    set_bkg_palette(0, 8, night ? pal_night : pal_day);
    hud_sprites();
}

static void fade_out(void) {
    uint8_t l;
    for (l = 7; l < 8; l--) {
        vsync();
        fade_apply(l);
        if (l == 0) {
            break;
        }
    }
}

static void fade_in(void) {
    uint8_t l;
    for (l = 1; l <= 8; l++) {
        vsync();
        fade_apply(l);
    }
}

/* ====================================================================== clock */

static void clock_update(void) {
    if (grow_left) {
        grow_left--;
        if ((plot[grow_left] & WET) && (plot[grow_left] & 3) < RIPE) {
            plot[grow_left] = (uint8_t)((plot[grow_left] & 3) + 1);
            plot_draw(grow_left);
        }
        return;
    }
    if (++minute_tick < FRAMES_PER_MINUTE) {
        if (minute == 0 && minute_tick == 1 && (hour & 1) == 0) {
            if (gp_happy) gp_happy--;
            if (hen_happy) hen_happy--;
            hud_hearts();
        } else if (minute == 0 && minute_tick == 2) {
            set_light();
            hud_clock();
        }
        return;
    }
    minute_tick = 0;
    if (++minute >= 60) {
        minute = 0;
        if (++hour >= 24) {
            hour = 0;
            day++;
        }
        grow_left = PLOTS;
    }
    hud_minutes();
}

/* ====================================================================== farm creatures (original logic) */

static void spray(uint8_t mx, uint8_t my, uint8_t dir) {
    uint8_t i;
    drop_t *d = drops;
    for (i = 0; i < DROPS; i++, d++) {
        if (d->life) {
            continue;
        }
        d->dx = dir == DIR_LEFT ? -2 : (dir == DIR_RIGHT ? 2 : 0);
        d->dy = dir == DIR_UP ? -2 : (dir == DIR_DOWN ? 2 : 0);
        d->x = (uint8_t)(mx + d->dx * 3);
        d->y = (uint8_t)(my - 4 + d->dy * 3);
        d->life = 20;
        if (shots < 255) {
            shots++;
        }
        return;
    }
}

static void shoo(mover_t *c) {
    c->state = 4;
    if (kills < 255) {
        kills++;
    }
    sound_hit();
}

static void drops_update(void) {
    uint8_t i, j;
    drop_t *d = drops;
    mover_t *c;
    for (i = 0; i < DROPS; i++, d++) {
        if (!d->life) {
            continue;
        }
        d->x = (uint8_t)(d->x + d->dx);
        d->y = (uint8_t)(d->y + d->dy);
        d->life--;
        if (d->x > 180 || d->y > 130) {
            d->life = 0;
            continue;
        }
        for (j = 0, c = crows; j < CROWS; j++, c++) {
            if (c->state && c->state < 4 && (uint8_t)(c->x - d->x + 10) < 20 && (uint8_t)(c->y - d->y + 10) < 20) {
                shoo(c);
                d->life = 0;
                break;
            }
        }
        if (!d->life) {
            water_at((uint8_t)(d->x + 4), (uint8_t)(d->y + 4));
        }
    }
}

static void crow_target(mover_t *c, uint8_t mx, uint8_t my) {
    uint8_t col = (uint8_t)((mx >> 3) + (rt_random() & 3));
    uint8_t row = (uint8_t)(my >> 3);
    if (row >= 10) {
        row = row > 13 ? 13 : row;
        col = col < 12 ? 12 : (col > 18 ? 18 : col);
    } else {
        row = row < 6 ? 6 : (row > 8 ? 8 : row);
        col = col < 11 ? 11 : (col > 18 ? 18 : col);
    }
    c->tx = (uint8_t)(col << 3);
    c->ty = (uint8_t)((row << 3) - 2);
}

static void crows_update(uint8_t mx, uint8_t my) {
    uint8_t i, free = 255;
    mover_t *c = crows;
    for (i = 0; i < CROWS; i++, c++) {
        switch (c->state) {
        case 0:
            free = i;
            break;
        case 1:
            if (--c->timer == 0) {
                crow_target(c, mx, my);
                c->state = 2;
            }
            break;
        case 2:
            if (frame & 1) {
                if (c->x != c->tx) c->x = c->x < c->tx ? c->x + 1 : c->x - 1;
                if (c->y != c->ty) c->y = c->y < c->ty ? c->y + 1 : c->y - 1;
            }
            if (c->x == c->tx && c->y == c->ty) {
                c->state = 3;
                c->timer = 200;
            }
            break;
        case 3:
            if (--c->timer == 0) {
                uint8_t p = plot_at((uint8_t)((c->x + 4) >> 3), (uint8_t)((c->y + 6) >> 3));
                if (p != 255) {
                    plot[p] = 0;
                    plot_mark(p);
                }
                c->state = 5;
            }
            break;
        default:
            c->x++;
            c->y -= 2;
            if (c->y > 200 || c->y < 4) {
                c->state = 0;
            }
            break;
        }
    }
    if (crow_wait) {
        crow_wait--;
    } else if (free != 255 && !night) {
        c = &crows[free];
        c->x = (uint8_t)(((mx < 88 ? 88 : (mx > 152 ? 152 : mx)) & 0xF8));
        c->y = 38;
        c->state = 1;
        c->timer = 40;
        crow_wait = 90;
    }
}

static void buddy_pick(void) {
    uint8_t i, col, row, kind;
    mover_t *c = crows;
    for (i = 0; i < CROWS; i++, c++) {
        if (c->state == 2 || c->state == 3) {
            buddy.tx = c->tx > 104 ? (uint8_t)(c->tx - 24) : (uint8_t)(c->tx + 24);
            buddy.ty = (uint8_t)(c->ty + 2);
            buddy.state = 1;
            return;
        }
    }
    i = (uint8_t)(rt_random() % PLOTS);
    plot_cell(i, &col, &row, &kind);
    buddy.tx = (uint8_t)(col << 3);
    buddy.ty = (uint8_t)(row << 3);
    buddy.state = 0;
}

static void buddy_update(void) {
    uint8_t i;
    mover_t *c = crows;
    if (buddy_cool) {
        buddy_cool--;
    } else {
        for (i = 0; i < CROWS; i++, c++) {
            if (c->state == 0 || c->state > 3) {
                continue;
            }
            if ((uint8_t)(c->y - buddy.y + 10) < 20 && (uint8_t)(c->x - buddy.x + 44) < 88) {
                buddy_dir = c->x < buddy.x ? DIR_LEFT : DIR_RIGHT;
            } else if ((uint8_t)(c->x - buddy.x + 8) < 16 && (uint8_t)(c->y - buddy.y + 44) < 88) {
                buddy_dir = c->y < buddy.y ? DIR_UP : DIR_DOWN;
            } else {
                continue;
            }
            spray((uint8_t)(buddy.x + 4), buddy.y, buddy_dir);
            buddy_cool = 16;
            break;
        }
    }
    if (buddy.timer) {
        buddy.timer--;
        return;
    }
    if (buddy.x != buddy.tx) {
        buddy_dir = buddy.x < buddy.tx ? DIR_RIGHT : DIR_LEFT;
        buddy.x = buddy.x < buddy.tx ? buddy.x + 1 : buddy.x - 1;
        buddy_step++;
    } else if (buddy.y != buddy.ty) {
        buddy_dir = buddy.y < buddy.ty ? DIR_DOWN : DIR_UP;
        buddy.y = buddy.y < buddy.ty ? buddy.y + 1 : buddy.y - 1;
        buddy_step++;
    } else {
        if (buddy.state == 0 && !harvest_at((uint8_t)(buddy.x + 4), (uint8_t)(buddy.y + 4))) {
            water_at((uint8_t)(buddy.x + 4), (uint8_t)(buddy.y + 4));
        }
        buddy.timer = 24;
        buddy_pick();
    }
}

static void pigs_update(void) {
    uint8_t i;
    mover_t *p = pigs;
    for (i = 0; i < PIGS; i++, p++) {
        if (p->timer) {
            p->timer--;
            continue;
        }
        if (frame % 3) {
            continue;
        }
        if (p->x != p->tx) {
            p->state = p->x > p->tx;
            p->x = p->x < p->tx ? p->x + 1 : p->x - 1;
        } else if (p->y != p->ty) {
            p->y = p->y < p->ty ? p->y + 1 : p->y - 1;
        } else {
            p->tx = (uint8_t)(24 + (rt_random() % 22));
            p->ty = (uint8_t)(36 + i * 7 + (rt_random() & 3));
            p->timer = (uint8_t)(30 + (rt_random() & 63));
        }
    }
    if (bubble_time) {
        bubble_time--;
    } else if (gp_happy >= 3 && (rt_random() & 127) == 0) {
        bubble_pig = (uint8_t)(rt_random() % PIGS);
        bubble_time = 60;
    }
}

static void hens_update(void) {
    uint8_t i;
    mover_t *h = hens;
    for (i = 0; i < HENS; i++, h++) {
        if (h->timer) {
            h->timer--;
            if ((h->timer & 15) == 0) {
                h->state ^= 2;
            }
            continue;
        }
        if (frame & 3) {
            continue;
        }
        if (h->x != h->tx) {
            h->state = (uint8_t)((h->state & 2) | (h->x > h->tx));
            h->x = h->x < h->tx ? h->x + 1 : h->x - 1;
        } else if (h->y != h->ty) {
            h->y = h->y < h->ty ? h->y + 1 : h->y - 1;
        } else {
            h->tx = (uint8_t)(146 + (rt_random() % 12));
            h->ty = (uint8_t)(50 + i * 9 + (rt_random() & 1));
            h->timer = (uint8_t)(40 + (rt_random() & 63));
        }
    }
}

static void feed_pigs(void) {
    static const uint8_t order[4] = {CUKE, LETTUCE, PEPPER, HAY};
    uint8_t i;
    if (gp_happy >= 5) {
        return;
    }
    for (i = 0; i < 4; i++) {
        if (crops[order[i]]) {
            crops[order[i]]--;
            gp_happy++;
            bubble_pig = order[i] == CUKE ? 2 : (uint8_t)(rt_random() % PIGS);
            bubble_time = 90;
            sound_wheek();
            hud_hearts();
            hud_dirty |= (uint8_t)(1 << order[i]);
            return;
        }
    }
}

static void feed_hens(void) {
    if (hen_happy < 5 && crops[CORN]) {
        crops[CORN]--;
        hen_happy++;
        hud_hearts();
        hud_dirty |= (uint8_t)(1 << CORN);
    }
}

/* ====================================================================== sprites */

static uint8_t band[20];
static OAM_item_t *oam;
static uint8_t s_x, s_y, s_t, s_p;

static void put(void) {
    uint8_t a, b;
    OAM_item_t *o = oam++;
    if (s_y > 0 && s_y < 160) {
        a = s_y > 16 ? (uint8_t)((uint8_t)(s_y - 16) >> 3) : 0;
        b = s_y > 9 ? (uint8_t)((uint8_t)(s_y - 9) >> 3) : 0;
        if (band[a] < 10 && band[b] < 10) {
            band[a]++;
            if (b != a) {
                band[b]++;
            }
            o->y = s_y;
            o->x = s_x;
            o->tile = s_t;
            o->prop = s_p;
            return;
        }
    }
    o->y = 0;
}

static void wide(void) {
    uint8_t left = s_t;
    if (s_p & S_FLIPX) {
        s_t++;
        put();
        s_t = left;
    } else {
        put();
        s_t++;
    }
    s_x += 8;
    put();
}

/* a 16x16 person: head palette hp, body palette bp, tiles base.. (head, body, walking body) */
static void person(uint8_t sx, uint8_t sy, uint8_t base, uint8_t hp, uint8_t bp, uint8_t flip, uint8_t walk) {
    s_x = sx;
    s_y = (uint8_t)(sy - 8);
    s_t = base;
    s_p = (uint8_t)(hp | flip);
    wide();
    s_x = sx;
    s_y = sy;
    s_t = walk ? (uint8_t)(base + 4) : (uint8_t)(base + 2);
    s_p = (uint8_t)(bp | flip);
    wide();
}

static void hud_sprites(void) {
    uint8_t i;
    OAM_item_t *o = &shadow_OAM[SLOT_HUD];
    o->y = 144; o->x = 8; o->tile = SP_ICON_PIG; o->prop = 4; o++;
    o->y = 144; o->x = 64; o->tile = SP_CHICKEN; o->prop = 5; o++;
    o->y = 144; o->x = 120; o->tile = night ? SP_MOON : SP_SUN; o->prop = night ? 7 : 0; o++;
    for (i = 0; i < 5; i++, o++) {
        o->y = 152;
        o->x = (uint8_t)(8 + i * 16);
        o->tile = hud_icon[i];
        o->prop = hud_pal[i];
    }
    o->y = 152; o->x = 88; o->tile = SP_COIN; o->prop = 0;
}

static void draw(void) {
    uint8_t i, vx = view_x;
    mover_t *m;
    npc_t *n;
    uint8_t walk = (uint8_t)(hmoving && (hstep & 8));
    uint8_t flip = hdir == DIR_LEFT ? S_FLIPX : 0;
    memset(band, 0, sizeof(band));
    oam = &shadow_OAM[SLOT_HERO];
    person((uint8_t)(hx - 4 - vx + 8), (uint8_t)(hy - view_y + 16), active ? SP_P2 : SP_P1, active ? 2 : 0, active ? 3 : 1, flip, walk);
    if (area == AREA_FARM) {
        oam = &shadow_OAM[SLOT_BUDDY];
        person((uint8_t)(buddy.x - 4 + 8 - vx), (uint8_t)(buddy.y + 16), !active ? SP_P2 : SP_P1, !active ? 2 : 0, !active ? 3 : 1,
               buddy_dir == DIR_LEFT ? S_FLIPX : 0, (uint8_t)((buddy.x != buddy.tx || buddy.y != buddy.ty) && (buddy_step & 8)));
        for (i = 0, m = crows; i < CROWS; i++, m++) {
            s_x = (uint8_t)(m->x + 8 - vx);
            s_y = m->state ? (uint8_t)(m->y + 16) : 0;
            s_t = (m->state == 1 || (m->state == 3 && (frame & 16))) ? SP_CROW : SP_CROW_FLY;
            s_p = (uint8_t)(6 | (m->x < m->tx ? S_FLIPX : 0));
            put();
        }
        for (i = 0, m = hens; i < HENS; i++, m++) {
            s_x = (uint8_t)(m->x + 8 - vx);
            s_y = (uint8_t)(m->y + 16);
            s_t = (m->state & 2) ? SP_CHICKEN_PECK : SP_CHICKEN;
            s_p = (uint8_t)(hen_pal[i] | ((m->state & 1) ? S_FLIPX : 0));
            put();
        }
        for (i = 0, m = pigs; i < PIGS; i++, m++) {
            s_x = (uint8_t)(m->x + 8 - vx);
            s_y = (uint8_t)(m->y + 16);
            s_t = pig_tile[i];
            s_p = (uint8_t)(4 | (m->state ? S_FLIPX : 0));
            wide();
        }
        s_x = (uint8_t)(pigs[bubble_pig].x + 18 - vx);
        s_y = bubble_time ? (uint8_t)(pigs[bubble_pig].y + 7) : 0;
        s_t = SP_BUBBLE;
        s_p = 7;
        put();
        s_x = (uint8_t)(32 - vx);
        s_y = 74;
        s_t = SP_BOWL;
        put();
        for (i = 0; i < DROPS; i++) {
            s_x = (uint8_t)(drops[i].x + 8 - vx);
            s_y = drops[i].life ? (uint8_t)(drops[i].y + 16) : 0;
            s_t = SP_DROP;
            put();
        }
    } else {
        oam = &shadow_OAM[SLOT_BUDDY];
        for (i = 0, n = npcs; i < nnpc; i++, n++) {
            person((uint8_t)(n->x - 4 - vx + 8), (uint8_t)(n->y - view_y + 16), npc_base[n->look], npc_head_pal[n->look],
                   npc_body_pal[n->look], n->dir == DIR_LEFT ? S_FLIPX : 0, (uint8_t)(n->moving && (n->step & 8)));
        }
    }
}

static void report(void) {
    uint8_t i, n = 0;
    for (i = 0; i < CROWS; i++) {
        if (crows[i].state && crows[i].state < 4) {
            n++;
        }
    }
    forge_tick = frame;
    forge_px = hx;
    forge_py = hy;
    forge_health = gp_happy;
    forge_enemies = n;
    forge_shots = shots;
    forge_kills = kills;
    forge_objective = harvested;
}

/* ====================================================================== movement and collision */

static uint8_t npc_blocks(uint16_t x, uint16_t y) {
    uint8_t i;
    npc_t *n = npcs;
    if (area == AREA_FARM) {
        return 0;
    }
    for (i = 0; i < nnpc; i++, n++) {
        if ((uint8_t)((uint8_t)x - n->x + 5) < 11 && (uint8_t)((uint8_t)y - n->y + 4) < 9) {
            return 1;
        }
    }
    return 0;
}

static uint8_t cell_solid(uint16_t px, uint16_t py) {
    uint8_t col = (uint8_t)(px >> 3);
    uint8_t row = (uint8_t)(py >> 3);
    if (px >= 256 || py >= 256 || col >= aw || row >= ah) {
        return 1;
    }
    return (uint8_t)(solid[(uint8_t)(row * ((aw + 7) >> 3) + (col >> 3))] & (0x80 >> (col & 7)));
}

/* the hero's feet: a 6 x 5 box inside the 8 x 8 cell at (x, y) */
static uint8_t hero_fits(uint16_t x, uint16_t y) {
    if (cell_solid(x + 1, y + 3) || cell_solid(x + 6, y + 3) || cell_solid(x + 1, y + 7) || cell_solid(x + 6, y + 7)) {
        return 0;
    }
    return (uint8_t)!npc_blocks(x, y);
}

static void hero_move(uint8_t keys) {
    int16_t nx, ny;
    int8_t dx = 0, dy = 0;
    if (keys & J_LEFT) {
        dx = -1;
        hdir = DIR_LEFT;
    } else if (keys & J_RIGHT) {
        dx = 1;
        hdir = DIR_RIGHT;
    }
    if (keys & J_UP) {
        dy = -1;
        if (!dx) hdir = DIR_UP;
    } else if (keys & J_DOWN) {
        dy = 1;
        if (!dx) hdir = DIR_DOWN;
    }
    hmoving = 0;
    if (dx) {
        nx = (int16_t)hx + dx;
        if (nx >= 0 && hero_fits((uint16_t)nx, hy)) {
            hx = (uint8_t)nx;
            hmoving = 1;
        }
    }
    if (dy) {
        ny = (int16_t)hy + dy;
        if (ny >= 0 && hero_fits(hx, (uint16_t)ny)) {
            hy = (uint8_t)ny;
            hmoving = 1;
        }
    }
    if (hmoving) {
        hstep++;
    }
}

static void camera_snap(void) {
    int16_t v;
    v = (int16_t)hx + 4 - 80;
    if (v < 0) v = 0;
    if (aw <= 20) v = 0;
    else if (v > (int16_t)(aw * 8 - 160)) v = (int16_t)(aw * 8 - 160);
    view_x = (uint8_t)v;
    v = (int16_t)hy + 4 - 64;
    if (v < 0) v = 0;
    if (ah <= 16) v = 0;
    else if (v > (int16_t)(ah * 8 - 128)) v = (int16_t)(ah * 8 - 128);
    view_y = (uint8_t)v;
}

static void camera(void) {
    int16_t rx = (int16_t)hx + 4 - view_x;
    int16_t ry = (int16_t)hy + 4 - view_y;
    if (aw > 20) {
        if (rx > 100 && view_x < (uint8_t)(aw * 8 - 160)) {
            view_x++;
        } else if (rx < 60 && view_x > 0) {
            view_x--;
        }
    }
    if (ah > 16) {
        if (ry > 80 && view_y < (uint8_t)(ah * 8 - 128)) {
            view_y++;
        } else if (ry < 48 && view_y > 0) {
            view_y--;
        }
    }
}

/* ====================================================================== villagers */

static void npcs_update(void) {
    uint8_t i, col, row;
    npc_t *n = npcs;
    for (i = 0; i < nnpc; i++, n++) {
        n->moving = 0;
        if (n->role == R_SHOP) {
            continue;
        }
        if (n->timer) {
            n->timer--;
            continue;
        }
        if (n->x != n->tx) {
            uint8_t nx = n->x < n->tx ? (uint8_t)(n->x + 1) : (uint8_t)(n->x - 1);
            n->dir = n->x < n->tx ? DIR_RIGHT : DIR_LEFT;
            if ((uint8_t)(nx - hx + 5) < 11 && (uint8_t)(n->y - hy + 4) < 9) {
                n->timer = 12;
            } else {
                n->x = nx;
                n->step++;
                n->moving = 1;
            }
        } else if (n->y != n->ty) {
            uint8_t ny = n->y < n->ty ? (uint8_t)(n->y + 1) : (uint8_t)(n->y - 1);
            n->dir = n->y < n->ty ? DIR_DOWN : DIR_UP;
            if ((uint8_t)(n->x - hx + 5) < 11 && (uint8_t)(ny - hy + 4) < 9) {
                n->timer = 12;
            } else {
                n->y = ny;
                n->step++;
                n->moving = 1;
            }
        } else {
            col = (uint8_t)(n->x0 + rt_random() % (uint8_t)(n->x1 - n->x0 + 1));
            row = (uint8_t)(n->y0 + rt_random() % (uint8_t)(n->y1 - n->y0 + 1));
            n->tx = (uint8_t)(col << 3);
            n->ty = (uint8_t)(row << 3);
            n->timer = (uint8_t)(40 + (rt_random() & 63));
        }
    }
}

/* ====================================================================== loading an area */

static uint16_t u16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static void area_load(void) {
    static uint8_t hdr[24], row[32], row2[32], tbuf[256];
    static uint8_t raw[9];
    uint8_t bank = area_bank[area];
    uint16_t base = area_addr[area];
    uint8_t i, y, n, ntiles;
    bk_copy(hdr, bank, base, 24);
    aw = hdr[0];
    ah = hdr[1];
    aflags = hdr[2];
    nexit = hdr[3] > MAX_EXITS ? MAX_EXITS : hdr[3];
    nhot = hdr[4] > MAX_HOTS ? MAX_HOTS : hdr[4];
    nnpc = hdr[5] > MAX_NPCS ? MAX_NPCS : hdr[5];
    ntiles = hdr[6];
    /* tiles */
    for (i = 0; i < ntiles; i = (uint8_t)(i + 16)) {
        n = (uint8_t)(ntiles - i);
        if (n > 16) {
            n = 16;
        }
        bk_copy(tbuf, bank, (uint16_t)(base + u16(&hdr[16]) + (uint16_t)i * 16), (uint16_t)n * 16);
        set_bkg_data((uint8_t)(128 + i), n, tbuf);
    }
    /* map + attributes, one row at a time */
    for (y = 0; y < ah; y++) {
        bk_copy(row, bank, (uint16_t)(base + u16(&hdr[8]) + (uint16_t)y * aw), aw);
        bk_copy(row2, bank, (uint16_t)(base + u16(&hdr[10]) + (uint16_t)y * aw), aw);
        VBK_REG = 1;
        set_bkg_tiles(0, y, aw, 1, row2);
        VBK_REG = 0;
        set_bkg_tiles(0, y, aw, 1, row);
    }
    bk_copy(solid, bank, (uint16_t)(base + u16(&hdr[12])), (uint16_t)(((aw + 7) >> 3) * ah));
    bk_copy(pal_day, bank, (uint16_t)(base + u16(&hdr[14])), 64);
    bk_copy(pal_night, bank, (uint16_t)(base + u16(&hdr[14]) + 64), 64);
    for (i = 0; i < nexit; i++) {
        bk_copy(exits[i], bank, (uint16_t)(base + u16(&hdr[18]) + (uint16_t)i * 8), 8);
    }
    for (i = 0; i < nhot; i++) {
        bk_copy(hots[i], bank, (uint16_t)(base + u16(&hdr[20]) + (uint16_t)i * 6), 6);
    }
    for (i = 0; i < nnpc; i++) {
        npc_t *np = &npcs[i];
        bk_copy(raw, bank, (uint16_t)(base + u16(&hdr[22]) + (uint16_t)i * 9), 9);
        np->x = np->tx = (uint8_t)(raw[0] << 3);
        np->y = np->ty = (uint8_t)(raw[1] << 3);
        np->look = raw[2];
        np->role = raw[3];
        np->x0 = raw[5];
        np->y0 = raw[6];
        np->x1 = raw[7];
        np->y1 = raw[8];
        np->dir = DIR_DOWN;
        np->step = 0;
        np->moving = 0;
        np->timer = (uint8_t)(20 + i * 30);
    }
    for (i = 0; i < SLOT_HUD; i++) {
        shadow_OAM[i].y = 0;
        shadow_OAM[i].prop = 0;
    }
}

/* each roaming hen wears the colour of one of the newest chickens in the Breeding Book */
static void flock_refresh(void) {
    uint8_t i, n = farm_animals_count(ANIMAL_CHICKEN);
    uint8_t start = n > HENS ? (uint8_t)(n - HENS) : 0;
    const animal_record_t *a;
    uint8_t t;
    for (i = 0; i < HENS; i++) {
        a = farm_animals_get(ANIMAL_CHICKEN, (uint8_t)(start + i));
        t = a ? a->traits : CHICKEN_FEATHER_WHITE;
        if (t & CHICKEN_FEATHER_WHITE) {
            hen_pal[i] = 5;
        } else if (t & CHICKEN_BARRING) {
            hen_pal[i] = 4;
        } else if (t & CHICKEN_FEATHER_BLACK) {
            hen_pal[i] = 6;
        } else {
            hen_pal[i] = 5;
        }
    }
}

static void farm_creatures_reset(void) {
    uint8_t i;
    flock_refresh();
    redraws = 0;
    spray_wait = 0;
    for (i = 0; i < DROPS; i++) {
        drops[i].life = 0;
    }
    for (i = 0; i < CROWS; i++) {
        crows[i].state = 1;
        crows[i].x = crows[i].tx = (uint8_t)(96 + i * 32);
        crows[i].y = crows[i].ty = 38;
        crows[i].timer = (uint8_t)(30 + i * 120);
    }
    crows[2].state = 0;
    crow_wait = 150;
    buddy.x = 112;
    buddy.y = 88;
    buddy.timer = 20;
    buddy_cool = 0;
    buddy_dir = DIR_DOWN;
    buddy_pick();
    bubble_pig = 2;
    bubble_time = 120;
}

static void plots_redraw(void) {
    uint8_t i;
    if (area == AREA_FARM) {
        for (i = 0; i < PLOTS; i++) {
            plot_draw(i);
        }
    }
}

/* walk through a door or off the end of a trail */
static void go(uint8_t id, uint8_t tx, uint8_t ty, uint8_t dir) {
    fade_out();
    DISPLAY_OFF;
    area = id;
    area_load();
    hx = (uint8_t)(tx << 3);
    hy = (uint8_t)(ty << 3);
    hdir = dir;
    hmoving = 0;
    camera_snap();
    move_bkg(view_x, view_y);
    light_calc();
    fade_apply(0);
    if (area == AREA_FARM) {
        farm_creatures_reset();
    }
    plots_redraw();
    hud_sprites();
    draw();
    DISPLAY_ON;
    fade_in();
    sound_effect();
}

/* ====================================================================== window box: dialogue and menus */

static void box_open(void) {
    uint8_t x, y;
    hud_y = 4;
    for (x = 0; x < 20; x++) {
        style_cell(x, 0, T_BOX, 1, 1);
        for (y = 1; y < 4; y++) {
            style_cell(x, y, T_HUD, 1, 1);
        }
    }
    hud_draw();
    move_win(7, 96);
}

static void box_close(void) {
    hud_y = 0;
    move_win(7, 128);
    hud_draw();
}

static void box_clear_line(uint8_t row) {
    uint8_t x;
    for (x = 1; x < 19; x++) {
        style_cell(x, row, T_HUD, 1, 1);
    }
}

static void dialog_show(void) {
    char line[18];
    uint8_t r, c;
    uint16_t at = (uint16_t)(DIALOG_ADDR + DLG_COUNT * 3 + (uint16_t)(dlg_first + dlg_page) * 54);
    for (r = 0; r < 3; r++) {
        bk_copy(line, DIALOG_BANK, (uint16_t)(at + r * 18), 18);
        for (c = 0; c < 18; c++) {
            style_cell((uint8_t)(1 + c), (uint8_t)(1 + r), (uint8_t)(128 + style_glyph(line[c])), (uint8_t)(1 | 0x08), 1);
        }
    }
}

static void dialog_open(uint8_t id) {
    uint8_t ent[3];
    bk_copy(ent, DIALOG_BANK, (uint16_t)(DIALOG_ADDR + id * 3), 3);
    dlg_first = ent[0];
    dlg_count = ent[2];
    dlg_page = 0;
    ui_mode = M_TALK;
    box_open();
    dialog_show();
    sound_effect();
}

static void dialog_input(uint8_t pressed) {
    if (pressed & (J_A | J_B)) {
        dlg_page++;
        if (dlg_page >= dlg_count) {
            ui_mode = M_PLAY;
            box_close();
            if (after_dialog) {
                after_dialog = 0;
                menu_open(MENU_SHOP);
            }
        } else {
            dialog_show();
            sound_effect();
        }
    }
}

static void menu_draw(void) {
    uint8_t i;
    static const char *shop_items[3] = {"Sell all crops", "Pig treat   4g", "Hen feed    4g"};
    static const char *bed_items[3] = {"Sleep til morning", "Not yet", ""};
    static const char *msgs[7] = {"B-leave", "Sold!", "Yum!", "Too poor", "No crops", "Full up", "Sweet dreams"};
    const char **items = menu_kind == MENU_SHOP ? shop_items : bed_items;
    box_clear_line(0);
    if (menu_kind == MENU_SHOP) {
        style_text(1, 0, "Gold", 1, 1);
        style_number(6, 0, gold, 2, 1, 1);
    } else {
        style_text(1, 0, "Rest?", 1, 1);
    }
    style_text(10, 0, msgs[menu_msg], 1, 1);
    for (i = 0; i < 3; i++) {
        box_clear_line((uint8_t)(1 + i));
        if (items[i][0]) {
            style_text(1, (uint8_t)(1 + i), i == menu_sel ? ">" : " ", 1, 1);
            style_text(3, (uint8_t)(1 + i), items[i], 1, 1);
        }
    }
}

static void menu_open(uint8_t kind) {
    ui_mode = M_MENU;
    menu_kind = kind;
    menu_sel = 0;
    menu_msg = 0;
    box_open();
    menu_draw();
    sound_effect();
}

static void gold_add(uint8_t amount) {
    gold = (uint8_t)(gold + amount > GOLD_MAX ? GOLD_MAX : gold + amount);
    hud_dirty |= 0x20;
}

static void sleep_now(void) {
    fade_out();
    if (hour >= 6) {
        day++;
    }
    hour = 6;
    minute = 0;
    minute_tick = 0;
    grow_left = PLOTS;
    if (gp_happy) gp_happy--;
    if (hen_happy) hen_happy--;
    light_calc();
    box_close();
    hud_sprites();
    fade_apply(0);
    fade_in();
    ui_mode = M_PLAY;
    dialog_open(DLG_SLEEP);
}

static void menu_action(void) {
    uint8_t k, total;
    if (menu_kind == MENU_BED) {
        if (menu_sel == 0) {
            sleep_now();
        } else {
            ui_mode = M_PLAY;
            box_close();
        }
        return;
    }
    if (menu_sel == 0) {
        total = 0;
        for (k = 0; k < 5; k++) {
            total = (uint8_t)(total + crops[k] * sell_price[k]);
            crops[k] = 0;
        }
        if (!total) {
            menu_msg = 4;
        } else {
            gold_add(total);
            menu_msg = 1;
            hud_counts();
        }
        hud_gold();
        hud_dirty &= (uint8_t)~0x3F;
    } else if (menu_sel == 1) {
        if (gp_happy >= 5) {
            menu_msg = 5;
        } else if (gold < 4) {
            menu_msg = 3;
        } else {
            gold -= 4;
            gp_happy++;
            menu_msg = 2;
            hud_hearts();
            hud_gold();
        }
    } else {
        if (hen_happy >= 5) {
            menu_msg = 5;
        } else if (gold < 4) {
            menu_msg = 3;
        } else {
            gold -= 4;
            hen_happy++;
            menu_msg = 2;
            hud_hearts();
            hud_gold();
        }
    }
    menu_draw();
}

static void menu_input(uint8_t pressed) {
    if (pressed & J_B) {
        ui_mode = M_PLAY;
        box_close();
    } else if (pressed & J_UP) {
        if (menu_sel > 0) {
            menu_sel--;
            menu_msg = 0;
            menu_draw();
        }
    } else if (pressed & J_DOWN) {
        if (menu_sel < (menu_kind == MENU_SHOP ? 2 : 1)) {
            menu_sel++;
            menu_msg = 0;
            menu_draw();
        }
    } else if (pressed & J_A) {
        menu_action();
    }
}

/* ====================================================================== talking and looking */

static void talk_npc(npc_t *n) {
    n->dir = (uint8_t)(hx < n->x ? DIR_LEFT : DIR_RIGHT);
    n->moving = 0;
    n->timer = 60;
    switch (n->role) {
    case R_PEARL:
        if (pearl_talks == 0) {
            dialog_open(DLG_PEARL_INTRO);
        } else {
            dialog_open((uint8_t)(DLG_PEARL_T1 + (pearl_talks - 1) % 7));
        }
        pearl_talks++;
        break;
    case R_SHOP:
        dialog_open(DLG_HOBBS_HELLO);
        after_dialog = 1;
        break;
    case R_DAISY:
        if (quest[0] == 0) {
            quest[0] = 1;
            dialog_open(DLG_DAISY_ASK);
        } else if (quest[0] == 1 && crops[CUKE] >= 3) {
            crops[CUKE] -= 3;
            hud_dirty |= (uint8_t)(1 << CUKE);
            gold_add(12);
            quest[0] = 2;
            dialog_open(DLG_DAISY_DONE);
        } else {
            dialog_open(quest[0] == 1 ? DLG_DAISY_WAIT : DLG_DAISY_AFTER);
        }
        break;
    case R_WENDELL:
        if (quest[1] == 0) {
            quest[1] = 1;
            dialog_open(DLG_WENDELL_ASK);
        } else if (quest[1] == 1 && crops[CORN] >= 4) {
            crops[CORN] -= 4;
            hud_dirty |= (uint8_t)(1 << CORN);
            gold_add(15);
            quest[1] = 2;
            dialog_open(DLG_WENDELL_DONE);
        } else {
            dialog_open(quest[1] == 1 ? DLG_WENDELL_WAIT : DLG_WENDELL_AFTER);
        }
        break;
    default:
        dialog_open((uint8_t)(DLG_GUS_T1 + gus_talks % 5));
        gus_talks++;
        break;
    }
}

/* returns a section to switch to, or 0 */
static uint8_t interact(void) {
    uint8_t i, tx, ty;
    int16_t px = (int16_t)hx + 4;
    int16_t py = (int16_t)hy + 5;
    uint8_t *h;
    npc_t *n;
    switch (hdir) {
    case DIR_LEFT: px -= 8; break;
    case DIR_RIGHT: px += 8; break;
    case DIR_UP: py -= 8; break;
    default: py += 8; break;
    }
    if (px < 0 || py < 0 || px >= 256 || py >= 256) {
        return 0;
    }
    tx = (uint8_t)(px >> 3);
    ty = (uint8_t)(py >> 3);
    for (i = 0, h = hots[0]; i < nhot; i++, h += 6) {
        if (tx >= h[0] && tx <= h[2] && ty >= h[1] && ty <= h[3]) {
            switch (h[4]) {
            case HS_ARCADE:
                sound_effect();
                return SECTION_BRAWLER;
            case HS_HOCKEY:
                sound_effect();
                return SECTION_AIR_HOCKEY;
            case HS_BED:
                menu_open(MENU_BED);
                return 255;
            case HS_SHOP:
                menu_open(MENU_SHOP);
                return 255;
            default:
                dialog_open(h[5]);
                return 255;
            }
        }
    }
    if (area != AREA_FARM) {
        for (i = 0, n = npcs; i < nnpc; i++, n++) {
            if ((uint8_t)((uint8_t)px - n->x + 3) < 14 && (uint8_t)((uint8_t)py - n->y + 3) < 14) {
                talk_npc(n);
                return 255;
            }
        }
    }
    return 0;
}

static uint8_t check_exits(void) {
    uint8_t i, cx = (uint8_t)((hx + 4) >> 3), cy = (uint8_t)((hy + 4) >> 3);
    uint8_t *e;
    for (i = 0, e = exits[0]; i < nexit; i++, e += 8) {
        if (cx >= e[0] && cx <= e[2] && cy >= e[1] && cy <= e[3]) {
            go(e[4], e[5], e[6], e[7]);
            return 1;
        }
    }
    return 0;
}

/* ====================================================================== entering and the frame loop */

static void seed(void) {
    uint8_t i;
    for (i = 0; i < PLOTS; i++) {
        plot[i] = RIPE;
    }
    plot[FIELD_PLOTS + 3] = 1 | WET;
    plot[FIELD_PLOTS + 10] = 0;
    crops[CUKE] = 4;
    crops[PEPPER] = 3;
    crops[LETTUCE] = 4;
    crops[CORN] = 2;
    crops[HAY] = 6;
    gp_happy = 4;
    hen_happy = 4;
    gold = 0;
    day = 3;
    hour = 10;
    minute = 24;
    grow_left = 0;
    minute_tick = 0;
    active = 0;
    harvested = 0;
    shots = 0;
    kills = 0;
    area = AREA_FARM;
    hx = 72;
    hy = 56;
    hdir = DIR_DOWN;
    for (i = 0; i < PIGS; i++) {
        pigs[i].x = pigs[i].tx = (uint8_t)(26 + i * 10);
        pigs[i].y = pigs[i].ty = (uint8_t)(38 + i * 6);
        pigs[i].timer = (uint8_t)(20 + i * 25);
        pigs[i].state = i & 1;
    }
    for (i = 0; i < HENS; i++) {
        hens[i].x = hens[i].tx = (uint8_t)(146 + i * 5);
        hens[i].y = hens[i].ty = (uint8_t)(50 + i * 10);
        hens[i].timer = (uint8_t)(10 + i * 20);
        hens[i].state = 0;
    }
    farm_animals_init();
    ready = 1;
}

void town_enter(void) {
    DISPLAY_OFF;
    if (!ready) {
        seed();
    }
    frame = 0;
    ui_mode = M_PLAY;
    hud_y = 0;
    rt_scene_enter();
    style_common();
    area_load();
    set_sprite_palette(0, 8, farm_spr_pal);
    light_calc();
    set_light();
    hud_draw();
    move_win(7, 128);
    SHOW_WIN;
    camera_snap();
    move_bkg(view_x, view_y);
    hmoving = 0;
    farm_creatures_reset();
    plots_redraw();
    draw();
    SHOW_SPRITES;
    SHOW_BKG;
    DISPLAY_ON;
    report();
}

#ifdef HOST_TEST
void town_debug_goto(uint8_t id, uint8_t tx, uint8_t ty) {
    go(id, tx, ty, DIR_DOWN);
}
uint8_t town_debug_area(void) {
    return area;
}
void town_debug_state(uint8_t *out) {
    uint8_t i;
    out[0] = area; out[1] = hx; out[2] = hy; out[3] = view_x; out[4] = view_y; out[5] = gold;
    out[6] = day; out[7] = hour; out[8] = ui_mode; out[9] = quest[0]; out[10] = quest[1];
    memcpy(out + 11, crops, 5);
    for (i = 0; i < nnpc && i < 4; i++) {
        out[16 + i * 2] = npcs[i].x;
        out[17 + i * 2] = npcs[i].y;
    }
}
void town_debug_give(uint8_t kind, uint8_t n) {
    crops[kind] = n;
}
void town_debug_time(uint8_t h) {
    hour = h;
    minute = 0;
    minute_tick = 0;
    light_calc();
    set_light();
    hud_clock();
}
void town_debug_npc(uint8_t i) {
    if (i < nnpc) {
        hx = (uint8_t)(npcs[i].x - 6);
        hy = npcs[i].y;
        hdir = DIR_RIGHT;
        camera_snap();
    }
}
#endif

uint8_t town_get_day(void) {
    return day;
}

uint8_t town_get_happy(uint8_t species) {
    return species == ANIMAL_CHICKEN ? hen_happy : gp_happy;
}

static uint8_t play_frame(uint8_t keys, uint8_t pressed) {
    uint8_t mx, my, r;
    hero_move(keys);
    camera();
    if (check_exits()) {
        return SECTION_TOWN;
    }
    mx = hx;
    my = hy;
    if (pressed & J_START) {
        sound_effect();
        return SECTION_BOOK;
    }
    if (area == AREA_FARM && (pressed & J_SELECT)) {
        uint8_t ox = buddy.x, oy = buddy.y;
        buddy.x = mx;
        buddy.y = my;
        buddy.timer = 30;
        hx = ox;
        hy = oy;
        active ^= 1;
        buddy_pick();
        mx = hx;
        my = hy;
    }
    if (pressed & J_A) {
        r = interact();
        if (r == 255) {
            return SECTION_TOWN;
        }
        if (r) {
            return r;
        }
        if (area == AREA_FARM) {
            if (!harvest_at((uint8_t)(mx + 4), (uint8_t)(my + 4))) {
                if (mx < 80 && my >= 16 && my < 76) {
                    feed_pigs();
                } else if (mx >= 128 && my >= 40 && my < 88) {
                    feed_hens();
                }
            }
            spray_wait = 0;
        }
    }
    if (area == AREA_FARM) {
        if (keys & J_A) {
            if (spray_wait) {
                spray_wait--;
            } else {
                spray((uint8_t)(mx + 4), my, hdir);
                spray_wait = 12;
            }
        }
        drops_update();
        if (frame & 1) {
            crows_update(mx, my);
            buddy_update();
        } else {
            pigs_update();
            hens_update();
            clock_update();
        }
    } else {
        if (frame & 1) {
            npcs_update();
        } else {
            clock_update();
        }
    }
    return SECTION_TOWN;
}

uint8_t town_frame(uint8_t keys, uint8_t prev) {
    uint8_t pressed = (uint8_t)(keys & ~prev);
    uint8_t next = SECTION_TOWN;
    if (ui_mode == M_TALK) {
        dialog_input(pressed);
    } else if (ui_mode == M_MENU) {
        menu_input(pressed);
    } else {
        next = play_frame(keys, pressed);
        if (next == SECTION_TOWN) {
            if (area == AREA_FARM && (pressed & J_A)) {
                sound_effect();
            } else if (redraws) {
                plot_draw(redraw[--redraws]);
            } else if (hud_dirty && !(frame & 1) && ui_mode == M_PLAY) {
                uint8_t k = 0;
                while (!(hud_dirty & (1 << k))) {
                    k++;
                }
                hud_dirty &= (uint8_t)~(1 << k);
                if (k == 5) {
                    hud_gold();
                } else {
                    hud_count(k);
                }
            }
        }
    }
    sound_music(frame);
    frame++;
    if (ui_mode == M_PLAY && next == SECTION_TOWN) {
        move_bkg(view_x, view_y);
        if (LY_REG < 62 || LY_REG >= 144) {
            draw();
        }
    }
    report();
    return next;
}
