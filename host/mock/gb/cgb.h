#ifndef MOCK_CGB_H
#define MOCK_CGB_H
#include <stdint.h>
typedef uint16_t palette_color_t;
#define RGB(r, g, b) ((uint16_t)(((r) & 31) | (((g) & 31) << 5) | (((b) & 31) << 10)))
void set_bkg_palette(uint8_t first, uint8_t n, const palette_color_t *p);
void set_sprite_palette(uint8_t first, uint8_t n, const palette_color_t *p);
#endif
