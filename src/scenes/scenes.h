#ifndef SCENES_H
#define SCENES_H
#include <stdint.h>
#define SECTION_TOWN 2
void town_enter(void);
uint8_t town_frame(uint8_t keys, uint8_t prev);
#define SECTION_AIR_HOCKEY 3
void air_hockey_enter(void);
uint8_t air_hockey_frame(uint8_t keys, uint8_t prev);
#define SECTION_BRAWLER 4
void brawler_enter(void);
uint8_t brawler_frame(uint8_t keys, uint8_t prev);
uint8_t town_get_day(void);
uint8_t town_get_happy(uint8_t species); /* 0 = chickens, 1 = guinea pigs */
#define SECTION_BOOK 5
void book_enter(void);
uint8_t book_frame(uint8_t keys, uint8_t prev);
#endif
