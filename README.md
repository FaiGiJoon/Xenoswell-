# Xenoswell

A surfing simulation on an alien ocean, written in C as a real PlayStation 1 program.
Low gravity, dense glowing liquid, huge slow swells. A heat lasts 3 minutes and your
best two waves count.

## Play

Load `build/xenoswell.cue` in a PS1 emulator. Build it first if `build/` is missing.

| Button | Action |
|---|---|
| D-pad / left stick | Steer |
| X | Paddle, pump on a wave face |
| O | Stand up when prompted, ollie while riding |
| L1 / R1 | Spin in the air |
| Triangle | Flip in the air |
| Start | Start / continue |
| Select (title) | Watch the demo rider |

Hold X as a swell nears, match its speed, press O when told to, and ride along the
wave ahead of the breaking section. Spins of 180 degrees or more can land either way round.

## Build

Needs Docker. `sh tools/build.sh` creates the disc image in `build/`.

`make test` runs the simulation tests natively with clang.

## Layout

- `src/sim/`: fixed-point game logic with no PS1 dependencies, tested on the host
- `src/ps1/`: GTE, GPU, pad, water, models, sky
- `src/game/`: screens, HUD, demo rider
- `tools/redux_probe.lua`: PCSX-Redux script for automated screenshots

All art is generated in code.
