#ifndef SHARED_STYLE_H
#define SHARED_STYLE_H
#include <stdint.h>
void style_boot(void);
void style_paint(void);
void style_fill(uint8_t tile);
void style_scene(const uint8_t *tiles, uint8_t count);
void style_common(void);
uint8_t style_glyph(char c);
void style_text_n(uint8_t x, uint8_t y, const char *text, uint8_t n, uint8_t pal, uint8_t win);
void style_map(uint8_t x, uint8_t y, uint8_t w, uint8_t h, const uint8_t *map, const uint8_t *attr);
void style_cell(uint8_t x, uint8_t y, uint8_t tile, uint8_t attr, uint8_t win);
void style_text(uint8_t x, uint8_t y, const char *text, uint8_t pal, uint8_t win);
void style_number(uint8_t x, uint8_t y, uint8_t value, uint8_t digits, uint8_t pal, uint8_t win);
uint8_t style_sprite(uint8_t slot, uint8_t x, uint8_t y, uint8_t tile, uint8_t prop);
uint8_t style_wide(uint8_t slot, uint8_t x, uint8_t y, uint8_t left, uint8_t right, uint8_t prop);
void style_hide(uint8_t slot);
#endif
