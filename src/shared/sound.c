/* Built with love and passion on retroable.dev */

#include <gb/gb.h>
#include <stdint.h>

#include "art.h"
#include "shared/sound.h"

static const uint16_t notes[36] = {1046, 1102, 1155, 1205, 1253, 1297, 1339, 1379, 1417, 1452, 1486, 1517,
                                   1547, 1575, 1602, 1627, 1650, 1673, 1694, 1714, 1732, 1750, 1767, 1783,
                                   1798, 1812, 1825, 1837, 1849, 1860, 1871, 1881, 1890, 1899, 1907, 1915};
static uint8_t wait;
static uint8_t step;

void sound_boot(void) {
    NR52_REG = 0x80;
    NR50_REG = 0x77;
    NR51_REG = 0xFF;
}

void sound_music(uint8_t frame) {
    uint8_t note;
    uint16_t value;
    (void)frame;
    if (wait) {
        wait--;
        return;
    }
    wait = MUSIC_STEP_FRAMES - 1;
    note = music_notes[step];
    step++;
    if (step >= MUSIC_STEPS) {
        step = 0;
    }
    if (note == 0 || note > 36) {
        return;
    }
    value = notes[note - 1];
    NR21_REG = 0x80;
    NR22_REG = 0x63;
    NR23_REG = (uint8_t)(value & 0xFF);
    NR24_REG = (uint8_t)(0x80 | (value >> 8));
}

void sound_effect(void) {
    NR10_REG = 0x00;
    NR11_REG = 0x80;
    NR12_REG = 0xF0;
    NR13_REG = 0x00;
    NR14_REG = 0x87;
    NR41_REG = 0x00;
    NR42_REG = 0xF1;
    NR43_REG = 0x2F;
    NR44_REG = 0x80;
}

void sound_wheek(void) {
    NR10_REG = 0x16;
    NR11_REG = 0x40;
    NR12_REG = 0xF2;
    NR13_REG = 0x80;
    NR14_REG = 0x87;
}

void sound_hit(void) {
    NR41_REG = 0x00;
    NR42_REG = 0xF2;
    NR43_REG = 0x55;
    NR44_REG = 0x80;
}
