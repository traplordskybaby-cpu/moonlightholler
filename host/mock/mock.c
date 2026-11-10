#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdio.h>
#include <string.h>

OAM_item_t shadow_OAM[40];
uint8_t g_vram[2][0x2000];
uint8_t g_vbk, g_lcdc = 0xE3 & 0x7F, g_scx, g_scy, g_wx, g_wy, g_ly, g_bgp, g_obp0, g_obp1, g_stat;
uint8_t g_regs[256];
uint16_t g_bgpal[32], g_objpal[32];
volatile uint16_t sys_time;
uint8_t _cpu = CGB_TYPE;

/* ---- test driver state ---- */
static uint8_t cur_keys;
static unsigned frame_no;
static unsigned max_frames = 600;
static char script[8192];
static const char *spos;
static int hold_left;
static uint8_t hold_keys;
static unsigned dumps[64];
static int ndumps;
static const char *prefix = "out";
unsigned g_frame(void) { return frame_no; }
extern void town_debug_goto(uint8_t id, uint8_t tx, uint8_t ty);
extern void town_debug_state(uint8_t *out);
extern void town_debug_give(uint8_t kind, uint8_t n);
extern void town_debug_npc(uint8_t i);
extern void town_debug_time(uint8_t h);
static char cmds[2048];
void (*harness_hook)(unsigned frame) = 0;
static unsigned rngseed = 1;

static uint8_t key_of(char c) {
    switch (c) {
    case 'R': return J_RIGHT; case 'L': return J_LEFT; case 'U': return J_UP; case 'D': return J_DOWN;
    case 'A': return J_A; case 'B': return J_B; case 'S': return J_START; case 'E': return J_SELECT;
    }
    return 0;
}
static void next_token(void) {
    while (*spos == ' ') spos++;
    if (!*spos) { hold_keys = 0; hold_left = 1 << 20; return; }
    hold_keys = 0;
    while (*spos && *spos != ':' && *spos != ' ') { hold_keys |= key_of(*spos); spos++; }
    hold_left = 1;
    if (*spos == ':') { spos++; hold_left = atoi(spos); while (*spos >= '0' && *spos <= '9') spos++; }
}
static void dump(const char *tag) {
    char name[256];
    snprintf(name, sizeof name, "%s_%s.bin", prefix, tag);
    FILE *f = fopen(name, "wb");
    if (!f) return;
    fwrite(g_vram[0], 1, 0x2000, f); fwrite(g_vram[1], 1, 0x2000, f);
    fwrite(shadow_OAM, 1, 160, f);
    fwrite(g_bgpal, 2, 32, f); fwrite(g_objpal, 2, 32, f);
    uint8_t regs[8] = {g_lcdc, g_scx, g_scy, g_wx, g_wy, g_bgp, g_obp0, g_obp1};
    fwrite(regs, 1, 8, f);
    fclose(f);
    {
        uint8_t st[40] = {0};
        town_debug_state(st);
        snprintf(name, sizeof name, "%s_%s.txt", prefix, tag);
        f = fopen(name, "w");
        if (f) {
            fprintf(f, "area=%u hx=%u hy=%u view=%u,%u gold=%u day=%u hour=%u mode=%u quest=%u,%u crops=%u,%u,%u,%u,%u\n",
                    st[0], st[1], st[2], st[3], st[4], st[5], st[6], st[7], st[8], st[9], st[10], st[11], st[12], st[13], st[14], st[15]);
            fprintf(f, "npc0=%u,%u npc1=%u,%u\n", st[16], st[17], st[18], st[19]);
            fclose(f);
        }
    }
}
void harness_dump(const char *tag) { dump(tag); }

uint8_t joypad(void) { return cur_keys; }
void vsync(void) {
    int i;
    frame_no++; sys_time++;
    for (i = 0; i < ndumps; i++) if (dumps[i] == frame_no) { char t[32]; snprintf(t, sizeof t, "f%u", frame_no); dump(t); }
    if (cmds[0]) {
        /* "frame:goto:area:tx:ty;frame:give:kind:n" */
        char *c = strdup(cmds), *save, *tok = strtok_r(c, ";", &save);
        while (tok) {
            unsigned f, a, b, d; char verb[16];
            if (sscanf(tok, "%u:%15[a-z]:%u:%u:%u", &f, verb, &a, &b, &d) >= 4 && f == frame_no) {
                if (!strcmp(verb, "goto")) town_debug_goto(a, b, d);
                if (!strcmp(verb, "give")) town_debug_give(a, b);
                if (!strcmp(verb, "npc")) town_debug_npc(a);
                if (!strcmp(verb, "time")) town_debug_time(a);
            }
            tok = strtok_r(0, ";", &save);
        }
        free(c);
    }
    if (harness_hook) harness_hook(frame_no);
    if (frame_no >= max_frames) { dump("final"); exit(0); }
    if (--hold_left <= 0) next_token();
    cur_keys = hold_keys;
}
void wait_vbl_done(void) { vsync(); }
void delay(uint16_t ms) { (void)ms; }
void cpu_fast(void) {}
void disable_interrupts(void) {}
void enable_interrupts(void) {}

