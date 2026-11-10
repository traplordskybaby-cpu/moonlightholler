#include <gb/gb.h>
#include <stdint.h>
#include <string.h>

#include "bank.h"

#ifdef HOST_TEST
/* desktop test build: pretend the data banks are a file */
#include <stdio.h>
static uint8_t *host_banks;
void bk_copy(void *dst, uint8_t bank, uint16_t addr, uint16_t n) {
    if (!host_banks) {
        const char *path = getenv("GB_DATABANK");
        FILE *f = fopen(path ? path : "build/databank.bin", "rb");
        host_banks = calloc(1, 1 << 20);
        if (f) {
            fread(host_banks, 1, 1 << 20, f);
            fclose(f);
        }
    }
    memcpy(dst, host_banks + (uint32_t)(bank - 2) * 0x4000 + (addr - 0x4000), n);
}
#else
/* Must sit in the home bank: it switches the 0x4000-0x7FFF window away from the code that called it. */
void bk_copy(void *dst, uint8_t bank, uint16_t addr, uint16_t n) NONBANKED {
    uint8_t *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)addr;
    disable_interrupts();
    *(volatile uint8_t *)0x2000 = bank;
    while (n--) {
        *d++ = *s++;
    }
    *(volatile uint8_t *)0x2000 = 1;
    enable_interrupts();
}
#endif

#define STAGE_BIG 1280 /* the largest banked array is the air hockey tile set (1232 bytes) */
static uint8_t stage_big[STAGE_BIG];
static uint8_t stage_a[512];
static uint8_t stage_b[512];
static uint8_t flip;

/* Returns a RAM copy.  Two small arrays can be alive at once (map + attributes); one big one. */
const void *bk_get(uint8_t bank, uint16_t addr, uint16_t size) {
    uint8_t *buf;
    if (size > STAGE_BIG) {
        size = STAGE_BIG; /* never overrun the buffer */
    }
    if (size > 512) {
        buf = stage_big;
    } else {
        flip ^= 1;
        buf = flip ? stage_a : stage_b;
    }
    bk_copy(buf, bank, addr, size);
    return buf;
}

void bk_vram(uint8_t *vram, uint8_t bank, uint16_t addr, uint16_t n) {
    while (n) {
        uint16_t c = n > 256 ? 256 : n;
        bk_copy(stage_big, bank, addr, c);
        set_data(vram, stage_big, c);
        vram += c;
        addr += c;
        n -= c;
    }
}
