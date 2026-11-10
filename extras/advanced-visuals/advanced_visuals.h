#ifndef ADVANCED_VISUALS_H
#define ADVANCED_VISUALS_H
#include <stdint.h>

#define AV_VIEW_W 20
#define AV_VIEW_H 18
#define AV_MAP_W 16
#define AV_MAP_H 16

typedef struct {
    uint8_t distance;
    uint8_t texture;
    uint8_t side;
    uint8_t top;
    uint8_t bottom;
} av_ray_column_t;

/* Q8.8 fixed-point helpers. World positions use 8.8 tile coordinates. */
int16_t av_fx_lerp(int16_t current, int16_t target, uint8_t fraction);
void av_raycast(const uint8_t *map, int16_t px, int16_t py,
                uint8_t facing, av_ray_column_t *columns);
void av_heightfield(const uint8_t *heightmap, uint8_t offset,
                    uint8_t *screen_heights);
uint16_t av_rle_decode(const uint8_t *src, uint16_t src_len,
                       uint8_t *dst, uint16_t dst_len);

#endif