static uint8_t *tile_addr(uint8_t idx, uint8_t bank, int bkg) {
    /* GBDK: bkg tiles <128 live at 0x9000, >=128 at 0x8800; sprites at 0x8000 */
    uint16_t base = bkg ? (idx < 128 ? 0x1000 + idx * 16 : 0x0800 + (idx - 128) * 16) : idx * 16;
    return &g_vram[bank][base];
}
void set_bkg_data(uint8_t first, uint8_t n, const uint8_t *d) {
    uint16_t i;
    for (i = 0; i < n; i++) memcpy(tile_addr((uint8_t)(first + i), g_vbk, 1), d + i * 16, 16);
}
void set_sprite_data(uint8_t first, uint8_t n, const uint8_t *d) {
    uint16_t i;
    for (i = 0; i < n; i++) memcpy(tile_addr((uint8_t)(first + i), g_vbk, 0), d + i * 16, 16);
}
void set_data(uint8_t *a, const uint8_t *d, uint16_t len) {
    uintptr_t v = (uintptr_t)a;
    if (v >= 0x8000 && v < 0xA000) memcpy(&g_vram[g_vbk][v - 0x8000], d, len);
}
static void tiles(uint16_t base, uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *t) {
    uint8_t i, j;
    for (j = 0; j < h; j++)
        for (i = 0; i < w; i++)
            g_vram[g_vbk][base + (uint8_t)((y + j) & 31) * 32 + ((x + i) & 31)] = t[j * w + i];
}
void set_bkg_tiles(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *t) { tiles(0x1800, x, y, w, h, t); }
void set_win_tiles(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *t) { tiles(0x1C00, x, y, w, h, t); }
void set_bkg_tile_xy(uint8_t x, uint8_t y, uint8_t t) { tiles(0x1800, x, y, 1, 1, &t); }
void set_win_tile_xy(uint8_t x, uint8_t y, uint8_t t) { tiles(0x1C00, x, y, 1, 1, &t); }
uint8_t get_bkg_tile_xy(uint8_t x, uint8_t y) { return g_vram[g_vbk][0x1800 + (y & 31) * 32 + (x & 31)]; }
void set_sprite_prop(uint8_t nb, uint8_t prop) { shadow_OAM[nb].prop = prop; }
void move_bkg(uint8_t x, uint8_t y) { g_scx = x; g_scy = y; }
void scroll_bkg(int8_t dx, int8_t dy) { g_scx = (uint8_t)(g_scx + dx); g_scy = (uint8_t)(g_scy + dy); }
void move_win(uint8_t x, uint8_t y) { g_wx = x; g_wy = y; }
void move_sprite(uint8_t nb, uint8_t x, uint8_t y) { shadow_OAM[nb].x = x; shadow_OAM[nb].y = y; }
void set_bkg_palette(uint8_t first, uint8_t n, const palette_color_t *p) { memcpy(&g_bgpal[first * 4], p, n * 8); }
void set_sprite_palette(uint8_t first, uint8_t n, const palette_color_t *p) { memcpy(&g_objpal[first * 4], p, n * 8); }

/* usage: game_host PREFIX FRAMES "script" dumpframe... */
__attribute__((constructor)) static void ctor(void) {
    /* args are passed through env vars so game main() stays untouched */
    const char *p = getenv("GB_PREFIX"); if (p) prefix = p;
    const char *m = getenv("GB_FRAMES"); if (m) max_frames = (unsigned)atoi(m);
    const char *s = getenv("GB_KEYS"); strncpy(script, s ? s : "", sizeof script - 1);
    spos = script; next_token(); cur_keys = hold_keys;
    const char *cm = getenv("GB_CMDS"); if (cm) strncpy(cmds, cm, sizeof cmds - 1);
    const char *d = getenv("GB_DUMPS");
    if (d) { char *q = strdup(d), *t = strtok(q, ","); while (t && ndumps < 64) { dumps[ndumps++] = (unsigned)atoi(t); t = strtok(0, ","); } }
}
