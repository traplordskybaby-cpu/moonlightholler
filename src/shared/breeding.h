#ifndef BREEDING_H
#define BREEDING_H

#include <stdint.h>

/* Compact, cartridge-friendly inheritance model for Moonlight Holler.
 * This is a game simulation, not a veterinary or poultry-genetics calculator.
 */
#define GENETIC_LOCI 6
#define BREEDING_RECORDS 24

enum animal_species { ANIMAL_CHICKEN = 0, ANIMAL_GUINEA_PIG = 1 };
enum animal_sex { ANIMAL_UNKNOWN = 0, ANIMAL_HEN = 1, ANIMAL_ROOSTER = 2,
                  ANIMAL_FEMALE = ANIMAL_HEN, ANIMAL_MALE = ANIMAL_ROOSTER };
enum chicken_line {
    CHICKEN_AYAM_CEMANI = 0,
    CHICKEN_ZOMBIE_CROSS = 1,
    CHICKEN_BLACK_SILKIE = 2,
    CHICKEN_EGYPTIAN_FAYOUMI = 3,
    CHICKEN_BARRED_ROCK = 4
};

enum guinea_pig_line { GP_ABYSSINIAN = 0, GP_PERUVIAN = 1, GP_TEDDY = 2 };

enum guinea_pig_trait {
    GP_LONG_HAIR = 0x01,
    GP_ROSETTES = 0x02,
    GP_TEDDY_COAT = 0x04,
    GP_SATIN_COAT = 0x08,
    GP_SKINNY = 0x10,
    GP_LARGE_FRAME = 0x20
};

enum chicken_trait {
    CHICKEN_FEATHER_BLACK = 0x01,
    CHICKEN_FEATHER_WHITE = 0x02,
    CHICKEN_SKIN_DARK = 0x04,
    CHICKEN_BARRING = 0x08,
    CHICKEN_SILKIED = 0x10,
    CHICKEN_LARGE_FRAME = 0x20
};

typedef struct {
    uint8_t allele[GENETIC_LOCI][2];
} animal_genome_t;

typedef struct {
    uint8_t id;
    uint8_t species;
    uint8_t sex;
    uint8_t generation;
    uint8_t sire_id;
    uint8_t dam_id;
    uint8_t line;
    uint8_t traits;
    animal_genome_t genome;
} animal_record_t;

/* Chicken loci: pigment, dominant-white modifier, fibromelanosis, barring,
 * silkied feather structure, and frame size. Alleles are 0/1. */
void chicken_line_genome(uint8_t line, animal_genome_t *out);
uint8_t chicken_phenotype(const animal_genome_t *genome);
uint8_t guinea_pig_phenotype(const animal_genome_t *genome);

/* Each offspring receives one randomly selected allele from each parent at
 * every locus. Seed is caller-owned so save-game determinism is possible. */
void genome_inherit(const animal_genome_t *sire,
                    const animal_genome_t *dam,
                    animal_genome_t *offspring,
                    uint16_t *seed);

/* Creates a chicken record while retaining explicit parentage. */
void guinea_pig_make_offspring(animal_record_t *out,
                               const animal_record_t *sire,
                               const animal_record_t *dam,
                               uint8_t new_id,
                               uint16_t *seed);
void chicken_make_offspring(animal_record_t *out,
                            const animal_record_t *sire,
                            const animal_record_t *dam,
                            uint8_t new_id,
                            uint16_t *seed);

/* Returns 1 when every requested visual trait is present. */
uint8_t chicken_matches_traits(const animal_record_t *bird, uint8_t wanted);

#endif
