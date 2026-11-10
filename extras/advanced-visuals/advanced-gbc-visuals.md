# Moonlight Holler — Advanced Game Boy Color Visual Technology

## Design commitment

Moonlight Holler will use the Game Boy Color's advanced display capabilities as a defining part of its presentation. These are not six unrelated demos: they will be integrated into the farm, holler, lake, caves, weather, seasonal transitions, title sequence, and selected story scenes.

The target is original Game Boy Color hardware, not an emulator-only renderer. Every effect must have a fallback or reduced-quality mode, obey real hardware timing, and leave enough CPU/RAM for the farming simulation, animals, NPCs, and (later) link play.

## Hardware reality and terminology

The CGB has a 160×144 display, 15-bit RGB555 color selection (32,768 possible colors), eight 4-color background palettes, eight 4-color object palettes, 16 KiB VRAM across two banks, 32 KiB work RAM, and an optional 2× CPU mode. Hardware sprites are limited to 40 total and 10 on a scanline. The PPU is tile/palette based; it does not provide a true-color framebuffer, 3D accelerator, texture-mapping unit, video decoder, or fractional-pixel display.

Therefore:
- “Hi-color” means selecting from the full RGB555 color space and using timed palette changes/raster effects. It does **not** mean thousands of independently addressable colors in every frame.
- Raycasting, voxel-like terrain, and video decoding are CPU software techniques, not hardware modes.
- FMV means a carefully compressed sequence of tile updates, not modern full-screen video playback.
- Sub-pixel motion means smooth-looking fixed-point movement, dithering, or temporal/spatial tricks; the LCD still displays whole pixels.
- Font anti-aliasing is simulated with palette shading and ordered dithering; there is no per-pixel alpha blending.

## Feature specifications

### 1. “Hi-Color” palette and raster system

**Use in game:** dawn/sunset gradients over the farm, moonlit holler, lake reflections, lightning flashes, cave glow, seasonal color grading, and story-scene lighting.

**Implementation:** maintain authored RGB555 palette sets. During carefully timed scanline interrupts, change selected palette entries or scroll registers to create horizontal color bands and gradient-like transitions. Use palette cycling for water, fireflies, stars, and magical/rare-animal effects. Keep ordinary gameplay within the native palette budget and reserve raster-heavy scenes for controlled camera layouts.

**Constraints:** palette writes must be timed and tested on hardware; interrupt work must be short. Avoid claiming more than the hardware can display simultaneously. Provide a static-palette fallback for compatibility and debugging.

### 2. Pseudo-3D raycasting and texture mapping

**Use in game:** first-person mine passages, a short walk through a dark cave, a dream sequence, and an optional “look through the old tunnel” viewpoint. The main farm remains a top-down life sim.

**Implementation:** a low-resolution column renderer (initial target 40 columns, doubled horizontally) using fixed-point math, a small ray-angle lookup table, bounded map dimensions, and compact wall textures. Draw walls as tile strips or update a small reserved tile cache; do not attempt a conventional 160×144 software framebuffer. Use distance shading through palette selection and texture variation. Precompute or table-drive expensive operations.

**Performance target:** a short, deliberately paced sequence at a stable low frame rate is acceptable; responsive movement and collision take priority over visual complexity. Benchmark on CGB double-speed and single-speed modes. Disable expensive effects if frame time exceeds budget.

### 3. FMV-style playback

**Use in game:** short title/intro vignette, seasonal transitions, the windmill discovery, and a few major story moments. These should be brief cinematic sequences, not continuous video during normal exploration.

**Implementation:** offline-encode source art into keyframes plus tile/palette deltas. Stream compressed frame data from banked ROM into a small tile cache during VBlank-safe upload windows. Use a limited frame rate and carefully chosen image dimensions/color palettes. Synchronize simple chiptune/audio cues separately. Return to normal tile maps after playback.

**Initial target:** 2–6 seconds per sequence, roughly 4–8 frames per second depending on compression and scene complexity. A still-image-plus-parallax fallback is mandatory. Never promise full-resolution, full-motion, full-color video until measured on a real device.

### 4. Advanced sprite splitting and parallax layering

**Use in game:** foreground fence posts and leaves passing in front of the player, layered tree lines, distant mountains, rain, fireflies, lake shimmer, and large boss/creature sprites built from metasprites.

**Implementation:** assemble large characters/animals from multiple hardware sprites; carefully order OAM entries and cap visible objects per scanline. Use the two background map/VRAM banks, tile attributes, window layer, and timed SCX/SCY changes for layered depth. Simulate a foreground layer with tile priority and selected sprite pieces. Use scanline-aware spawn/culling rules to avoid the 10-objects-per-line limit.

**Constraints:** sprite splitting consumes OAM entries quickly. Crowds of chickens/guinea pigs must use compact metasprites and avoid stacking all animals on one scanline. Parallax bands must not interfere with HUD timing or scrolling maps.

