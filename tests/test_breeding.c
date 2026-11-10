#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../src/shared/breeding.h"

int main(void) {
    animal_record_t sire, dam, chick, pup;
    animal_genome_t a, b, child;
    uint16_t seed = 0x4A31;
    uint8_t i;

    chicken_line_genome(CHICKEN_AYAM_CEMANI, &a);
    chicken_line_genome(CHICKEN_ZOMBIE_CROSS, &b);
    assert((chicken_phenotype(&a) & CHICKEN_SKIN_DARK) != 0);
    assert((chicken_phenotype(&a) & CHICKEN_FEATHER_BLACK) != 0);
    assert((chicken_phenotype(&b) & CHICKEN_SKIN_DARK) != 0);
    assert((chicken_phenotype(&b) & CHICKEN_FEATHER_WHITE) != 0);

    /* Homozygous parents must pass their allele at every locus. */
    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    for (i = 0; i < GENETIC_LOCI; ++i) {
        a.allele[i][0] = a.allele[i][1] = 1;
        b.allele[i][0] = b.allele[i][1] = 0;
    }
    genome_inherit(&a, &b, &child, &seed);
    for (i = 0; i < GENETIC_LOCI; ++i) {
        assert(child.allele[i][0] == 1);
        assert(child.allele[i][1] == 0);
    }

    memset(&sire, 0, sizeof(sire));
    memset(&dam, 0, sizeof(dam));
    sire.id = 7; sire.generation = 2; sire.line = CHICKEN_AYAM_CEMANI;
    dam.id = 9; dam.generation = 3; dam.line = CHICKEN_ZOMBIE_CROSS;
    chicken_line_genome(CHICKEN_AYAM_CEMANI, &sire.genome);
    chicken_line_genome(CHICKEN_ZOMBIE_CROSS, &dam.genome);
    chicken_make_offspring(&chick, &sire, &dam, 12, &seed);
    assert(chick.id == 12);
    assert(chick.species == ANIMAL_CHICKEN);
    assert(chick.sire_id == 7 && chick.dam_id == 9);
    assert(chick.generation == 4);
    assert(chick.sex == ANIMAL_HEN || chick.sex == ANIMAL_ROOSTER);
    assert(chick.traits == chicken_phenotype(&chick.genome));
    assert(chicken_matches_traits(&chick, 0) == 1);

    /* Guinea pig coat traits use their own phenotype decoder. */
    memset(&a, 0, sizeof(a));
    a.allele[0][0] = a.allele[0][1] = 1; /* long hair */
    a.allele[1][0] = 1;                 /* rosettes */
    a.allele[2][0] = a.allele[2][1] = 1; /* teddy coat */
    assert((guinea_pig_phenotype(&a) & GP_LONG_HAIR) != 0);
    assert((guinea_pig_phenotype(&a) & GP_ROSETTES) != 0);
    assert((guinea_pig_phenotype(&a) & GP_TEDDY_COAT) != 0);
    memset(&sire, 0, sizeof(sire));
    memset(&dam, 0, sizeof(dam));
    sire.id = 21; sire.generation = 1;
    dam.id = 22; dam.generation = 2;
    sire.species = dam.species = ANIMAL_GUINEA_PIG;
    chicken_line_genome(CHICKEN_AYAM_CEMANI, &sire.genome);
    chicken_line_genome(CHICKEN_ZOMBIE_CROSS, &dam.genome);
    guinea_pig_make_offspring(&pup, &sire, &dam, 23, &seed);
    assert(pup.id == 23 && pup.species == ANIMAL_GUINEA_PIG);
    assert(pup.sire_id == 21 && pup.dam_id == 22);
    assert(pup.generation == 3);
    assert(pup.traits == guinea_pig_phenotype(&pup.genome));

    puts("Moonlight Holler breeding tests passed.");
    return 0;
}
