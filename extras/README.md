# extras (not part of the build)

`advanced-visuals/` is the pseudo-3D cave raycaster, FMV encoder and display-effects code from the other
version of the project (the one worked on in ChatGPT). Nothing here is compiled into the ROM.

Why it is parked: the raycaster and video code were never built with GBDK or run on a Game Boy, and the
cave scene would need real-hardware profiling (frame rate, VRAM timing). The plan is to get the base game
building and playing on your machine first, then bring this back one piece at a time.
The desktop tests that exist for it still pass on their own:

    gcc -std=c89 -Wall -Wextra -Werror -pedantic -Iadvanced-visuals advanced-visuals/test_advanced_visuals.c advanced-visuals/advanced_visuals.c -o /tmp/t && /tmp/t
