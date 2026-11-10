#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>

#include "art.h"
#include "probe.h"
#include "rt/rt.h"
#include "scenes/scenes.h"
#include "shared/sound.h"
#include "shared/style.h"
#include "title.h"

static uint8_t section;
static uint8_t prev;

static void enter(uint8_t which) {
    switch (which) {
    case SECTION_AIR_HOCKEY:
        section = SECTION_AIR_HOCKEY;
        air_hockey_enter();
        break;
    case SECTION_BRAWLER:
        section = SECTION_BRAWLER;
        brawler_enter();
        break;
    case SECTION_BOOK:
        section = SECTION_BOOK;
        book_enter();
        break;
    case SECTION_TOWN:
    default:
        section = SECTION_TOWN;
        town_enter();
        break;
    }
}

static uint8_t run(uint8_t keys) {
    switch (section) {
    case SECTION_AIR_HOCKEY:
        return air_hockey_frame(keys, prev);
    case SECTION_BRAWLER:
        return brawler_frame(keys, prev);
    case SECTION_BOOK:
        return book_frame(keys, prev);
    case SECTION_TOWN:
    default:
        return town_frame(keys, prev);
    }
}

void main(void) {
    uint8_t next, keys;
    if (_cpu == CGB_TYPE) {
        cpu_fast();
    }
#ifndef HOST_TEST
    *(volatile uint8_t *)0x2000 = 1; /* MBC5: make sure the code half of the cartridge is mapped */
#endif
    style_boot();
    sound_boot();
    title_run();
    prev = 0xFF; /* buttons still held from the title screen do not count as new presses */
    enter(SECTION_TOWN);
    forge_state = 1;
    forge_scene = 1;
    while (1) {
        keys = joypad();
        next = run(keys);
        if (next != section) {
            enter(next);
        }
        forge_section = section;
        prev = keys;
        if ((keys & J_A) && forge_interact < 255) {
            forge_interact++;
        }
        vsync();
    }
}
