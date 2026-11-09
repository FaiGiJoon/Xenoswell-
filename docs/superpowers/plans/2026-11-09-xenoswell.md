# Xenoswell Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A bootable PS1 disc image of an alien-planet surfing simulation with timed heats.

**Architecture:** `src/sim/` is pure fixed-point C with no PS1 headers, compiled both for PS1 and natively for tests. `src/ps1/` owns GTE, GPU and pad access. `src/game/` connects them with screens.

**Tech Stack:** C11, PSn00bSDK 0.24 + GCC 12.3 mipsel in Docker (linux/amd64), host clang for tests, PCSX-Redux + OpenBIOS for emulation.

**Spec:** `docs/superpowers/specs/2026-11-09-xenoswell-design.md`

## Global Constraints

- No `float` or `double` anywhere under `src/`. Check: `grep -rnE '\b(float|double)\b' src` is empty.
- `src/sim/` includes only `<stdint.h>` and `sim/*.h`.
- `typedef int32_t fx;`, `FX_ONE 4096`, angles 4096 per turn, `FX_DT 137` (1/30 s).
- Gravity `G 16056` (3.92 m/s^2). Heat 180 s. Display 320x240 NTSC.
- Target 30 FPS. Budget: at most 600 water triangles per frame.
- All art generated in code. No Sony BIOS or SDK-external assets committed.

## Review Focus

- A swell that dies while the player rides it: ride ends as closeout, wave score banked (test in Task 5).
- Timer expiring mid-ride: the current wave is banked before results (test in Task 6).
- Heat with zero or one scored wave: results total is valid, no garbage slot (test in Task 6).
- Fixed-point overflow at far positions and large swells: `fx_mul` is 64-bit, height stays bounded (test in Task 3).
- Water vertices behind the camera or at the near plane: quad is rejected, no screen-spanning polygons (test in Task 8).

---

### Task 1: Toolchain, repo, booting disc

**Files:** Create `Dockerfile`, `tools/build.sh`, `tools/run_emu.sh`, `CMakeLists.txt`, `iso.xml`, `system.cnf`, `src/main.c`, `Makefile`, `.gitignore`

- [ ] Git init, commit docs on `main`, create worktree on `feature/xenoswell`.
- [ ] `Dockerfile` (`--platform linux/amd64`) unpacks `gcc-mipsel-none-elf-12.3.0-linux.zip` and `PSn00bSDK-0.24-Linux.zip` from release v0.24. `tools/build.sh` runs CMake in it.
- [ ] `src/main.c`: init display, clear to violet, print `XENOSWELL BOOT OK` on TTY.
- [ ] Run `tools/build.sh`. Expected: `build/xenoswell.cue` and `.bin` exist.
- [ ] Install PCSX-Redux outside the repo; `tools/run_emu.sh` boots the cue with OpenBIOS.
- [ ] Expected: the emulator shows the violet screen and TTY prints the boot line. Capture a screenshot.
- [ ] Commit `build: PS1 toolchain and booting disc image`.

### Task 2: Fixed-point math

**Files:** Create `src/sim/fx.h`, `src/sim/fx.c`, `tests/test_fx.c`, `tests/t.h`. `make test` builds every `tests/test_*.c` with host clang.

**Interfaces — Produces:** `fx fx_mul(fx,fx)`, `fx fx_div(fx,fx)`, `fx fx_sin(int32_t a)`, `fx fx_cos(int32_t a)`, `fx fx_sqrt(fx)`, `int32_t fx_atan2(fx y, fx x)`, `typedef struct {fx x,y,z;} Vec3`, `fx v3_dot(const Vec3*,const Vec3*)`, `void v3_norm(Vec3*)`.

- [ ] Tests: `fx_mul(3*FX_ONE, FX_ONE/2)==6144`; `fx_mul(100000*FX_ONE, 2*FX_ONE)==200000*FX_ONE`; `fx_sin(1024)==4096`, `fx_sin(-1024)==-4096`, `fx_sin(4096+1024)==4096`; `fx_cos(0)==4096`; `fx_sqrt(4*FX_ONE)` within 2 of 8192; `fx_sqrt(0)==0`; `fx_atan2(FX_ONE,0)` within 4 of 1024; `v3_norm` of (3,0,4) within 8 of (2458,0,3277).
- [ ] Run, see failure. Implement with a generated 1025-entry quarter-sine table. Run, see pass. Commit.

