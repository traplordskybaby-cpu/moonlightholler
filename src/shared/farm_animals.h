#ifndef FARM_ANIMALS_H
#define FARM_ANIMALS_H
#include <stdint.h>
#include "breeding.h"

#define FARM_ANIMAL_CAPACITY BREEDING_RECORDS

void farm_animals_init(void);
uint8_t farm_animals_count(uint8_t species);
const animal_record_t *farm_animals_get(uint8_t species, uint8_t index);
uint8_t farm_animals_breed(uint8_t species, uint8_t first, uint8_t second);
uint8_t farm_animals_last_id(void);

#endif
