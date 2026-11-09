#include "ps1/sky.h"
#include "ps1/gfx.h"

#define SLOT_GRADIENT (OT_LEN - 1)
#define SLOT_STARS (OT_LEN - 2)
#define SLOT_BODIES (OT_LEN - 3)
#define SLOT_RING (OT_LEN - 4)
#define STARS 48

static void gradient(int top, int bottom, uint32_t c0, uint32_t c1) {
	POLY_G4 *q = gfx_alloc(sizeof(POLY_G4));
	if (!q) return;
	setPolyG4(q);
	setRGB0(q, c0 >> 16, c0 >> 8, c0);
	setRGB1(q, c0 >> 16, c0 >> 8, c0);
	setRGB2(q, c1 >> 16, c1 >> 8, c1);
	setRGB3(q, c1 >> 16, c1 >> 8, c1);
	setXY4(q, 0, top, 320, top, 0, bottom, 320, bottom);
	addPrim(gfx_ot(SLOT_GRADIENT), q);
}

static void disc(int cx, int cy, int r, uint32_t rgb, int slot) {
	for (int i = 0; i < 12; i++) {
		int32_t a = i * 4096 / 12, b = (i + 1) * 4096 / 12;
		POLY_F3 *t = gfx_alloc(sizeof(POLY_F3));
		if (!t) return;
		setPolyF3(t);
		setRGB0(t, rgb >> 16, rgb >> 8, rgb);
		setXY3(t, cx, cy, cx + (fx_cos(a) * r >> 12), cy + (fx_sin(a) * r >> 12),
		       cx + (fx_cos(b) * r >> 12), cy + (fx_sin(b) * r >> 12));
		addPrim(gfx_ot(slot), t);
	}
}

static void ring(int cx, int cy, int r) {
	for (int i = 0; i < 20; i++) {
		int32_t a = i * 4096 / 20, b = (i + 1) * 4096 / 20;
		/* Only the half passing in front of the planet, plus the outer sides. */
		POLY_F4 *q = gfx_alloc(sizeof(POLY_F4));
		if (!q) return;
		int ax = fx_cos(a) * r >> 12, ay = fx_sin(a) * r >> 14;
		int bx = fx_cos(b) * r >> 12, by = fx_sin(b) * r >> 14;
		setPolyF4(q);
		setRGB0(q, 248, 214, 150);
		setXY4(q, cx + ax, cy + ay, cx + bx, cy + by, cx + ax * 3 / 4, cy + ay * 3 / 4, cx + bx * 3 / 4, cy + by * 3 / 4);
		addPrim(gfx_ot(fx_sin(a) > 0 ? SLOT_RING : SLOT_BODIES + 1), q);
	}
}

static void stars(int horizon) {
	uint32_t r = 77123;
	for (int i = 0; i < STARS; i++) {
		r = r * 1664525u + 1013904223u;
		Vec3 d = {(fx)(r >> 20) * 2 - 4096, (fx)(r >> 21 & 2047) + 800, (fx)(r >> 8 & 4095) * 2 - 4096};
		v3_norm(&d);
		int x, y;
		if (!gfx_project_dir(&d, &x, &y) || x < 0 || x >= 320 || y < 0 || y >= horizon) continue;
		TILE_1 *t = gfx_alloc(sizeof(TILE_1));
		if (!t) return;
		setTile1(t);
		setXY0(t, x, y);
		setRGB0(t, 230, 225, 255);
		addPrim(gfx_ot(SLOT_STARS), t);
	}
}

static void body(fx dx, fx dy, fx dz, int r, uint32_t rgb, int with_ring) {
	Vec3 d = {dx, dy, dz};
	int x, y;
	v3_norm(&d);
	if (!gfx_project_dir(&d, &x, &y) || x < -80 || x > 400 || y < -80 || y > 320) return;
	disc(x, y, r, rgb, SLOT_BODIES);
	if (with_ring) ring(x, y, r * 7 / 4);
}

void sky_draw(void) {
	const Vec3 *f = gfx_forward();
	Vec3 level = {f->x, 0, f->z};
	int x, horizon = 120;
	v3_norm(&level);
	gfx_project_dir(&level, &x, &horizon);
	if (horizon > 239) horizon = 239;
	if (horizon > 0) gradient(horizon - 200 < 0 ? 0 : horizon - 200, horizon, GFX_RGB(18, 8, 66), SKY_HORIZON);
	if (horizon > 0) stars(horizon);
	body(FX_ONE, 1500, FX_ONE * 3, 34, GFX_RGB(236, 150, 90), 1);
	body(FX_ONE * 3, 2000, FX_ONE, 12, GFX_RGB(180, 230, 240), 0);
	body(-FX_ONE * 2, 1400, FX_ONE * 3, 8, GFX_RGB(250, 240, 200), 0);
}
