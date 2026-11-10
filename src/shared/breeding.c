#include "breeding.h"
#include <string.h>

static uint8_t roll_allele(uint16_t *seed, const uint8_t pair[2]) {
    /* 16-bit LCG: small, deterministic, and suitable for the Game Boy CPU. */
    *seed = (uint16_t)(*seed * 25173u + 13849u);
    return pair[(*seed >> 8) & 1];
}

void chicken_line_genome(uint8_t line, animal_genome_t *out) {
    memset(out, 0, sizeof(*out));

    /* Locus order: pigment, dominant white, fibromelanosis, barring,
     * silkied feather structure, large frame. These are deliberately
     * simplified game alleles; named lines are starting templates. */
    switch (line) {
    case CHICKEN_AYAM_CEMANI:
        out->allele[0][0] = out->allele[0][1] = 1;
        out->allele[2][0] = out->allele[2][1] = 1;
        break;
    case CHICKEN_ZOMBIE_CROSS:
        /* White-feathered, dark-skinned Cemani/Leghorn-style game line. */
        out->allele[0][0] = out->allele[0][1] = 1;
        out->allele[1][0] = 1;
        out->allele[2][0] = out->allele[2][1] = 1;
        break;
    case CHICKEN_BLACK_SILKIE:
        out->allele[0][0] = out->allele[0][1] = 1;
        out->allele[2][0] = out->allele[2][1] = 1;
        out->allele[4][0] = out->allele[4][1] = 1;
        break;
    case CHICKEN_EGYPTIAN_FAYOUMI:
        out->allele[0][0] = 1;
        out->allele[3][0] = out->allele[3][1] = 1;
        break;
    case CHICKEN_BARRED_ROCK:
        out->allele[0][0] = out->allele[0][1] = 1;
        out->allele[3][0] = out->allele[3][1] = 1;
        out->allele[5][0] = out->allele[5][1] = 1;
        break;
    default:
        break;
    }
}

uint8_t chicken_phenotype(const animal_genome_t *g) {
    uint8_t traits = 0;
    uint8_t pigment = (uint8_t)(g->allele[0][0] + g->allele[0][1]);
    uint8_t white = (uint8_t)(g->allele[1][0] | g->allele[1][1]);
    uint8_t fm = (uint8_t)(g->allele[2][0] + g->allele[2][1]);
    uint8_t barring = (uint8_t)(g->allele[3][0] | g->allele[3][1]);
    uint8_t silky = (uint8_t)(g->allele[4][0] + g->allele[4][1]);
    uint8_t frame = (uint8_t)(g->allele[5][0] | g->allele[5][1]);

    if (white) traits |= CHICKEN_FEATHER_WHITE;
    else if (pigment) traits |= CHICKEN_FEATHER_BLACK;
    if (fm == 2) traits |= CHICKEN_SKIN_DARK;
    if (barring) traits |= CHICKEN_BARRING;
    if (silky == 2) traits |= CHICKEN_SILKIED;
    if (frame) traits |= CHICKEN_LARGE_FRAME;
    return traits;
}

uint8_t guinea_pig_phenotype(const animal_genome_t *g) {
    uint8_t traits = 0;
    if (g->allele[0][0] && g->allele[0][1]) traits |= GP_LONG_HAIR;
    if (g->allele[1][0] || g->allele[1][1]) traits |= GP_ROSETTES;
    if (g->allele[2][0] && g->allele[2][1]) traits |= GP_TEDDY_COAT;
    if (g->allele[3][0] && g->allele[3][1]) traits |= GP_SATIN_COAT;
    if (g->allele[4][0] && g->allele[4][1]) traits |= GP_SKINNY;
    if (g->allele[5][0] || g->allele[5][1]) traits |= GP_LARGE_FRAME;
    return traits;
}

void genome_inherit(const animal_genome_t *sire,
                    const animal_genome_t *dam,
                    animal_genome_t *offspring,
                    uint16_t *seed) {
    uint8_t locus;
    for (locus = 0; locus < GENETIC_LOCI; ++locus) {
        offspring->allele[locus][0] = roll_allele(seed, sire->allele[locus]);
        offspring->allele[locus][1] = roll_allele(seed, dam->allele[locus]);
    }
}

void chicken_make_offspring(animal_record_t *out,
                            const animal_record_t *sire,
                            const animal_record_t *dam,
                            uint8_t new_id,
                            uint16_t *seed) {
    memset(out, 0, sizeof(*out));
    out->id = new_id;
    out->species = ANIMAL_CHICKEN;
    out->sex = ((*seed = (uint16_t)(*seed * 25173u + 13849u)) & 1)
        ? ANIMAL_ROOSTER : ANIMAL_HEN;
    out->generation = (sire->generation > dam->generation)
        ? (uint8_t)(sire->generation + 1) : (uint8_t)(dam->generation + 1);
    out->sire_id = sire->id;
    out->dam_id = dam->id;
    out->line = sire->line;
    genome_inherit(&sire->genome, &dam->genome, &out->genome, seed);
    out->traits = chicken_phenotype(&out->genome);
}

void guinea_pig_make_offspring(animal_record_t *out,
                               const animal_record_t *sire,
                               const animal_record_t *dam,
                               uint8_t new_id,
                               uint16_t *seed) {
    memset(out, 0, sizeof(*out));
    out->id = new_id;
    out->species = ANIMAL_GUINEA_PIG;
    out->sex = ((*seed = (uint16_t)(*seed * 25173u + 13849u)) & 1)
        ? ANIMAL_MALE : ANIMAL_FEMALE;
    out->generation = (sire->generation > dam->generation)
        ? (uint8_t)(sire->generation + 1) : (uint8_t)(dam->generation + 1);
    out->sire_id = sire->id;
    out->dam_id = dam->id;
    out->line = sire->line;
    genome_inherit(&sire->genome, &dam->genome, &out->genome, seed);
    out->traits = guinea_pig_phenotype(&out->genome);
}

uint8_t chicken_matches_traits(const animal_record_t *bird, uint8_t wanted) {
    return (uint8_t)((bird->traits & wanted) == wanted);
}
