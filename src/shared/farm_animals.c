#include "farm_animals.h"
#include <string.h>

static animal_record_t animals[FARM_ANIMAL_CAPACITY];
static uint8_t animal_count;
static uint8_t next_id;
static uint16_t breeding_seed = 0x4D48u;
static uint8_t initialized;

static void add_chicken(uint8_t line, uint8_t sex) {
    animal_record_t *a;
    if (animal_count >= FARM_ANIMAL_CAPACITY) return;
    a = &animals[animal_count++];
    memset(a, 0, sizeof(*a));
    a->id = next_id++;
    a->species = ANIMAL_CHICKEN;
    a->sex = sex;
    a->generation = 0;
    a->line = line;
    chicken_line_genome(line, &a->genome);
    a->traits = chicken_phenotype(&a->genome);
}

static void add_guinea_pig(uint8_t sex, uint8_t traits, uint8_t line) {
    animal_record_t *a;
    uint8_t i;
    if (animal_count >= FARM_ANIMAL_CAPACITY) return;
    a = &animals[animal_count++];
    memset(a, 0, sizeof(*a));
    a->id = next_id++;
    a->species = ANIMAL_GUINEA_PIG;
    a->sex = sex;
    a->generation = 0;
    a->line = line;
    /* Seed simple homozygous/dominant templates for visible starter varieties. */
    for (i = 0; i < GENETIC_LOCI; ++i) {
        uint8_t bit = (uint8_t)(1u << i);
        if (traits & bit) {
            a->genome.allele[i][0] = 1;
            a->genome.allele[i][1] = 1;
        }
    }
    a->traits = guinea_pig_phenotype(&a->genome);
}

void farm_animals_init(void) {
    if (initialized) return;
    animal_count = 0;
    next_id = 1;
    /* Initial breeding stock reflects the farm's named chicken lines. */
    add_chicken(CHICKEN_ZOMBIE_CROSS, ANIMAL_ROOSTER);
    add_chicken(CHICKEN_AYAM_CEMANI, ANIMAL_HEN);
    add_chicken(CHICKEN_BLACK_SILKIE, ANIMAL_HEN);
    add_chicken(CHICKEN_EGYPTIAN_FAYOUMI, ANIMAL_HEN);
    add_chicken(CHICKEN_BARRED_ROCK, ANIMAL_HEN);
    /* A small starter sample; later updates will connect all 11 discovered varieties. */
    add_guinea_pig(ANIMAL_MALE, GP_ROSETTES, GP_ABYSSINIAN);
    add_guinea_pig(ANIMAL_FEMALE, GP_LONG_HAIR, GP_PERUVIAN);
    add_guinea_pig(ANIMAL_FEMALE, GP_TEDDY_COAT, GP_TEDDY);
    initialized = 1;
}

uint8_t farm_animals_count(uint8_t species) {
    uint8_t i, count = 0;
    for (i = 0; i < animal_count; ++i)
        if (animals[i].species == species) ++count;
    return count;
}

const animal_record_t *farm_animals_get(uint8_t species, uint8_t index) {
    uint8_t i, found = 0;
    for (i = 0; i < animal_count; ++i) {
        if (animals[i].species == species) {
            if (found == index) return &animals[i];
            ++found;
        }
    }
    return 0;
}

uint8_t farm_animals_breed(uint8_t species, uint8_t first, uint8_t second) {
    const animal_record_t *a = farm_animals_get(species, first);
    const animal_record_t *b = farm_animals_get(species, second);
    animal_record_t *out;
    const animal_record_t *sire, *dam;
    if (!a || !b || a == b || animal_count >= FARM_ANIMAL_CAPACITY) return 0;
    if (a->sex == b->sex) return 0;
    sire = (a->sex == ANIMAL_ROOSTER || a->sex == ANIMAL_MALE) ? a : b;
    dam = (sire == a) ? b : a;
    out = &animals[animal_count];
    if (species == ANIMAL_CHICKEN)
        chicken_make_offspring(out, sire, dam, next_id, &breeding_seed);
    else
        guinea_pig_make_offspring(out, sire, dam, next_id, &breeding_seed);
    ++animal_count;
    ++next_id;
    return 1;
}

uint8_t farm_animals_last_id(void) { return (uint8_t)(next_id - 1); }
