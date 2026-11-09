#ifndef SIM_OCEAN_H
#define SIM_OCEAN_H
#include "sim/fx.h"

typedef struct { fx z, amp, speed, break_x, peel; int alive; } Swell;
typedef struct { Swell swells[3]; fx t, spawn_timer; uint32_t rng; } Ocean;

void ocean_init(Ocean *o, uint32_t seed);
void ocean_step(Ocean *o, fx dt);
fx ocean_height(const Ocean *o, fx x, fx z);
void ocean_normal(const Ocean *o, fx x, fx z, Vec3 *out);
/* The live swell covering z, or 0. */
const Swell *ocean_swell_at(const Ocean *o, fx z);
int ocean_is_broken(const Swell *s, fx x);

#endif
