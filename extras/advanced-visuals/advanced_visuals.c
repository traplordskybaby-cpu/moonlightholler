#include "advanced_visuals.h"

int16_t av_fx_lerp(int16_t current, int16_t target, uint8_t fraction) {
    int32_t delta = (int32_t)target - current;
    return (int16_t)(current + ((delta * fraction) >> 8));
}

static uint8_t wall_at(const uint8_t *map, int16_t x, int16_t y) {
    uint8_t tx = (uint8_t)(x >> 8);
    uint8_t ty = (uint8_t)(y >> 8);
    if (tx >= AV_MAP_W || ty >= AV_MAP_H) return 1;
    return map[(uint16_t)ty * AV_MAP_W + tx];
}

void av_raycast(const uint8_t *map, int16_t px, int16_t py,
                uint8_t facing, av_ray_column_t *columns) {
    static const int8_t dx[4] = { 0, 1, 0, -1 };
    static const int8_t dy[4] = { -1, 0, 1, 0 };
    static const int8_t plane_x[4] = { 1, 0, -1, 0 };
    static const int8_t plane_y[4] = { 0, 1, 0, -1 };
    uint8_t col;
    facing &= 3;
    for (col = 0; col < AV_VIEW_W; ++col) {
        int16_t camera = (int16_t)((int16_t)col * 24 - 228);
        int16_t rx = (int16_t)(dx[facing] * 256 + (plane_x[facing] * camera));
        int16_t ry = (int16_t)(dy[facing] * 256 + (plane_y[facing] * camera));
        uint8_t depth;
        uint8_t hit = 0;
        uint8_t tex = 0;
        uint8_t side = 0;
        for (depth = 1; depth <= 14; ++depth) {
            int16_t sx = (int16_t)(px + ((int32_t)rx * depth));
            int16_t sy = (int16_t)(py + ((int32_t)ry * depth));
            if (wall_at(map, sx, sy)) {
                uint8_t tx = (uint8_t)(sx >> 8);
                uint8_t ty = (uint8_t)(sy >> 8);
                hit = 1;
                tex = (uint8_t)((tx * 3u + ty * 5u) & 3u);
                side = (uint8_t)((rx < 0 || rx > 256) ? 1 : 0);
                break;
            }
        }
        if (!hit) depth = 14;
        columns[col].distance = depth;
        columns[col].texture = tex;
        columns[col].side = side;
        columns[col].top = (uint8_t)(9 - (7 / (depth + 1)) - (depth < 3 ? 2 : 0));
        columns[col].bottom = (uint8_t)(9 + (7 / (depth + 1)) + (depth < 3 ? 2 : 0));
    }
}

void av_heightfield(const uint8_t *heightmap, uint8_t offset,
                    uint8_t *screen_heights) {
    uint8_t x;
    for (x = 0; x < AV_VIEW_W; ++x) {
        uint8_t sample = heightmap[(uint8_t)(x + offset) & 31];
        screen_heights[x] = (uint8_t)(2 + (sample >> 1));
    }
}

uint16_t av_rle_decode(const uint8_t *src, uint16_t src_len,
                       uint8_t *dst, uint16_t dst_len) {
    uint16_t in = 0;
    uint16_t out = 0;
    while (in + 1 < src_len && out < dst_len) {
        uint8_t count = src[in++];
        uint8_t value = src[in++];
        while (count-- && out < dst_len) dst[out++] = value;
    }
    return out;
}
