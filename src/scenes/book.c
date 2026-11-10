/*
 * The Breeding Book: a record of every chicken and guinea pig on the farm.  Pick a hen and a rooster (or a
 * sow and a boar) to start a new generation.  Animals need to be well cared for, and each kind can be bred
 * once a day.  The genetics live in shared/breeding.c; this file is only the screen.
 */
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include <string.h>

#include "art.h"
#include "scenes/scenes.h"
#include "shared/farm_animals.h"
#include "shared/sound.h"
#include "shared/style.h"

#define NONE 255
#define MSG_NONE 0
#define MSG_BORN 1
#define MSG_NEED_PAIR 2
#define MSG_FULL 3
#define MSG_SECOND 4
#define MSG_CARE 5
#define MSG_TOMORROW 6

static uint8_t species;
static uint8_t cursor;
static uint8_t first;
static uint8_t msg;
static uint8_t last_day[2];

static const char *const chicken_traits[6] = {"Black feathers", "White feathers", "Dark skin",
                                                "Barred pattern", "Silkied feathers", "Large frame"};
static const char *const pig_traits[6] = {"Long hair", "Rosettes", "Teddy coat", "Satin coat", "Skinny (hairless)", "Large frame"};
static const char *const chicken_lines[5] = {"Ayam Cemani", "Zombie Cross", "Black Silkie", "Egyptian Fayoumi", "Barred Rock"};
static const char *const pig_lines[3] = {"Abyssinian", "Peruvian", "Teddy"};
static const char *const messages[7] = {"", "A new one is born!", "Pick one of each sex", "The book is full",
                                          "Pick the second one", "They need more care", "Try again tomorrow"};

static uint8_t put_text(char *buf, uint8_t at, const char *s) {
    while (*s && at < 20) {
        buf[at++] = *s++;
    }
    return at;
}

static uint8_t put_number(char *buf, uint8_t at, uint8_t v) {
    if (v >= 100 && at < 20) {
        buf[at++] = (char)('0' + v / 100);
    }
    if (v >= 10 && at < 20) {
        buf[at++] = (char)('0' + (v / 10) % 10);
    }
    if (at < 20) {
        buf[at++] = (char)('0' + v % 10);
    }
    return at;
}

static void line(uint8_t y, const char *text) {
    char buf[20];
    memset(buf, ' ', sizeof(buf));
    put_text(buf, 0, text);
    style_text_n(0, y, buf, 20, 1, 0);
}

static void draw_page(void) {
    char buf[20];
    uint8_t at, i, shown = 0;
    const animal_record_t *f;
    uint8_t count = farm_animals_count(species);
    const animal_record_t *a = count ? farm_animals_get(species, cursor) : 0;
    const char *const *names = species == ANIMAL_CHICKEN ? chicken_traits : pig_traits;

    line(0, "   BREEDING BOOK");
    memset(buf, ' ', sizeof(buf));
    at = put_text(buf, 0, species == ANIMAL_CHICKEN ? "Chickens  " : "Guinea pigs  ");
    at = put_number(buf, at, (uint8_t)(cursor + 1));
    at = put_text(buf, at, " of ");
    put_number(buf, at, count);
    style_text_n(0, 1, buf, 20, 1, 0);
    line(2, "");
    if (!a) {
        line(3, "No animals yet.");
        for (i = 4; i < 13; i++) {
            line(i, "");
        }
    } else {
        if (a->generation == 0) {
            line(3, species == ANIMAL_CHICKEN ? chicken_lines[a->line % 5] : pig_lines[a->line % 3]);
        } else {
            line(3, "Holler mix");
        }
        memset(buf, ' ', sizeof(buf));
        at = put_text(buf, 0, "ID ");
        at = put_number(buf, at, a->id);
        at = put_text(buf, at, "  ");
        if (species == ANIMAL_CHICKEN) {
            at = put_text(buf, at, a->sex == ANIMAL_ROOSTER ? "Rooster" : "Hen");
        } else {
            at = put_text(buf, at, a->sex == ANIMAL_MALE ? "Boar" : "Sow");
        }
        at = put_text(buf, at, "  Gen ");
        put_number(buf, at, a->generation);
        style_text_n(0, 4, buf, 20, 1, 0);
        if (a->generation == 0) {
            line(5, "Founder stock");
        } else {
            memset(buf, ' ', sizeof(buf));
            at = put_text(buf, 0, "Parents: ID ");
            at = put_number(buf, at, a->sire_id);
            at = put_text(buf, at, " + ID ");
            put_number(buf, at, a->dam_id);
            style_text_n(0, 5, buf, 20, 1, 0);
        }
        line(7, "Traits:");
        for (i = 0; i < 6; i++) {
            if (a->traits & (1 << i)) {
                line((uint8_t)(8 + shown), names[i]);
                shown++;
            }
        }
        for (i = shown; i < 6; i++) {
            line((uint8_t)(8 + i), i == 0 ? "(nothing special)" : "");
        }
    }
    if (first != NONE) {
        memset(buf, ' ', sizeof(buf));
        at = put_text(buf, 0, "First pick: ID ");
        f = farm_animals_get(species, first);
        put_number(buf, at, f ? f->id : 0);
        style_text_n(0, 15, buf, 20, 1, 0);
    } else {
        line(15, "");
    }
    line(14, "");
    line(16, messages[msg]);
    line(17, "A pick B clear Start");
}

