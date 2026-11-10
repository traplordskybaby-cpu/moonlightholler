# Moonlight Holler (Game Boy Color)

A cozy farm life-sim: tend crops, keep your guinea pigs and chickens happy, breed new lines, and wander
out to meet the neighbors. Arcade (Dirt Road Brawler 2) and air hockey are waiting in the trailer.

## What is in the game

| Place | How you get there |
|---|---|
| **The farm** | where you start: crops, guinea pig pen, hens, barn, crows, and your farm buddy |
| **The trailer** | the door on the left of the farm. Arcade cabinet, air hockey table, bed, Mamaw Pearl |
| **The village** | the east trail off the farm. Hobbs' General Store, a cottage, a well, notice board, Daisy and Wendell |
| **Hobbs' store** | the door under the STORE sign. Sell crops, buy pig treats and hen feed |
| **Owsley Fork Lake** | the south trail off the farm. A creek with a bridge, a pond and dock, Old Gus |

Title screen: your box art. Day/night, a clock, gold, and a Breeding Book are built in.

### Controls
| Button | Does |
|---|---|
| D-pad | walk |
| A | talk, read signs, use machines, harvest, feed. **Hold A** to spray water (waters crops, shoos crows) |
| SELECT | swap places with your farm buddy (on the farm) |
| START | open the **Breeding Book** (anywhere) |
| START / SELECT | leave air hockey / leave Brawler early |

### Breeding Book
Chickens and guinea pigs each have genes (feather color, barring, silkie feathers, coat type...).
Pick a hen and a rooster (or sow and boar), A on each, and a new animal is born with traits inherited
from both parents. Animals need to be well cared for (3+ hearts), and each kind can breed once per day.
The newest chickens show up in the flock on the farm, in their real feather colors.
SELECT switches between chickens and guinea pigs.

### Little loops
* Sell crops at Hobbs' store (gold shows in the HUD next to the coin); buy treats to refill hearts.
* Daisy wants 3 cucumbers; Wendell wants 4 corn. Both pay gold.
* Sleep in the trailer bed to skip to the next morning.

## Building the ROM

**Easiest, nothing to install:** make a free GitHub account, create a new repository, and upload this whole
folder (including the hidden `.github` folder). Open the repository's **Actions** tab, click the newest
"Build ROM" run, and download `moonlight-holler-rom` at the bottom. If the run turns red, open it, copy the
error text, and send it to me.

**Or build on your own computer:**

You need two free things: **GBDK-2020 version 4.2 or newer** (https://github.com/gbdk-2020/gbdk-2020/releases,
just unzip it) and **Python 3** (with the `Pillow` package, `pip install pillow`).

**Windows (PowerShell)**

    .\build-windows.ps1 -GbdkHome 'C:\gbdk'

**Mac / Linux**

    ./build.sh /path/to/gbdk

The result is `build/moonlight-holler.gb`. Open it in any Game Boy Color emulator (mGBA, BGB, SameBoy...).
Pick an emulator or flash cart that supports **MBC5** cartridges (practically all do).

If something goes wrong, copy the red error text and send it back: the build was written carefully but
could not be run through the real GBDK compiler where it was authored, so a first-build hiccup is possible.

### How the ROM is laid out
The cartridge is 64 KB with an MBC5 chip. The program code lives in banks 0-1 (always visible). The big
stuff (cover art, the five maps with their tiles, dialogue, and the minigame art) lives in banks 2-3 and is
copied out when needed by `src/bank.c`. `tools/make_assets.py` builds that data and `tools/pack_rom.py`
appends it to the compiled program and fixes the cartridge header.

### If the build says the code is too big
`pack_rom.py` stops with a message if the compiled code passes 32 KB. We estimate it will use about 25 KB.
If it does overflow, tell me and I will move more code into a banked layout.

## Changing things

| To change | Edit |
|---|---|
| a place's layout, doors, signs, NPC spots | `tools/areas.py` (see `build_village()` for a worked example) |
| NPC and sign text (3 lines of 18 characters per page) | `tools/dialog.py` (it checks line lengths for you) |
| villager sprites | `tools/npc_art.py` |
| palettes | `tools/worldkit.py` |
| quests, prices, sleep, gold | `src/scenes/town.c` |
| Breeding Book screen | `src/scenes/book.c`; the genetics are in `src/shared/breeding.c` |
| the cover art | replace `art/box.png`, then `python3 tools/cover_convert.py` (needs numpy, scikit-learn) |

`art/preview/` has a picture of every place, regenerated each build, so you can see a map change without
a Game Boy. `plan/` has the design notes; `extras/` holds the parked 3D-cave and video experiments.

## Testing without hardware
`make host-test` runs the genetics tests. `host/README.md` describes a desktop harness that runs the real game
code, takes screenshots, and scripts button presses.
