/* Host-side stand-in for GBDK-2020 <gb/gb.h>. Only what this project needs. */
#ifndef MOCK_GB_H
#define MOCK_GB_H
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define NONBANKED
#define BANKED
#define OLDCALL
#define NORETURN
#define CRITICAL
#define __critical

#define J_RIGHT 0x01
#define J_LEFT 0x02
#define J_UP 0x04
#define J_DOWN 0x08
#define J_A 0x10
#define J_B 0x20
#define J_SELECT 0x40
#define J_START 0x80

#define S_PRIORITY 0x80
#define S_FLIPY 0x40
#define S_FLIPX 0x20
#define S_PALETTE 0x10

typedef struct { uint8_t y, x, tile, prop; } OAM_item_t;
extern OAM_item_t shadow_OAM[40];

/* hardware state (mock) */
extern uint8_t g_vram[2][0x2000];
extern uint8_t g_vbk, g_lcdc, g_scx, g_scy, g_wx, g_wy, g_ly, g_bgp, g_obp0, g_obp1, g_stat;
extern uint8_t g_regs[256];
extern uint16_t g_bgpal[32], g_objpal[32];
extern volatile uint16_t sys_time;
extern uint8_t _cpu;
#define CGB_TYPE 0x11
#define DMG_TYPE 0x01

#define VBK_REG g_vbk
#define LCDC_REG g_lcdc
#define SCX_REG g_scx
#define SCY_REG g_scy
#define WX_REG g_wx
#define WY_REG g_wy
#define LY_REG g_ly
#define STAT_REG g_stat
#define BGP_REG g_bgp
#define OBP0_REG g_obp0
#define OBP1_REG g_obp1
#define NR10_REG g_regs[0x10]
#define NR11_REG g_regs[0x11]
#define NR12_REG g_regs[0x12]
#define NR13_REG g_regs[0x13]
#define NR14_REG g_regs[0x14]
#define NR21_REG g_regs[0x16]
#define NR22_REG g_regs[0x17]
#define NR23_REG g_regs[0x18]
#define NR24_REG g_regs[0x19]
#define NR30_REG g_regs[0x1A]
#define NR31_REG g_regs[0x1B]
#define NR32_REG g_regs[0x1C]
#define NR33_REG g_regs[0x1D]
#define NR34_REG g_regs[0x1E]
#define NR41_REG g_regs[0x20]
#define NR42_REG g_regs[0x21]
#define NR43_REG g_regs[0x22]
#define NR44_REG g_regs[0x23]
#define NR50_REG g_regs[0x24]
#define NR51_REG g_regs[0x25]
#define NR52_REG g_regs[0x26]

#define LCDCF_ON 0x80
#define LCDCF_WINON 0x20
#define LCDCF_BGON 0x01
#define LCDCF_OBJON 0x02
#define LCDCF_BG8000 0x10
#define DISPLAY_ON (g_lcdc |= 0x80)
#define DISPLAY_OFF (g_lcdc &= (uint8_t)~0x80)
#define SHOW_BKG (g_lcdc |= 0x01)
#define HIDE_BKG (g_lcdc &= (uint8_t)~0x01)
#define SHOW_WIN (g_lcdc |= 0x20)
#define HIDE_WIN (g_lcdc &= (uint8_t)~0x20)
#define SHOW_SPRITES (g_lcdc |= 0x02)
#define HIDE_SPRITES (g_lcdc &= (uint8_t)~0x02)

uint8_t joypad(void);
void vsync(void);
void wait_vbl_done(void);
void delay(uint16_t ms);
void cpu_fast(void);
void disable_interrupts(void);
void enable_interrupts(void);

void set_bkg_data(uint8_t first, uint8_t n, const uint8_t *data);
void set_sprite_data(uint8_t first, uint8_t n, const uint8_t *data);
void set_data(uint8_t *vram_addr, const uint8_t *data, uint16_t len);
void set_bkg_tiles(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *t);
void set_win_tiles(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *t);
void set_bkg_tile_xy(uint8_t x, uint8_t y, uint8_t t);
void set_win_tile_xy(uint8_t x, uint8_t y, uint8_t t);
uint8_t get_bkg_tile_xy(uint8_t x, uint8_t y);
void set_sprite_prop(uint8_t nb, uint8_t prop);
void move_bkg(uint8_t x, uint8_t y);
void scroll_bkg(int8_t dx, int8_t dy);
void move_win(uint8_t x, uint8_t y);
void move_sprite(uint8_t nb, uint8_t x, uint8_t y);
#endif
