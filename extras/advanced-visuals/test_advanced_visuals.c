#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "advanced_visuals.h"

int main(void) {
    uint8_t map[AV_MAP_W * AV_MAP_H];
    uint8_t heights[32];
    uint8_t output[AV_VIEW_W];
    uint8_t decoded[12];
    uint8_t encoded[] = { 3, 7, 4, 2, 5, 9 };
    av_ray_column_t rays[AV_VIEW_W];
    uint8_t x;

    assert(av_fx_lerp(0, 256, 128) == 128);
    assert(av_fx_lerp(256, 0, 128) == 128);

    memset(map, 0, sizeof(map));
    for (x = 0; x < AV_MAP_W; ++x) {
        map[x] = 1;
        map[(AV_MAP_H - 1) * AV_MAP_W + x] = 1;
    }
    for (x = 0; x < AV_MAP_H; ++x) {
        map[x * AV_MAP_W] = 1;
        map[x * AV_MAP_W + AV_MAP_W - 1] = 1;
    }
    map[3 * AV_MAP_W + 7] = 1;
    av_raycast(map, (int16_t)(3 * 256 + 128), (int16_t)(3 * 256 + 128), 1, rays);
    assert(rays[9].distance == 4);
    for (x = 0; x < AV_VIEW_W; ++x) {
        assert(rays[x].distance >= 1 && rays[x].distance <= 14);
        assert(rays[x].top <= 9);
        assert(rays[x].bottom >= 9);
        assert(rays[x].bottom < AV_VIEW_H);
    }

    for (x = 0; x < 32; ++x) heights[x] = x;
    av_heightfield(heights, 0, output);
    assert(output[0] == 2);
    assert(output[19] == 11);
    av_heightfield(heights, 1, output);
    assert(output[0] == 2);

    memset(decoded, 0, sizeof(decoded));
    assert(av_rle_decode(encoded, sizeof(encoded), decoded, sizeof(decoded)) == 12);
    for (x = 0; x < 3; ++x) assert(decoded[x] == 7);
    for (x = 3; x < 7; ++x) assert(decoded[x] == 2);
    for (x = 7; x < 12; ++x) assert(decoded[x] == 9);
    assert(av_rle_decode(encoded, sizeof(encoded), decoded, 5) == 5);

    puts("Moonlight Holler advanced visual engine tests passed.");
    return 0;
}
