#include <assert.h>
#include <stdio.h>
#include "../src/shared/farm_animals.h"

int main(void) {
    const animal_record_t *sire;
    const animal_record_t *dam;
    const animal_record_t *child;

    farm_animals_init();
    assert(farm_animals_count(ANIMAL_CHICKEN) == 5);
    assert(farm_animals_count(ANIMAL_GUINEA_PIG) == 3);

    sire = farm_animals_get(ANIMAL_CHICKEN, 0);
    dam = farm_animals_get(ANIMAL_CHICKEN, 1);
    assert(sire && dam);
    assert(sire->line == CHICKEN_ZOMBIE_CROSS);
    assert(sire->sex == ANIMAL_ROOSTER);
    assert(dam->line == CHICKEN_AYAM_CEMANI);
    assert(dam->sex == ANIMAL_HEN);
    assert(farm_animals_breed(ANIMAL_CHICKEN, 0, 1) == 1);
    assert(farm_animals_count(ANIMAL_CHICKEN) == 6);
    child = farm_animals_get(ANIMAL_CHICKEN, 5);
    assert(child && child->generation == 1);
    assert(child->sire_id == sire->id);
    assert(child->dam_id == dam->id);

    /* Same-sex pairs and self-pairing are rejected. */
    assert(farm_animals_breed(ANIMAL_CHICKEN, 1, 2) == 0);
    assert(farm_animals_breed(ANIMAL_CHICKEN, 0, 0) == 0);

    /* Guinea-pig records use the same parentage and inheritance pipeline. */
    assert(farm_animals_breed(ANIMAL_GUINEA_PIG, 0, 1) == 1);
    assert(farm_animals_count(ANIMAL_GUINEA_PIG) == 4);
    child = farm_animals_get(ANIMAL_GUINEA_PIG, 3);
    assert(child && child->species == ANIMAL_GUINEA_PIG);
    assert(child->generation == 1);
    assert(child->sire_id == 6);
    assert(child->dam_id == 7);

    puts("Moonlight Holler animal registry tests passed.");
    return 0;
}
