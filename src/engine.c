#include <gb/gb.h>
#include <stdint.h>

#include "rt/rt.h"

static uint16_t rng = 0xACE1;

void rt_seed(uint8_t extra) {
    rng ^= (uint16_t)((uint16_t)extra << 8 | extra);
    if (!rng) {
        rng = 0xACE1;
    }
}

uint8_t rt_random(void) {
    rng ^= (uint16_t)(rng << 7);
    rng ^= (uint16_t)(rng >> 9);
    rng ^= (uint16_t)(rng << 8);
    return (uint8_t)(rng >> 3);
}

void rt_scene_enter(void) {
    move_bkg(0, 0);
}
