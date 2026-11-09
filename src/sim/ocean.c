#include "sim/ocean.h"

#define FRONT (10 * FX_ONE)
#define BACK (22 * FX_ONE)
#define GRACE (8 * FX_ONE) /* how far ahead of the face a rider still belongs to it */
#define SPAWN_Z (-60 * FX_ONE)
#define DIE_Z (180 * FX_ONE)
#define PEEL_START_Z (20 * FX_ONE)
#define SPAWN_EVERY (14 * FX_ONE)

static uint32_t next_rand(Ocean *o) {
	uint32_t x = o->rng ? o->rng : 1;
	x ^= x << 13; x ^= x >> 17; x ^= x << 5;
	return o->rng = x;
}

static void spawn(Ocean *o, Swell *s, fx z) {
	s->z = z;
	s->amp = 5 * FX_ONE + (fx)(next_rand(o) % (2 * FX_ONE));
	s->speed = 7 * FX_ONE;
	s->break_x = -60 * FX_ONE;
	s->peel = 5 * FX_ONE;
	s->alive = 1;
}

void ocean_init(Ocean *o, uint32_t seed) {
	*o = (Ocean){0};
	o->rng = seed;
	o->spawn_timer = SPAWN_EVERY;
	spawn(o, &o->swells[0], -30 * FX_ONE);
}

void ocean_step(Ocean *o, fx dt) {
	o->t += dt;
	for (int i = 0; i < 3; i++) {
		Swell *s = &o->swells[i];
		if (!s->alive) continue;
		s->z += fx_mul(s->speed, dt);
		if (s->z > PEEL_START_Z) s->break_x += fx_mul(s->peel, dt);
		if (s->z > DIE_Z) s->alive = 0;
	}
	o->spawn_timer -= dt;
	if (o->spawn_timer <= 0) {
		o->spawn_timer = SPAWN_EVERY;
		for (int i = 0; i < 3; i++)
			if (!o->swells[i].alive) { spawn(o, &o->swells[i], SPAWN_Z); break; }
	}
}

fx ocean_height(const Ocean *o, fx x, fx z) {
	fx h = fx_mul(819, fx_sin(x / 17 + fx_mul(o->t, 1229)))
	     + fx_mul(819, fx_sin(z / 23 + x / 41 - fx_mul(o->t, 900)));
	for (int i = 0; i < 3; i++) {
		const Swell *s = &o->swells[i];
		if (!s->alive) continue;
		fx d = z - s->z, w = d >= 0 ? FRONT : BACK;
		if (d >= w || d <= -w) continue;
		h += fx_mul(s->amp, (FX_ONE + fx_cos(fx_mul(fx_div(d, w), 2048))) / 2);
	}
	return h;
}

void ocean_normal(const Ocean *o, fx x, fx z, Vec3 *out) {
	const fx e = FX_ONE / 2;
	out->x = ocean_height(o, x - e, z) - ocean_height(o, x + e, z);
	out->y = 2 * e;
	out->z = ocean_height(o, x, z - e) - ocean_height(o, x, z + e);
	v3_norm(out);
}

const Swell *ocean_swell_at(const Ocean *o, fx z) {
	for (int i = 0; i < 3; i++) {
		const Swell *s = &o->swells[i];
		if (s->alive && z - s->z < FRONT + GRACE && z - s->z > -BACK) return s;
	}
	return 0;
}

int ocean_is_broken(const Swell *s, fx x) { return x < s->break_x; }
