#ifndef BANK_H
#define BANK_H
#include <gb/gb.h>
#include <stdint.h>

/*
 * Bulk art, maps and text live in ROM banks 2 and up (the cartridge is an MBC5).  The game code lives in
 * the always-visible ROM (banks 0 and 1), so it can never be switched away while it is running.  Data is
 * pulled out through these helpers, which copy a few bytes at a time into RAM.
 */
void bk_copy(void *dst, uint8_t bank, uint16_t addr, uint16_t n) NONBANKED;
const void *bk_get(uint8_t bank, uint16_t addr, uint16_t size);
void bk_vram(uint8_t *vram, uint8_t bank, uint16_t addr, uint16_t n);
#endif
