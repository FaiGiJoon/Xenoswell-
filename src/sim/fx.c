#include "sim/fx.h"
#include "sim/sin_table.h"

fx fx_mul(fx a, fx b) { return (fx)(((int64_t)a * b) >> 12); }

fx fx_div(fx a, fx b) { return b ? (fx)(((int64_t)a * FX_ONE) / b) : 0; }

fx fx_sin(int32_t angle) {
	int32_t a = angle & 4095;
	if (a < 1024) return SIN_Q[a];
	if (a < 2048) return SIN_Q[2048 - a];
	if (a < 3072) return -SIN_Q[a - 2048];
	return -SIN_Q[4096 - a];
}

fx fx_cos(int32_t angle) { return fx_sin(angle + 1024); }

static uint32_t isqrt64(uint64_t n) {
	uint64_t res = 0, bit = (uint64_t)1 << 62;
	while (bit > n) bit >>= 2;
	while (bit) {
		if (n >= res + bit) { n -= res + bit; res = (res >> 1) + bit; }
		else res >>= 1;
		bit >>= 2;
	}
	return (uint32_t)res;
}

fx fx_sqrt(fx a) { return a <= 0 ? 0 : (fx)isqrt64((uint64_t)a << 12); }

fx fx_hyp(fx a, fx b) {
	return (fx)isqrt64((uint64_t)((int64_t)a * a) + (uint64_t)((int64_t)b * b));
}

int32_t fx_atan2(fx y, fx x) {
	fx ax = x < 0 ? -x : x, ay = y < 0 ? -y : y;
	int32_t a;
	if (ax == 0 && ay == 0) return 0;
	if (ax >= ay) {
		fx r = fx_div(ay, ax);
		a = fx_mul(r, 512 + fx_mul(FX_ONE - r, 178));
	} else {
		fx r = fx_div(ax, ay);
		a = 1024 - fx_mul(r, 512 + fx_mul(FX_ONE - r, 178));
	}
	if (x < 0) a = 2048 - a;
	return y < 0 ? -a : a;
}

fx v3_dot(const Vec3 *a, const Vec3 *b) {
	return fx_mul(a->x, b->x) + fx_mul(a->y, b->y) + fx_mul(a->z, b->z);
}

void v3_norm(Vec3 *v) {
	uint64_t s = (uint64_t)((int64_t)v->x * v->x) + (uint64_t)((int64_t)v->y * v->y) + (uint64_t)((int64_t)v->z * v->z);
	fx len = (fx)isqrt64(s);
	if (!len) return;
	v->x = fx_div(v->x, len);
	v->y = fx_div(v->y, len);
	v->z = fx_div(v->z, len);
}
