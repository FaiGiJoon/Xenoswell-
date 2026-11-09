#include "sim/score.h"

#define HEAT_TIME (180 * FX_ONE)
#define CHAIN_WINDOW (3 * FX_ONE)
#define MAX_MULT 5
#define RIDE_POINTS_PER_SEC 10

void heat_init(Heat *h) {
	*h = (Heat){0};
	h->time_left = HEAT_TIME;
}

static void bank_chain(Heat *h) {
	h->wave_score += h->chain;
	h->chain = 0;
	h->mult = 0;
	h->chain_timer = 0;
}

static void end_wave(Heat *h) {
	int32_t w = h->wave_score;
	h->wave_score = 0;
	if (w > h->best[0]) { h->best[1] = h->best[0]; h->best[0] = w; }
	else if (w > h->best[1]) h->best[1] = w;
}

void heat_on_trick(Heat *h, int32_t spin_abs, int flipped) {
	static const int32_t SPIN_POINTS[4] = {0, 500, 1200, 2000};
	int32_t halves = (spin_abs + 512) / 2048;
	int32_t pts = SPIN_POINTS[halves > 3 ? 3 : halves] + (flipped ? 1500 : 0);
	if (!pts) return;
	h->mult = h->chain_timer > 0 ? (h->mult < MAX_MULT ? h->mult + 1 : MAX_MULT) : 1;
	h->chain += pts * h->mult;
	h->chain_timer = CHAIN_WINDOW;
}

void heat_on_events(Heat *h, uint32_t ev) {
	if (ev & EV_LAND_SKETCHY) h->chain /= 2;
	if (ev & EV_WIPEOUT) {
		h->chain = 0;
		h->mult = 0;
		h->chain_timer = 0;
		end_wave(h);
	}
	if (ev & EV_CLOSEOUT) {
		bank_chain(h);
		end_wave(h);
	}
}

void heat_step(Heat *h, fx dt, int riding, int airborne) {
	if (h->over) return;
	if (riding) {
		h->ride_accum += dt;
		if (h->ride_accum >= FX_ONE) {
			h->ride_accum -= FX_ONE;
			h->wave_score += RIDE_POINTS_PER_SEC;
		}
	}
	if (h->chain_timer > 0 && !airborne) {
		h->chain_timer -= dt;
		if (h->chain_timer <= 0) bank_chain(h);
	}
	h->time_left -= dt;
	if (h->time_left <= 0) {
		h->time_left = 0;
		bank_chain(h);
		end_wave(h);
		h->over = 1;
	}
}

int32_t heat_total(const Heat *h) { return h->best[0] + h->best[1]; }
