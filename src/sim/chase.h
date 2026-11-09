#ifndef SIM_CHASE_H
#define SIM_CHASE_H
#include "sim/board.h"

void chase_step(Vec3 *eye, Vec3 *target, const Board *b, const Ocean *o, fx dt);
/* Depths are view-space, Q12 metres. False if any corner is nearer than 0.5 m. */
int chase_quad_visible(fx z0, fx z1, fx z2, fx z3);

#endif
