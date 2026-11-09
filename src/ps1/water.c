#include "ps1/water.h"
#include "ps1/gfx.h"

#define N 16
#define VERTS ((N + 1) * (N + 1))
#define CELL (5 * FX_ONE)
#define FOG_START (22 * 256)
#define FOG_RANGE (28 * 256)
#define MAX_HEIGHT (7 * FX_ONE)

static Vec3 pos[VERTS];
static int sxy[VERTS], sz[VERTS];
static uint32_t col[VERTS];

static inline int mix(int a, int b, int t) { return a + (((b - a) * t) >> 8); }

static uint32_t mix_rgb(uint32_t a, uint32_t b, int t) {
	return GFX_RGB(mix(a >> 16 & 255, b >> 16 & 255, t), mix(a >> 8 & 255, b >> 8 & 255, t), mix(a & 255, b & 255, t));
}

static uint32_t base_colour(const Ocean *o, const Vec3 *p) {
	int t = (p->y + FX_ONE) >> 4;
	int range = (MAX_HEIGHT + FX_ONE) >> 4;
	t = t < 0 ? 0 : t > range ? 256 : (t << 8) / range;
	uint32_t c = t < 128
		? mix_rgb(GFX_RGB(44, 18, 128), GFX_RGB(0, 130, 170), t * 2)
		: mix_rgb(GFX_RGB(0, 130, 170), GFX_RGB(70, 255, 200), (t - 128) * 2);
	const Swell *s = ocean_swell_at(o, p->z);
	if (s && ocean_is_broken(s, p->x) && p->y > FX_ONE) c = GFX_RGB(236, 244, 255);
	return c;
}

static uint32_t fogged(uint32_t c, int depth) {
	int f = depth - FOG_START;
	if (f <= 0) return c;
	if (f >= FOG_RANGE) return SKY_HORIZON;
	return mix_rgb(c, SKY_HORIZON, (f << 8) / FOG_RANGE);
}

static void draw_lip(const Ocean *o, const Swell *s, fx x0, fx z_centre) {
	if (s->z < z_centre - 45 * FX_ONE || s->z > z_centre + 45 * FX_ONE) return;
	for (int i = 0; i < N; i++) {
		fx xa = x0 + i * CELL, xb = xa + CELL;
		fx ha = ocean_height(o, xa, s->z), hb = ocean_height(o, xb, s->z);
		fx drop = FX_ONE * 3 / 2, reach = 3 * FX_ONE;
		Vec3 q[4] = {
			{xa, ha + FX_ONE / 4, s->z}, {xb, hb + FX_ONE / 4, s->z},
			{xa, ha - drop, s->z + reach}, {xb, hb - drop, s->z + reach},
		};
		uint32_t c = ocean_is_broken(s, xa) ? GFX_RGB(240, 246, 255) : GFX_RGB(110, 255, 214);
		gfx_quad(q, c, 0);
	}
}

static void draw_spray(const Ocean *o, const Swell *s) {
	uint32_t r = (uint32_t)(o->t >> 8) * 2654435761u;
	for (int i = 0; i < 16; i++) {
		r = r * 1664525u + 1013904223u;
		Vec3 p = {s->break_x + (fx)(r >> 20) * 2 - 8 * FX_ONE, 0, s->z + (fx)(r >> 22 & 1023) * 8};
		p.y = ocean_height(o, p.x, s->z) + (fx)(r >> 12 & 4095) * 2;
		int xy, z;
		gfx_transform(&p, 1, &xy, &z);
		if (z < 128) continue;
		int x = (short)(xy & 0xffff), y = (short)(xy >> 16);
		if (x < 0 || x >= 320 || y < 0 || y >= 240) continue;
		TILE *t = gfx_alloc(sizeof(TILE));
		if (!t) return;
		setTile(t);
		setXY0(t, x, y);
		setWH(t, 2, 2);
		setRGB0(t, 240, 248, 255);
		addPrim(gfx_ot(gfx_depth_index(z)), t);
	}
}

void water_draw(const Ocean *o, const Vec3 *c) {
	/* Snapping to the grid keeps vertices from sliding across the swell as the camera moves. */
	fx x0 = (c->x / CELL) * CELL - (N / 2) * CELL;
	fx z0 = (c->z / CELL) * CELL - (N / 2) * CELL;

	for (int j = 0, k = 0; j <= N; j++)
		for (int i = 0; i <= N; i++, k++) {
			pos[k].x = x0 + i * CELL;
			pos[k].z = z0 + j * CELL;
			pos[k].y = ocean_height(o, pos[k].x, pos[k].z);
		}
	gfx_transform(pos, VERTS, sxy, sz);
	for (int k = 0; k < VERTS; k++) col[k] = fogged(base_colour(o, &pos[k]), sz[k]);

	for (int j = 0; j < N; j++)
		for (int i = 0; i < N; i++) {
			int a = j * (N + 1) + i, b = a + 1, cc = a + N + 1, d = cc + 1;
			int qxy[4] = {sxy[a], sxy[b], sxy[cc], sxy[d]};
			int qz[4] = {sz[a], sz[b], sz[cc], sz[d]};
			if (qz[0] < GFX_CLIP_SZ || qz[1] < GFX_CLIP_SZ || qz[2] < GFX_CLIP_SZ || qz[3] < GFX_CLIP_SZ) {
				const Vec3 ring[4] = {pos[a], pos[b], pos[d], pos[cc]};
				const uint32_t cols[4] = {col[a], col[b], col[d], col[cc]};
				gfx_quad_clipped(ring, cols);
				continue;
			}
			if (!gfx_quad_ok(qxy, qz)) continue;
			POLY_G4 *q = gfx_alloc(sizeof(POLY_G4));
			if (!q) return;
			setPolyG4(q);
			setRGB0(q, col[a] >> 16, col[a] >> 8, col[a]);
			setRGB1(q, col[b] >> 16, col[b] >> 8, col[b]);
			setRGB2(q, col[cc] >> 16, col[cc] >> 8, col[cc]);
			setRGB3(q, col[d] >> 16, col[d] >> 8, col[d]);
			setXY4(q, (short)qxy[0], qxy[0] >> 16, (short)qxy[1], qxy[1] >> 16,
			       (short)qxy[2], qxy[2] >> 16, (short)qxy[3], qxy[3] >> 16);
			addPrim(gfx_ot(gfx_depth_index((qz[0] + qz[1] + qz[2] + qz[3]) >> 2)), q);
		}

	for (int i = 0; i < 3; i++) {
		const Swell *s = &o->swells[i];
		if (!s->alive) continue;
		draw_lip(o, s, x0, c->z);
		if (s->z > 20 * FX_ONE) draw_spray(o, s);
	}
}