### Task 3: Ocean

**Files:** Create `src/sim/ocean.h/.c`, `tests/test_ocean.c`

**Interfaces — Produces:** `Swell {fx z, amp, speed, break_x, peel; int alive;}`, `Ocean {Swell swells[3]; fx t, spawn_timer; uint32_t rng;}`, `ocean_init(Ocean*, uint32_t seed)`, `ocean_step(Ocean*, fx dt)`, `fx ocean_height(const Ocean*, fx x, fx z)`, `ocean_normal(const Ocean*, fx x, fx z, Vec3*)`, `const Swell* ocean_swell_at(const Ocean*, fx z)`, `int ocean_is_broken(const Swell*, fx x)`.

Height = two small sine chop terms (total amplitude at most 0.4 m) + each live swell's raised-cosine profile (front half-width 10 m, back 22 m). Defaults: amp 6 m, speed 7 m/s, peel 5 m/s, spawn every 14 s at z=-60, dies past z=180.

- [ ] Tests: crest within 0.5 m of amp on a single swell; deterministic for one seed; swell z advances 7 m ±0.05 in 30 steps; swell dead past 180; normal points up on flat water (`y>4000`) and tilts on the face; `ocean_is_broken` true behind the peel and false ahead; height at x and z of ±30000 m is within ±8 m.
- [ ] Run, fail, implement, pass, commit.

### Task 4: Board physics

**Files:** Create `src/sim/board.h/.c`, `tests/test_board.c`

**Interfaces — Produces:** `Board {Vec3 pos, vel; int32_t yaw, spin_rate; int airborne;}`, `BoardInput {fx steer; int pump;}`, `board_step(Board*, const Ocean*, const BoardInput*, fx dt)`, `fx board_speed(const Board*)`.

While on the surface: slope force along the tangent, dense-liquid drag, sideways grip that bends velocity toward yaw. Pumping adds 1.5 m/s only on a downhill face and has a 0.5 s cooldown. Leaves the surface when upward velocity exceeds the surface's by 1 m/s.

- [ ] Tests: free fall for 30 steps gives `vel.y` within 200 of `-G`; a board on the swell face gains speed; steering changes heading and keeps at least 85% of speed; pump on flat water adds nothing; pump cooldown blocks a second pump inside 0.5 s; launch at the lip sets `airborne`; speed never exceeds 24 m/s.
- [ ] Run, fail, implement, pass, commit.

### Task 5: Surfer state machine

**Files:** Create `src/sim/surfer.h/.c`, `tests/test_surfer.c`

**Interfaces — Produces:** `enum SurfState {S_LINEUP,S_PADDLE,S_POPUP,S_RIDE,S_AIR,S_WIPEOUT,S_RESPAWN}`, `SurfInput {fx steer; int paddle, pump, action; int spin;}`, `Surfer {SurfState state; Board board; fx timer; int32_t air_spin; int flipped;}`, `surfer_init(Surfer*)`, `uint32_t surfer_step(Surfer*, const Ocean*, const SurfInput*, fx dt)` returning `EV_CAUGHT|EV_LAND_CLEAN|EV_LAND_SKETCHY|EV_WIPEOUT|EV_CLOSEOUT|EV_MISSED`.

Catch requires paddle speed within 1.5 m/s of swell speed on its face, then `action` within a 0.8 s window. Landing error is the yaw-to-velocity angle: at most 341 clean, at most 683 sketchy (speed halved), otherwise wipeout. Wipeout 1.5 s then respawn 1 s.

- [ ] Tests, one per transition: lineup to paddle; paddling too slowly misses; catch then `action` reaches `S_RIDE` with `EV_CAUGHT`; missed window returns to paddle with `EV_MISSED`; clean, sketchy and wipeout at angles 300, 600, 1000; riding into broken water emits `EV_WIPEOUT`; **swell dying mid-ride emits `EV_CLOSEOUT`**.
- [ ] Run, fail, implement, pass, commit.

