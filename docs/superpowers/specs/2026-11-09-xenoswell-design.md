# Xenoswell (working title): design

A surfing simulation on an alien ocean, written in C as a real PlayStation 1
program and shipped as a disc image that boots in an emulator.

## Intent

The player should feel they are reading and riding huge, slow swells of glowing
liquid on another planet. It is a simulation in spirit: the board responds to the
moving surface, catching a wave is a skill, and bad landings end the ride.

## Decided by the user

- Simulation (not arcade), alien and futuristic setting.
- The planet changes the physics, same rules throughout: low gravity, dense
  liquid, huge slow swells.
- A run is a timed heat: float in the lineup, pick a swell, paddle to match it,
  pop up, ride until it closes out or you wipe out, go again. Best two waves score.
- Carve, pump for speed, air off the lip, tricks, wipe out on bad landings.
- Chase camera, score and combos, pixel HUD, title screen, heat timer, results.
- Must run on a PS1 emulator, so: C on a PS1 homebrew SDK. The earlier
  Python/pygame/numba/trimesh requirements and the Pygame renderer are dropped.
- No copyrighted assets; all art generated in code.
- Worked on a feature branch, committed per milestone.

## Builder's calls (not yet confirmed by the user)

- Gravity 3.92 m/s^2 (40% of Earth). Heat length 180 s.
- After a wipeout or closeout, a short respawn in the lineup; no paddling back.
- A wipeout ends that wave and banks nothing from the unfinished combo. No fail state.
- No true barrel: a steep leaning wall with a pitching lip strip and spray.
- 320x240 NTSC, 30 FPS target. PS1 pad, digital and analog.
- No audio in this version.
- Toolchain: PSn00bSDK 0.24 in Docker. Test emulator: PCSX-Redux with OpenBIOS,
  because no Sony BIOS is available and none will be downloaded.

## How the PS1 look maps to hardware

Affine warping, integer vertex coordinates, 15-bit colour and ordered dithering are
native GPU behaviour; enable dithering on Gouraud primitives. Fog uses the GTE depth
cue blending to the sky colour, with a short draw distance. Textures are 64x64 at most
and generated in code.

## Coordinates and units

Sim uses Q12 fixed point (4096 = 1.0 metre or 1.0 unitless). Angles use 4096 per
full turn. +Y is up in sim space (negated at the render boundary), +Z toward shore,
swells travel +Z, and crests lie along X. Breaking peels along +X.

## Non-goals

Audio, true hollow barrels, multiplayer, memory-card saves, real hardware testing.

## Anti-goals

- Claiming it runs on PS1 without booting the disc image in an emulator.
- Floating point in game code (the PS1 has no FPU).
