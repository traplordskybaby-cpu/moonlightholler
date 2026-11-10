/* Small helpers the minigames share (the old generated runtime is no longer needed). */
#ifndef RT_H
#define RT_H
#include <stdint.h>
uint8_t rt_random(void);
void rt_seed(uint8_t extra);
void rt_scene_enter(void);
#endif
