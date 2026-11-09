#include "sim/board.h"

#define TURN_RATE 1000
#define DRAG 492 /* 0.12 /s, relative to the moving liquid */
#define GRIP (4 * FX_ONE)
#define PUMP_BOOST (FX_ONE * 3 / 2)
#define PUMP_COOLDOWN (FX_ONE / 2)
#define PUMP_MIN_SLOPE 614
#define MAX_SPEED (24 * FX_ONE)
#define LAUNCH_DELTA FX_ONE

fx board_speed(const Board *b) { return fx_hyp(b->vel.x, b->vel.z); }

static void scale_speed(Board *b, fx from, fx to) {
	if (from <= 0) return;
	fx k = fx_div(to, from);
	b->vel.x = fx_mul(b->vel.x, k);
	b->vel.z = fx_mul(b->vel.z, k);
}

static void step_air(Board *b, const Ocean *o, fx dt) {
	b->vel.y -= fx_mul(G, dt);
	b->pos.x += fx_mul(b->vel.x, dt);
	b->pos.y += fx_mul(b->vel.y, dt);
	b->pos.z += fx_mul(b->vel.z, dt);
	fx h = ocean_height(o, b->pos.x, b->pos.z);
	if (b->pos.y <= h) {
		b->pos.y = h;
		b->vel.y = 0;
		b->airborne = 0;
	}
}

void board_step(Board *b, const Ocean *o, const BoardInput *in, fx dt) {
	b->yaw = (b->yaw + fx_mul(b->spin_rate, dt)) & 4095;
	if (b->pump_cd > 0) b->pump_cd -= dt;
	if (b->airborne) { step_air(b, o, dt); return; }

	b->yaw = (b->yaw + fx_mul(fx_mul(in->steer, TURN_RATE), dt)) & 4095;

	Vec3 n;
	ocean_normal(o, b->pos.x, b->pos.z, &n);
	b->vel.x += fx_mul(fx_mul(G, n.x), dt);
	b->vel.z += fx_mul(fx_mul(G, n.z), dt);

	/* The dense liquid is carried along by the swell and drags the board with it. */
	fx current = 0;
	const Swell *s = ocean_swell_at(o, b->pos.z);
	if (s) {
		fx frac = fx_div(b->pos.y, s->amp);
		if (frac < 0) frac = 0;
		if (frac > FX_ONE) frac = FX_ONE;
		current = fx_mul(s->speed, frac);
	}
	b->vel.x -= fx_mul(fx_mul(b->vel.x, DRAG), dt);
	b->vel.z -= fx_mul(fx_mul(b->vel.z - current, DRAG), dt);

	/* Fins bend the velocity toward the board's heading. */
	fx speed = board_speed(b);
	fx k = fx_mul(GRIP, dt);
	b->vel.x += fx_mul(fx_mul(fx_sin(b->yaw), speed) - b->vel.x, k);
	b->vel.z += fx_mul(fx_mul(fx_cos(b->yaw), speed) - b->vel.z, k);
	scale_speed(b, board_speed(b), speed);

	if (in->pump && b->pump_cd <= 0 && fx_hyp(n.x, n.z) >= PUMP_MIN_SLOPE
	    && fx_mul(b->vel.x, n.x) + fx_mul(b->vel.z, n.z) > 0) {
		speed = board_speed(b);
		scale_speed(b, speed, speed + PUMP_BOOST);
		b->pump_cd = PUMP_COOLDOWN;
	}
	speed = board_speed(b);
	if (speed > MAX_SPEED) scale_speed(b, speed, MAX_SPEED);

	b->pos.x += fx_mul(b->vel.x, dt);
	b->pos.z += fx_mul(b->vel.z, dt);
	fx h = ocean_height(o, b->pos.x, b->pos.z);
	fx surface_vy = fx_div(h - b->pos.y, dt);
	if (b->vel.y > 0 && b->vel.y - surface_vy > LAUNCH_DELTA) {
		b->airborne = 1;
		b->pos.y += fx_mul(b->vel.y, dt);
		return;
	}
	b->pos.y = h;
	b->vel.y = surface_vy;
}
