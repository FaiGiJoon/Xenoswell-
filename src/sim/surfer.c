#include "sim/surfer.h"

#define PADDLE_ACCEL (3 * FX_ONE)
#define PADDLE_MAX (7 * FX_ONE)
#define CATCH_TOL (FX_ONE * 3 / 2)
#define CATCH_FACE (10 * FX_ONE)
#define POPUP_WINDOW (FX_ONE * 4 / 5)
#define OLLIE_VY (5 * FX_ONE)
#define WIPEOUT_TIME (FX_ONE * 3 / 2)
#define RESPAWN_TIME FX_ONE
#define CATCH_COOLDOWN (FX_ONE * 3 / 2)
#define PADDLE_LIMIT_Z (40 * FX_ONE)
#define CLEAN_ANGLE 341
#define SKETCHY_ANGLE 683

void surfer_init(Surfer *s) { *s = (Surfer){0}; }

static void to_lineup(Surfer *s) {
	s->state = S_LINEUP;
	s->board = (Board){0};
}

static void stop(Surfer *s, SurfState next, fx time) {
	s->state = next;
	s->timer = time;
	s->board.vel = (Vec3){0, 0, 0};
	s->board.spin_rate = 0;
	s->board.airborne = 0;
}

static void hold_surface(Board *b, const Ocean *o) {
	b->airborne = 0;
	b->pos.y = ocean_height(o, b->pos.x, b->pos.z);
}

static void enter_air(Surfer *s) {
	s->state = S_AIR;
	s->air_spin = 0;
	s->flipped = 0;
}

static uint32_t land(Surfer *s) {
	Board *b = &s->board;
	int32_t err = (b->yaw - fx_atan2(b->vel.x, b->vel.z)) & 4095;
	if (err > 2048) err = 4096 - err;
	if (err > 1024) { /* landed switch: the other end now leads */
		err = 2048 - err;
		b->yaw = (b->yaw + 2048) & 4095;
	}
	b->spin_rate = 0;
	if (err > SKETCHY_ANGLE) { stop(s, S_WIPEOUT, WIPEOUT_TIME); return EV_WIPEOUT; }
	s->state = S_RIDE;
	if (err <= CLEAN_ANGLE) return EV_LAND_CLEAN;
	b->vel.x /= 2;
	b->vel.z /= 2;
	return EV_LAND_SKETCHY;
}

static uint32_t step_paddle(Surfer *s, const Ocean *o, const SurfInput *in, fx dt) {
	Board *b = &s->board;
	BoardInput bi = {in->steer, 0};
	if (in->paddle && board_speed(b) < PADDLE_MAX) {
		b->vel.x += fx_mul(fx_mul(fx_sin(b->yaw), PADDLE_ACCEL), dt);
		b->vel.z += fx_mul(fx_mul(fx_cos(b->yaw), PADDLE_ACCEL), dt);
	}
	board_step(b, o, &bi, dt);
	hold_surface(b, o);
	if (s->timer > 0) s->timer -= dt;
	const Swell *sw = ocean_swell_at(o, b->pos.z);
	if (sw) {
		fx d = b->pos.z - sw->z, diff = b->vel.z - sw->speed;
		if (s->timer <= 0 && d >= 0 && d <= CATCH_FACE && diff >= -CATCH_TOL && diff <= CATCH_TOL) {
			s->state = S_POPUP;
			s->timer = POPUP_WINDOW;
		}
	} else if (b->pos.z > PADDLE_LIMIT_Z) {
		stop(s, S_RESPAWN, RESPAWN_TIME);
		return EV_MISSED;
	}
	return 0;
}

static uint32_t step_ride(Surfer *s, const Ocean *o, const SurfInput *in, int action, fx dt) {
	Board *b = &s->board;
	BoardInput bi = {in->steer, in->pump};
	board_step(b, o, &bi, dt);
	const Swell *sw = ocean_swell_at(o, b->pos.z);
	if (!sw) { stop(s, S_RESPAWN, RESPAWN_TIME); return EV_CLOSEOUT; }
	if (ocean_is_broken(sw, b->pos.x)) { stop(s, S_WIPEOUT, WIPEOUT_TIME); return EV_WIPEOUT; }
	if (!b->airborne && action) {
		b->vel.y += OLLIE_VY;
		b->airborne = 1;
	}
	if (b->airborne) enter_air(s);
	return 0;
}

static uint32_t step_air_state(Surfer *s, const Ocean *o, const SurfInput *in, int flip, fx dt) {
	Board *b = &s->board;
	BoardInput bi = {0, 0};
	b->spin_rate = in->spin * FX_ONE;
	fx turned = fx_mul(b->spin_rate, dt);
	s->air_spin += turned;
	if (flip) s->flipped = 1;
	board_step(b, o, &bi, dt);
	return b->airborne ? 0 : land(s);
}

uint32_t surfer_step(Surfer *s, const Ocean *o, const SurfInput *in, fx dt) {
	int action = in->action && !s->prev_action;
	int flip = in->flip && !s->prev_flip;
	uint32_t ev = 0;
	s->prev_action = in->action;
	s->prev_flip = in->flip;

	switch (s->state) {
	case S_LINEUP:
		hold_surface(&s->board, o);
		if (in->paddle) s->state = S_PADDLE;
		break;
	case S_PADDLE:
		ev = step_paddle(s, o, in, dt);
		break;
	case S_POPUP: {
		BoardInput bi = {0, 0};
		board_step(&s->board, o, &bi, dt);
		hold_surface(&s->board, o);
		s->timer -= dt;
		if (action) { s->state = S_RIDE; ev = EV_CAUGHT; }
		else if (s->timer <= 0) { s->state = S_PADDLE; s->timer = CATCH_COOLDOWN; ev = EV_MISSED; }
		break;
	}
	case S_RIDE:
		ev = step_ride(s, o, in, action, dt);
		break;
	case S_AIR:
		ev = step_air_state(s, o, in, flip, dt);
		break;
	case S_WIPEOUT:
		hold_surface(&s->board, o);
		s->timer -= dt;
		if (s->timer <= 0) stop(s, S_RESPAWN, RESPAWN_TIME);
		break;
	case S_RESPAWN:
		hold_surface(&s->board, o);
		s->timer -= dt;
		if (s->timer <= 0) {
			to_lineup(s);
			hold_surface(&s->board, o);
		}
		break;
	}
	return ev;
}
