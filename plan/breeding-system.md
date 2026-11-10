# Moonlight Holler: animal breeding system

## Design goal

Breeding is a long-term farm activity, not a menu that instantly converts two named breeds into a third. Each animal is an individual with a stable ID, sex, generation, parents, genome, visible phenotype, needs, and relationship state. Offspring inherit alleles from both parents; siblings can therefore differ. The player can keep lineage records and select future breeding pairs to stabilize a desired appearance over multiple generations.

## Chicken lines represented at the start

- Ayam Cemani
- White-feathered / dark-skinned Cemani-Leghorn-style "zombie" line
- Black Silkie
- Egyptian Fayoumi
- Barred Rock

The game should make the player's breeding goal legible: develop a recognizable, consistent line rather than simply collect cross names. A future Breed Book / flock registry can track the frequency of desired traits among the latest generation and reward repeatability across several generations.

## Individual record

The initial cartridge-friendly record stores:

- ID, species, sex, generation
- sire ID and dam ID
- starting line / family label
- compact genome (six two-allele loci in the first implementation)
- cached phenotype flags

The initial chicken simulation loci are feather pigment, dominant-white modifier, fibromelanosis, barring, silkied feather structure, and frame size. Each offspring receives one allele from each parent at each locus. The random seed is explicit and caller-owned so a save system can preserve deterministic outcomes.

**Genetics disclaimer:** These are intentionally simplified game alleles, not a scientifically complete model of chicken inheritance. Real traits such as fibromelanosis, white plumage, barring, and feather structure have more complex inheritance, modifiers, sex-linkage, and breed-dependent effects. The game's model should be tunable and should not be presented as real breeding advice.

## Guinea pig system

Use the same animal-record and lineage concepts, but a separate species-specific phenotype decoder. Guinea pig loci should cover coat length, rosette pattern, coat texture, base color, color distribution, and body type. Do not assume all eleven of Rebecca's actual varieties until the user supplies their names or photographs; the in-game collection can begin with a few recognizable coat families and add the rest as confirmed references.

## Breeding loop

1. Keep eligible adult animals of compatible species and sex in suitable housing.
2. Maintain nutrition, cleanliness, health, and happiness; poor care can reduce breeding readiness.
3. Select a sire and dam, with clear display of their known traits and lineage.
4. A pregnancy / incubation timer advances in game time.
5. An offspring record is generated from parental genomes and assigned a permanent ID.
6. The player names the offspring and may keep, trade, or release it where the story permits.
7. The lineage book records parents, generation, visible traits, and breeding outcomes.

No animal should be created from an invalid pairing, and no offspring should lose its parent IDs when the roster is rearranged.

## Selection and breed stabilization

The player chooses a target profile (for example: white feathers + dark skin, or black feathers + dark skin + non-silkie feather structure). The registry tracks the percentage of living, eligible birds in the selected line that express every target trait. A line is considered visually stable only after the target profile appears consistently across multiple consecutive generations and a minimum sample size. This is a gameplay milestone, not a real-world breed-recognition claim.

## Guinea pig windmill event

The windmill is a separate rare-variety introduction system, not a replacement for normal breeding. Once unlocked, a new wild guinea pig appears every three in-game days. The player can bring it home or leave it. Left-behind animals can establish a feral colony over time. The colony's temperament and population are driven by player choices and food availability. Carrying suitable food can calm or distract territorial animals; encounters should be avoidable through preparation rather than random unavoidable damage.

## Game Boy Color constraints

- Keep the active animal roster bounded and use compact IDs rather than pointers in save data.
- Store six loci as twelve small allele values per animal; expand only after measuring ROM/RAM use.
- Cache phenotype flags rather than recalculating them every frame.
- Run breeding only on explicit interaction / day transition, never in the frame loop.
- Use deterministic 16-bit random state and include it in save data.
- Keep the original farm systems intact: 52 crop plots, five crops, feeding, happiness, roaming animals, crows, buddy behavior, day/night clock, and current minigames remain in place.

## Current implementation status

`src/shared/breeding.h` and `src/shared/breeding.c` provide the compact genetics model, named-line templates, inheritance, phenotype decoding, parentage, and trait matching. `src/shared/farm_animals.c/.h` adds the session roster and the town scene opens a basic breeding-book UI. Host-side tests cover inheritance and registry behavior. Save persistence, incubation timing, and one-to-one sprite binding remain unfinished.

## First in-game registry milestone

The project now includes `src/shared/farm_animals.c/.h`, a compact session registry
that seeds five named chicken lines (Zombie Cross rooster, Ayam Cemani hen, Black
Silkie hen, Egyptian Fayoumi hen, Barred Rock hen) and three starter guinea-pig
coat templates. Each record stores an ID, species, sex, generation, sire ID, dam
ID, line, expressed trait mask, and six-locus genome. The registry rejects
same-sex and self-pairings and creates offspring through the shared inheritance
routine.

In the farm scene, press **START** to open the Farm Breeding Book. **SELECT**
switches between chicken and guinea-pig stock; **UP/DOWN** changes the selected
animal; press **A** once for the first parent and again for the second parent;
**B** clears the pending parent; **START** returns to the farm. A successful pair
adds an offspring record and records both parent IDs and the next generation.

Current limitations are intentional and explicit: this first integration is a
session-only prototype (the registry is not yet serialized into a save file),
offspring are registered immediately rather than passing through a gestation,
incubation, or brooding timer, and the visible roaming animal sprites are not yet
bound one-to-one to registry records. The six-locus model is a simplified game
heritability model, not a scientifically validated poultry or cavy genetics
predictor. The starter guinea-pig templates are placeholders pending the full
catalog of the eleven real varieties.

## Validation for this milestone

The host-side commands below passed with GCC `-Wall -Wextra -Werror` and
AddressSanitizer/UndefinedBehaviorSanitizer:

```sh
gcc -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined -Isrc \
  src/shared/breeding.c src/shared/farm_animals.c tests/test_breeding.c -o test_breeding
./test_breeding

gcc -std=c99 -Wall -Wextra -Werror -fsanitize=address,undefined -Isrc \
  src/shared/breeding.c src/shared/farm_animals.c tests/test_farm_animals.c -o test_farm_animals
./test_farm_animals
```

A Game Boy ROM build was attempted with `make release`; it stopped before C
compilation because `/opt/gbdk/bin/lcc` is absent in the current environment.
No new ROM is claimed for this milestone.
