#include "sim/chase.h"

#define NEAR_PLANE (FX_ONE / 2)
#define BACK_DIST (9 * FX_ONE)
#define HEIGHT (FX_ONE * 9 / 2)
#define LOOK_AHEAD (4 * FX_ONE)
#define EYE_RATE (5 * FX_ONE)
#define TARGET_RATE (8 * FX_ONE)
#define MIN_CLEARANCE FX_ONE

void chase_step(Vec3 *eye, Vec3 *target, const Board *b, const Ocean *o, fx dt) {
	fx dx, dz;
	if (board_speed(b) > FX_ONE) {
		int32_t a = fx_atan2(b->vel.x, b->vel.z);
		dx = fx_sin(a);
		dz = fx_cos(a);
	} else {
		dx = fx_sin(b->yaw);
		dz = fx_cos(b->yaw);
	}

	fx k = fx_mul(EYE_RATE, dt);
	eye->x += fx_mul(b->pos.x - fx_mul(dx, BACK_DIST) - eye->x, k);
	eye->y += fx_mul(b->pos.y + HEIGHT - eye->y, k);
	eye->z += fx_mul(b->pos.z - fx_mul(dz, BACK_DIST) - eye->z, k);
	fx floor_y = ocean_height(o, eye->x, eye->z) + MIN_CLEARANCE;
	if (eye->y < floor_y) eye->y = floor_y;

	k = fx_mul(TARGET_RATE, dt);
	target->x += fx_mul(b->pos.x + fx_mul(dx, LOOK_AHEAD) - target->x, k);
	target->y += fx_mul(b->pos.y + FX_ONE - target->y, k);
	target->z += fx_mul(b->pos.z + fx_mul(dz, LOOK_AHEAD) - target->z, k);
}

int chase_quad_visible(fx z0, fx z1, fx z2, fx z3) {
	return z0 >= NEAR_PLANE && z1 >= NEAR_PLANE && z2 >= NEAR_PLANE && z3 >= NEAR_PLANE;
}