### Task 6: Scoring and heat

**Files:** Create `src/sim/score.h/.c`, `tests/test_score.c`

**Interfaces — Produces:** `Heat {fx time_left; int32_t wave_score, chain, best[2]; int mult; fx chain_timer; int over;}`, `heat_init(Heat*)`, `heat_on_trick(Heat*, int32_t spin_abs, int flipped)`, `heat_on_events(Heat*, uint32_t ev)`, `heat_step(Heat*, fx dt, int riding)`, `int32_t heat_total(const Heat*)`.

Spin values: 180 = 500, 360 = 1200, 540 = 2000; flip +1500. Multiplier +1 per trick in a chain within 3 s, cap 5. Chain banks on clean landing timeout. Wipeout voids the chain only; sketchy landing halves it.

- [ ] Tests: each spin value; flip adds; multiplier caps at 5; wipeout voids chain and keeps `wave_score`; a wave ending keeps the top two scores in `best` sorted; **timer expiring mid-ride banks the wave and sets `over`**; **zero waves totals 0, one wave totals that wave**.
- [ ] Run, fail, implement, pass, commit.

### Task 7: PS1 graphics layer

**Files:** Create `src/ps1/gfx.h/.c`, `src/ps1/font.h/.c`; modify `CMakeLists.txt`, `src/main.c`

**Interfaces — Produces:** `gfx_init()`, `gfx_begin()`, `gfx_end()`, `gfx_text(int x, int y, const char*, uint32_t rgb)`, `gfx_set_camera(const Vec3* eye, const Vec3* target)`, `gfx_fps()`. Double-buffered 320x240, ordering table of 1024, 64 KB packet buffers per frame, GPU dither on. Font is an 8x8 glyph table in code uploaded to VRAM.

- [ ] Show text in the test disc. Build and boot in the emulator. Expected: text is legible. Screenshot and commit.

### Task 8: Water, models, chase camera

**Files:** Create `src/sim/chase.h/.c`, `tests/test_chase.c`, `src/ps1/water.h/.c`, `src/ps1/models.h/.c`, `src/ps1/sky.h/.c`

**Interfaces — Produces:** `chase_step(Vec3* eye, Vec3* target, const Board*, fx dt)`; `int chase_quad_visible(fx z0, fx z1, fx z2, fx z3)` (all view depths at or above near plane 0.5 m); `water_draw(const Ocean*, const Vec3* center)`; `model_draw_board(const Board*)`; `model_draw_surfer(const Surfer*)`; `sky_draw()`.

Water is a player-centred 16x16 quad grid (512 triangles) sampling `ocean_height`, glowing teal to violet by height, foam on broken water, GTE depth cue fog. Sky has a large ringed planet, two moons and stars.

- [ ] Tests: camera stays behind travel direction, smooths with no teleport over 1 step, stays above the water; **`chase_quad_visible` false when any depth is below 0.5 m**.
- [ ] Run, fail, implement, pass. Boot a scene in the emulator. Expected: moving swell with fog and at least 30 FPS from `gfx_fps`. Screenshot, commit.

### Task 9: Playable game

**Files:** Create `src/ps1/pad.h/.c`, `src/game/game.h/.c`, `src/game/hud.c`; modify `src/main.c`

**Interfaces:** `pad_read(SurfInput*, int* start)`; `game_run()`. Layout: D-pad or left stick steers, X paddle/pump, O pop-up/ollie, L1/R1 spin, Triangle flip. HUD: timer, score, multiplier, speed, best two waves. Screens: title, heat, results; Start advances.

- [ ] Boot and scripted-input run in the emulator through catch, ride and wipeout. Expected: TTY event log shows the sequence, FPS printed. Commit.

### Task 10: Verification and finish

**Files:** Create `README.md`.

- [ ] `make test` all pass; float grep empty; clean Docker build.
- [ ] Emulator: full heat title to results. Report measured FPS and what was seen.
- [ ] Code review, finish the branch, open the folder in Finder.