void book_enter(void) {
    static const palette_color_t pal[4] = {RGB(18, 26, 31), RGB(31, 31, 31), RGB(28, 5, 4), RGB(3, 5, 14)};
    static const uint8_t blank[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    uint8_t row[20];
    uint8_t y;
    DISPLAY_OFF;
    HIDE_SPRITES;
    HIDE_WIN;
    move_bkg(0, 0);
    set_bkg_palette(1, 1, pal);
    VBK_REG = 1;
    set_bkg_data(128, FONT_TILE_COUNT, font_tiles);
    VBK_REG = 0;
    set_bkg_data(128, 1, blank);
    memset(row, 1, sizeof(row));
    for (y = 0; y < 18; y++) {
        VBK_REG = 1;
        set_bkg_tiles(0, y, 20, 1, row);
        VBK_REG = 0;
    }
    style_fill(128);
    species = ANIMAL_CHICKEN;
    cursor = 0;
    first = NONE;
    msg = MSG_NONE;
    draw_page();
    SHOW_BKG;
    DISPLAY_ON;
}

static void try_breed(void) {
    const animal_record_t *a;
    const animal_record_t *b;
    uint8_t count = farm_animals_count(species);
    if (first == NONE) {
        first = cursor;
        msg = MSG_SECOND;
        return;
    }
    a = farm_animals_get(species, first);
    b = farm_animals_get(species, cursor);
    if (!a || !b || a == b || a->sex == b->sex) {
        msg = MSG_NEED_PAIR;
    } else if (town_get_happy(species) < 3) {
        msg = MSG_CARE;
    } else if (last_day[species] == town_get_day()) {
        msg = MSG_TOMORROW;
    } else if (count >= FARM_ANIMAL_CAPACITY || !farm_animals_breed(species, first, cursor)) {
        msg = MSG_FULL;
    } else {
        msg = MSG_BORN;
        last_day[species] = town_get_day();
        cursor = (uint8_t)(farm_animals_count(species) - 1);
        sound_wheek();
    }
    first = NONE;
}

uint8_t book_frame(uint8_t keys, uint8_t prev) {
    uint8_t pressed = (uint8_t)(keys & ~prev);
    uint8_t count = farm_animals_count(species);
    uint8_t dirty = 0;
    if (pressed & J_START) {
        sound_effect();
        return SECTION_TOWN;
    }
    if (pressed & J_SELECT) {
        species ^= 1;
        cursor = 0;
        first = NONE;
        msg = MSG_NONE;
        dirty = 1;
    } else if (count && (pressed & J_UP)) {
        cursor = cursor ? (uint8_t)(cursor - 1) : (uint8_t)(count - 1);
        msg = MSG_NONE;
        dirty = 1;
    } else if (count && (pressed & J_DOWN)) {
        cursor = (uint8_t)((cursor + 1) % count);
        msg = MSG_NONE;
        dirty = 1;
    } else if (pressed & J_B) {
        first = NONE;
        msg = MSG_NONE;
        dirty = 1;
    } else if (count && (pressed & J_A)) {
        try_breed();
        dirty = 1;
    }
    if (dirty) {
        sound_effect();
        draw_page();
    }
    sound_music(0);
    return SECTION_BOOK;
}
