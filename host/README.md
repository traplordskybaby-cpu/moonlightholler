# Desktop test harness (optional)

This lets the game's real C code run on a normal computer, without GBDK or a Game Boy, so changes can be
tested quickly. `host/mock/` is a stand-in for the GBDK library; `host/emu.py` draws the Game Boy Color
screen from the dumped video memory.

    python3 tools/make_assets.py
    sh host/build.sh                     # builds build/game_host with gcc
    sh host/run.sh NAME FRAMES "KEYS" DUMP_FRAMES [COMMANDS]

* KEYS are held for a number of frames: `R:30 A:2 .:20` (R L U D A B S=start E=select, `.` = nothing)
* COMMANDS teleport and cheat for testing: `100:goto:AREA:TX:TY;130:npc:0:0:0;60:give:KIND:N;50:time:22:0:0`
* output: `build/NAME_f<frame>.png` (screenshot) and `.txt` (game state)

It cannot catch problems that only the real compiler (SDCC) or hardware would show.
