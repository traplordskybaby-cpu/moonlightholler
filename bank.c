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
/*
 * The cartridge has 4 banks: the code fills banks 0 and 1 (0x0000-0x7FFF) and the art/maps/text sit in banks 2 and 3.
 * Reading a data bank means swapping it into 0x4000-0x7FFF, which also removes the second half of the code.  The
 * code doing the swap therefore cannot live in that half (the CPU would fetch its next instruction from the wrong
 * bank), and the linker puts everything above 16 KB there, including any "NONBANKED" function.  So the copy loop is
 * a tiny machine-code routine that is built in RAM at start-up and runs from there.
 */
static volatile uint8_t bk_bank;
static volatile uint16_t bk_src;
static volatile uint16_t bk_dst;
static volatile uint16_t bk_len;
static uint8_t bk_code[50];
static uint8_t bk_ready;

static void bk_at(uint8_t i, uint16_t a) {
    bk_code[i] = (uint8_t)a;
    bk_code[(uint8_t)(i + 1)] = (uint8_t)(a >> 8);
}

static void bk_build(void) {
    uint8_t *c = bk_code;
    c[0] = 0xC5;  /* push bc */
    c[1] = 0xD5;  /* push de */
    c[2] = 0xFA;  /* ld a,(bank) */
    bk_at(3, (uint16_t)&bk_bank);
    c[5] = 0xEA;  /* ld (0x2000),a   select the data bank */
    c[6] = 0x00;
    c[7] = 0x20;
    c[8] = 0xFA;  /* ld a,(src.lo) ; ld l,a ; ld a,(src.hi) ; ld h,a */
    bk_at(9, (uint16_t)&bk_src);
    c[11] = 0x6F;
    c[12] = 0xFA;
    bk_at(13, (uint16_t)&bk_src + 1);
    c[15] = 0x67;
    c[16] = 0xFA; /* dst into de */
    bk_at(17, (uint16_t)&bk_dst);
    c[19] = 0x5F;
    c[20] = 0xFA;
    bk_at(21, (uint16_t)&bk_dst + 1);
    c[23] = 0x57;
    c[24] = 0xFA; /* length into bc */
    bk_at(25, (uint16_t)&bk_len);
    c[27] = 0x4F;
    c[28] = 0xFA;
    bk_at(29, (uint16_t)&bk_len + 1);
    c[31] = 0x47;
    c[32] = 0x78; /* loop: ld a,b */
    c[33] = 0xB1; /* or c */
    c[34] = 0x28; /* jr z,done */
    c[35] = 0x06;
    c[36] = 0x2A; /* ld a,(hl+) */
    c[37] = 0x12; /* ld (de),a */
    c[38] = 0x13; /* inc de */
    c[39] = 0x0B; /* dec bc */
    c[40] = 0x18; /* jr loop */
    c[41] = 0xF6;
    c[42] = 0x3E; /* done: ld a,1 */
    c[43] = 0x01;
    c[44] = 0xEA; /* ld (0x2000),a   code bank back */
    c[45] = 0x00;
    c[46] = 0x20;
    c[47] = 0xD1; /* pop de */
    c[48] = 0xC1; /* pop bc */
    c[49] = 0xC9; /* ret */
    bk_ready = 1;
}

void bk_copy(void *dst, uint8_t bank, uint16_t addr, uint16_t n) {
    if (!bk_ready) {
        bk_build();
    }
    bk_bank = bank;
    bk_src = addr;
    bk_dst = (uint16_t)dst;
    bk_len = n;
    disable_interrupts();
    ((void (*)(void))bk_code)();
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
