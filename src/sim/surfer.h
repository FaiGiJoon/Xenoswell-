#ifndef SIM_SURFER_H
#define SIM_SURFER_H
#include "sim/board.h"

typedef enum { S_LINEUP, S_PADDLE, S_POPUP, S_RIDE, S_AIR, S_WIPEOUT, S_RESPAWN } SurfState;

enum {
	EV_CAUGHT = 1, EV_LAND_CLEAN = 2, EV_LAND_SKETCHY = 4,
	EV_WIPEOUT = 8, EV_CLOSEOUT = 16, EV_MISSED = 32
};

/* action and flip are level inputs; the surfer detects the press edge. spin is -1, 0 or 1. */
typedef struct { fx steer; int paddle, pump, action, spin, flip; } SurfInput;

typedef struct {
	SurfState state;
	Board board;
	fx timer;
	int32_t air_spin;
	int flipped, prev_action, prev_flip;
} Surfer;

void surfer_init(Surfer *s);
uint32_t surfer_step(Surfer *s, const Ocean *o, const SurfInput *in, fx dt);

#endif
