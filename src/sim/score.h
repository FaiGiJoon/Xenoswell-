#ifndef SIM_SCORE_H
#define SIM_SCORE_H
#include "sim/surfer.h"

typedef struct {
	fx time_left, chain_timer, ride_accum;
	int32_t wave_score, chain, best[2];
	int mult, over;
} Heat;

void heat_init(Heat *h);
/* Call before heat_on_events for the same landing, so a sketchy landing halves it. */
void heat_on_trick(Heat *h, int32_t spin_abs, int flipped);
void heat_on_events(Heat *h, uint32_t ev);
/* The combo window is held while airborne, so landing is what decides a chain. */
void heat_step(Heat *h, fx dt, int riding, int airborne);
int32_t heat_total(const Heat *h);

#endif
