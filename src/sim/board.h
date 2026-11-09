#ifndef SIM_BOARD_H
#define SIM_BOARD_H
#include "sim/ocean.h"

#define G 16056 /* 3.92 m/s^2 */

typedef struct { Vec3 pos, vel; int32_t yaw, spin_rate; int airborne; fx pump_cd; } Board;
typedef struct { fx steer; int pump; } BoardInput;

void board_step(Board *b, const Ocean *o, const BoardInput *in, fx dt);
/* Horizontal speed. */
fx board_speed(const Board *b);

#endif
