#ifndef PS1_WATER_H
#define PS1_WATER_H
#include "sim/ocean.h"

/* Draws the sea around center, which should lie ahead of the camera. */
void water_draw(const Ocean *o, const Vec3 *center);

#endif
