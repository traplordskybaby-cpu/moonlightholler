# Moonlight Holler — core direction

A Game Boy Color life simulation and small-hobby-farm RPG set around the real Big Hill / Alexander Road setting in Kentucky. Its emotional center is a love letter to Rebecca, who helped originate the idea, and to the everyday life the farm represents.

The design combines the approachable daily rhythm of Harvest Moon and Stardew Valley with top-down exploration, secrets, and consequential dialogue choices inspired by classic Fallout. The player builds a life in the holler through animal care, selective breeding, crops, fishing, exploration, friendships, and choices that change how the community responds.

## Non-negotiable preservation

The original farm is the foundation, not disposable prototype content. Preserve the existing three roaming guinea pigs, three roaming chickens, animal feeding and happiness, 52 crop plots, cucumber / pepper / lettuce / corn / hay, watering, crop growth and harvesting, crop-count and animal-heart HUD, crows stealing crops and player shooing, farm buddy crop care / crow deterrence, day-night clock, living-room environment, and both existing minigames. The Air Hockey and Dirt Road Brawler cabinets should ultimately be accessed inside the explorable trailer interior, not the animal yard.

## Core pillars

1. **Small-farm life simulation:** daily chores, food production, animal care, weather / time, home, and gradual property improvements.
2. **Selective breeding:** individual chickens and guinea pigs have IDs, parents, generations, inheritable traits, and a lineage book. The chicken lines include Ayam Cemani, white-feathered/dark-skinned Cemani-Leghorn-style zombie chickens, Black Silkies, Egyptian Fayoumi, and Barred Rocks.
3. **A living community:** NPC schedules, relationship states, memory of key player actions, evolving dialogue, gifts, favors, and relationship-gated scenes.
4. **Consequential narrative:** dialogue choices and actions set persistent story flags, opening or closing quests, relationships, and endings. Choices should be legible but not always consequence-free.
5. **Folklore at the edge of realism:** the hidden windmill introduces rare guinea pig varieties and a feral-colony system, contrasting with the otherwise grounded rural setting.
6. **Exploration:** a connected holler with the trailer, neighboring homes, village, woods, lake, caves, and mountain paths.

## Animal breeding

The first implementation is documented in `plan/breeding-system.md`. The initial code module is a compact, deterministic chicken inheritance model with explicit parentage and a cached phenotype. It is a game abstraction, not a scientifically complete poultry genetics model. Guinea pigs should use the same lineage architecture with their own phenotype rules.

## Social and choice systems

NPCs should track relationship level plus a small set of durable memory flags: helped, insulted, promised, gift liked / disliked, quest outcome, and major story choices. Dialogue selection should use these flags and current relationship rather than a single generic friendship threshold. Branching choices write persistent story flags; the same flags can change later dialogue and quest availability.

## Windmill event

After a story unlock, a secluded windmill in the woods has a cutscene. Every three in-game days a different guinea pig variety appears at its base. The player can retrieve and keep it or leave it. Animals left behind may form feral colonies. Bringing appropriate food can calm or distract territorial feral guinea pigs during travel. The system should make preparation and player choice matter without creating unavoidable random punishment.

## Co-op

The intended multiplayer mode is two Game Boy systems linked by a compatible Link Cable / emulator link feature, with each player controlling a separate character. This is a later technical milestone; emulator and hardware compatibility must be tested rather than assumed.

## Build order

1. Preserve and regression-test original farm behavior.
2. Add and test the compact animal genetics / lineage data model.
3. Integrate breeding interactions, animal roster, and persistent save data.
4. Build the trailer interior and move both minigame entrances inside.
5. Add windmill event and feral guinea pig colonies.
6. Add NPC memory, relationship-driven dialogue, and branching quest flags.
7. Expand the connected world and exploration systems.
8. Implement and test Link Cable co-op.

## Current implementation status

The original source has been inspected. A first standalone chicken breeding module and host-side tests have been added. The module is not yet integrated into playable menus, animal state, save data, or art. The GBDK compiler is not currently available in the working environment, so no new Game Boy ROM build is claimed.