### 5. CPU-driven software rendering and voxel-like terrain

**Use in game:** a special mountain overlook, misty hills, a mine-depth sequence, and stylized water/terrain scenes.

**Implementation:** render a low-resolution heightfield or column/row-span landscape using fixed-point lookup tables and a compact height map. Convert output into tile patterns and update only changed tiles in safe transfer windows. Use palette bands for distance fog and lighting. This is a stylized voxel-inspired effect, not a general-purpose 3D engine.

**Constraints:** keep map dimensions small, update a bounded number of tiles per frame, and run this renderer only in dedicated scenes. Farm simulation and NPC logic must not be starved by the renderer.

### 6. Sub-pixel motion and anti-aliased custom fonts

**Use in game:** smoother camera easing, slow moonlit water drift, subtle character idle motion, title lettering, readable dialogue, and menu typography.

**Implementation:** store camera/entity positions in fixed-point units and round only when writing hardware coordinates. Use temporal dithering or alternating tile patterns for selected slow-moving effects; never make core collision depend on visual sub-pixel positions. Build small bitmap font variants with edge shades represented by palette colors and ordered dithering. Use a crisp non-antialiased font fallback for tiny text and fast dialogue.

**Constraints:** the physical pixel grid remains integer-addressed. Temporal dithering can flicker on the original LCD, so it must be subtle and optional. Font shading consumes palette colors and tile memory; prioritize legibility.

## Integration into Moonlight Holler

| Game area | Techniques |
|---|---|
| Farm and trailer | Palette lighting, subtle parallax, fixed-point camera easing, readable shaded UI font |
| Weather and seasons | RGB555 palette swaps, scanline sky gradients, rain/snow tile animation |
| Owsley Fork Lake | Palette cycling, layered reflections, distant parallax |
| Forest and windmill | Foreground sprite splitting, moonlight raster bands, short FMV-style reveal |
| Mines and caves | Raycast corridors, texture mapping, palette distance shading |
| Mountain overlook | Voxel-inspired heightfield panorama and parallax mountain layers |
| Title and major story beats | Short compressed FMV-style sequences and custom shaded lettering |

## Engineering order and acceptance tests

1. **CGB display foundation:** explicitly target CGB mode; add palette tables, double-speed detection, safe VRAM update queues, and a scanline/raster test scene. Verify palette and interrupt behavior on at least one cycle-accurate emulator and original hardware if available.
2. **Parallax and sprite compositor:** build a scanline budget tool/test scene. Confirm no more than 10 hardware objects intersect any scanline and that the 40-object frame limit is respected.
3. **Fixed-point camera and font renderer:** compare movement/collision against integer-coordinate gameplay; confirm text remains legible on real LCD.
4. **Raycaster prototype:** benchmark 40-column and 32-column variants; record frame time and code/RAM usage. Select quality based on measurements, not assumptions.
5. **Voxel-style panorama:** reuse the tile upload queue and lookup-table infrastructure; cap changed tiles per frame.
6. **FMV encoder/player:** create an offline asset encoder, measure ROM size and per-frame tile upload cost, then test the windmill scene first.
7. **Whole-game integration:** profile worst-case farm scenes with all animals, crows, crop plots, HUD, music, and NPCs active. Advanced scenes must yield to core gameplay and future link-cable timing.

## Required test matrix

- Real CGB at normal and double CPU speed.
- Accurate emulators (including a scanline-sensitive emulator) with frame-time and dropped-frame checks.
- DMG fallback only if retained as a product requirement; the primary target is CGB.
- Worst-case sprite density: all roaming animals, player, effects, and foreground pieces visible together.
- Worst-case farm logic running while palette/raster effects are active.
- ROM banking and asset streaming tests for every FMV/raycast texture bank.
- Link-cable mode later: disable or simplify raster/FM V effects if they compromise serial timing or deterministic simulation.

## Current implementation status (2026-11-08)

Source prototypes now exist for the core techniques: RGB555 palette cycling, a 20-column pseudo-3D cave raycaster, RLE tile-keyframe playback, a window/background parallax mountain view with a CPU heightfield, fixed-point camera easing, a four-piece metasprite compositor, and shaded font-tile generation. The cave scene is wired into the main scene dispatcher and is entered through the visible cave arch on the farm's upper path.

Host-side tests pass for the pure C raycaster/heightfield/fixed-point/RLE module. A syntax-only build against GBDK API stubs was mentioned in an earlier report, but the stubs and invocation are not packaged and that check was not independently reproduced in the current session. This is **not** a GBDK ROM build, cycle-accurate emulator test, or original-hardware test. Timed scanline raster gradients, full story-length FMV content, lake/windmill-specific sequences, and real CGB timing/VRAM/OAM profiling remain unimplemented or unverified. Do not mark the whole advanced-visuals feature complete until those checks pass on a compiled ROM and hardware-accurate emulator/device.
