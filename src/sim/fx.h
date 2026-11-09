#ifndef SIM_FX_H
#define SIM_FX_H
#include <stdint.h>

typedef int32_t fx;
#define FX_ONE 4096
#define FX_DT 137 /* 1/30 s */

typedef struct { fx x, y, z; } Vec3;

fx fx_mul(fx a, fx b);
fx fx_div(fx a, fx b);
fx fx_sin(int32_t angle);
fx fx_cos(int32_t angle);
fx fx_sqrt(fx a);
/* Length of (a, b). */
fx fx_hyp(fx a, fx b);
/* Angle of (x, y), 4096 per turn, in [-2048, 2048]. */
int32_t fx_atan2(fx y, fx x);
fx v3_dot(const Vec3 *a, const Vec3 *b);
void v3_norm(Vec3 *v);

#endif
