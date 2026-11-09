#include "ps1/gfx.h"
#include <stdio.h>
#include <inline_c.h>
#include "ps1/font.h"
#include "sim/chase.h"

#define PACKET_BYTES 65536
#define FONT_X 640
#define FONT_Y 0
#define GLYPHS_PER_ROW 32

/* The SDK's GTE macros take pointers without declaring a memory read or write, so
   without this the optimiser drops or moves accesses to the structs they use. */
#define GTE_SYNC(ptr) __asm__ volatile("" : : "r"(ptr) : "memory")

typedef struct {
	DISPENV disp;
	DRAWENV draw;
	uint32_t ot[OT_LEN];
	uint8_t packets[PACKET_BYTES];
} Buffer;

static Buffer buffers[2];
static int current;
static uint8_t *next_packet;

static Vec3 cam_eye, cam_r, cam_u, cam_f;

static int last_vsync, fps_value = 30, fps_frames, fps_start, cpu_hblanks;

static uint16_t font_pixels[192 * 16] __attribute__((aligned(4)));

static void upload_font(void) {
	for (int i = 0; FONT_CHARS[i]; i++) {
		int ox = (i % GLYPHS_PER_ROW) * 6, oy = (i / GLYPHS_PER_ROW) * 8;
		for (int cx = 0; cx < 5; cx++)
			for (int cy = 0; cy < 7; cy++)
				if (FONT_COLS[i][cx] >> cy & 1)
					font_pixels[(oy + cy) * 192 + ox + cx] = 0x7fff;
	}
	RECT r = {FONT_X, FONT_Y, 192, 16};
	LoadImage(&r, (const uint32_t *)font_pixels);
	DrawSync(0);
}

void gfx_init(void) {
	ResetGraph(0);
	SetVideoMode(MODE_NTSC);
	for (int i = 0; i < 2; i++) {
		Buffer *b = &buffers[i];
		SetDefDispEnv(&b->disp, 0, i ? 240 : 0, 320, 240);
		SetDefDrawEnv(&b->draw, 0, i ? 0 : 240, 320, 240);
		setRGB0(&b->draw, 176, 72, 132);
		b->draw.isbg = 1;
		b->draw.dtd = 1;
	}
	InitGeom();
	gte_SetGeomOffset(160, 120);
	gte_SetGeomScreen(GFX_FOCAL);
	upload_font();
	SetDispMask(1);
	last_vsync = fps_start = VSync(-1);
}

void gfx_begin(void) {
	Buffer *b = &buffers[current];
	ClearOTagR(b->ot, OT_LEN);
	next_packet = b->packets;
}

void gfx_end(void) {
	Buffer *b = &buffers[current];
	cpu_hblanks = VSync(1);
	DrawSync(0);
	while (VSync(-1) - last_vsync < 2) VSync(0);
	last_vsync = VSync(-1);
	PutDispEnv(&b->disp);
	DrawOTagEnv(&b->ot[OT_LEN - 1], &b->draw);
	current ^= 1;

	fps_frames++;
	int elapsed = last_vsync - fps_start;
	if (elapsed >= 60) {
		fps_value = fps_frames * 60 / elapsed;
		fps_frames = 0;
		fps_start = last_vsync;
	}
}

int gfx_fps(void) { return fps_value; }
int gfx_cpu_hblanks(void) { return cpu_hblanks; }

void gfx_set_camera(const Vec3 *eye, const Vec3 *target) {
	cam_eye = *eye;
	cam_f = (Vec3){target->x - eye->x, target->y - eye->y, target->z - eye->z};
	v3_norm(&cam_f);
	cam_r = (Vec3){cam_f.z, 0, -cam_f.x};
	v3_norm(&cam_r);
	cam_u = (Vec3){
		fx_mul(cam_f.y, cam_r.z) - fx_mul(cam_f.z, cam_r.y),
		fx_mul(cam_f.z, cam_r.x) - fx_mul(cam_f.x, cam_r.z),
		fx_mul(cam_f.x, cam_r.y) - fx_mul(cam_f.y, cam_r.x),
	};
	MATRIX m = {0};
	m.m[0][0] = cam_r.x;  m.m[0][1] = cam_r.y;  m.m[0][2] = cam_r.z;
	m.m[1][0] = -cam_u.x; m.m[1][1] = -cam_u.y; m.m[1][2] = -cam_u.z;
	m.m[2][0] = cam_f.x;  m.m[2][1] = cam_f.y;  m.m[2][2] = cam_f.z;
	GTE_SYNC(&m);
	gte_SetRotMatrix(&m);
	gte_SetTransMatrix(&m);
}

const Vec3 *gfx_eye(void) { return &cam_eye; }
const Vec3 *gfx_forward(void) { return &cam_f; }

void *gfx_alloc(size_t size) {
	Buffer *b = &buffers[current];
	if (next_packet + size > b->packets + PACKET_BYTES) return 0;
	void *p = next_packet;
	next_packet += size;
	return p;
}

uint32_t *gfx_ot(int index) { return &buffers[current].ot[index]; }

static inline short rel(fx v) {
	v >>= 4;
	return v > 32000 ? 32000 : v < -32000 ? -32000 : v;
}

static inline void to_view(const Vec3 *w, SVECTOR *v) {
	v->vx = rel(w->x - cam_eye.x);
	v->vy = rel(w->y - cam_eye.y);
	v->vz = rel(w->z - cam_eye.z);
}

void gfx_transform(const Vec3 *world, int n, int *sxy, int *sz) {
	SVECTOR v[3];
	int i = 0;
	for (; i + 3 <= n; i += 3) {
		to_view(&world[i], &v[0]);
		to_view(&world[i + 1], &v[1]);
		to_view(&world[i + 2], &v[2]);
		GTE_SYNC(v);
		gte_ldv3(&v[0], &v[1], &v[2]);
		gte_rtpt();
		gte_stsxy3(&sxy[i], &sxy[i + 1], &sxy[i + 2]);
		gte_stsz3(&sz[i], &sz[i + 1], &sz[i + 2]);
		GTE_SYNC(sxy);
		GTE_SYNC(sz);
	}
	for (; i < n; i++) {
		to_view(&world[i], &v[0]);
		GTE_SYNC(v);
		gte_ldv0(&v[0]);
		gte_rtps();
		gte_stsxy(&sxy[i]);
		gte_stsz(&sz[i]);
		GTE_SYNC(sxy);
		GTE_SYNC(sz);
	}
}

int gfx_project_dir(const Vec3 *d, int *x, int *y) {
	fx depth = v3_dot(d, &cam_f);
	if (depth < 200) return 0;
	*x = 160 + (int)(((int64_t)v3_dot(d, &cam_r) * GFX_FOCAL) / depth);
	*y = 120 - (int)(((int64_t)v3_dot(d, &cam_u) * GFX_FOCAL) / depth);
	return 1;
}

#define SX(v) ((short)((v) & 0xffff))
#define SY(v) ((short)((v) >> 16))

int gfx_quad_ok(const int *sxy, const int *sz) {
	if (!chase_quad_visible(sz[0] * 16, sz[1] * 16, sz[2] * 16, sz[3] * 16)) return 0;
	int minx = 32767, maxx = -32768, miny = 32767, maxy = -32768;
	for (int i = 0; i < 4; i++) {
		int x = SX(sxy[i]), y = SY(sxy[i]);
		if (x < minx) minx = x;
		if (x > maxx) maxx = x;
		if (y < miny) miny = y;
		if (y > maxy) maxy = y;
	}
	if (maxx < 0 || minx >= 320 || maxy < 0 || miny >= 240) return 0;
	return maxx - minx < 1023 && maxy - miny < 511;
}

int gfx_depth_index(int avg_sz) {
	int i = avg_sz >> 5;
	return i < 1 ? 1 : i > OT_LEN - 8 ? OT_LEN - 8 : i;
}

static int depth_bias;
void gfx_set_depth_bias(int bias) { depth_bias = bias; }

void gfx_quad(const Vec3 p[4], uint32_t rgb, int cull) {
	int sxy[4], sz[4];
	gfx_transform(p, 4, sxy, sz);
	if (!gfx_quad_ok(sxy, sz)) return;
	if (cull) {
		int ax = SX(sxy[1]) - SX(sxy[0]), ay = SY(sxy[1]) - SY(sxy[0]);
		int bx = SX(sxy[2]) - SX(sxy[0]), by = SY(sxy[2]) - SY(sxy[0]);
		if (ax * by - ay * bx <= 0) return;
	}
	POLY_F4 *q = gfx_alloc(sizeof(POLY_F4));
	if (!q) return;
	setPolyF4(q);
	setRGB0(q, rgb >> 16, rgb >> 8, rgb);
	setXY4(q, SX(sxy[0]), SY(sxy[0]), SX(sxy[1]), SY(sxy[1]), SX(sxy[2]), SY(sxy[2]), SX(sxy[3]), SY(sxy[3]));
	addPrim(gfx_ot(gfx_depth_index(((sz[0] + sz[1] + sz[2] + sz[3]) >> 2) + depth_bias * 32)), q);
}

typedef struct { fx x, y, z; int r, g, b; } ClipVert;

static ClipVert clip_lerp(const ClipVert *a, const ClipVert *b, fx near_z) {
	fx t = fx_div(near_z - a->z, b->z - a->z);
	int k = t >> 4;
	return (ClipVert){
		a->x + fx_mul(b->x - a->x, t), a->y + fx_mul(b->y - a->y, t), near_z,
		a->r + (((b->r - a->r) * k) >> 8), a->g + (((b->g - a->g) * k) >> 8), a->b + (((b->b - a->b) * k) >> 8),
	};
}

void gfx_quad_clipped(const Vec3 p[4], const uint32_t c[4]) {
	const fx near_z = FX_ONE * 3 / 2;
	ClipVert in[4], out[5];
	int n = 0;
	for (int i = 0; i < 4; i++) {
		Vec3 d = {p[i].x - cam_eye.x, p[i].y - cam_eye.y, p[i].z - cam_eye.z};
		in[i] = (ClipVert){v3_dot(&d, &cam_r), v3_dot(&d, &cam_u), v3_dot(&d, &cam_f),
		                   c[i] >> 16 & 255, c[i] >> 8 & 255, c[i] & 255};
	}
	for (int i = 0; i < 4; i++) {
		const ClipVert *a = &in[i], *b = &in[(i + 1) & 3];
		if (a->z >= near_z) out[n++] = *a;
		if ((a->z >= near_z) != (b->z >= near_z)) out[n++] = clip_lerp(a, b, near_z);
	}
	if (n < 3) return;

	int sx[5], sy[5], minx = 32767, maxx = -32768, miny = 32767, maxy = -32768;
	for (int i = 0; i < n; i++) {
		sx[i] = 160 + (int)(((int64_t)out[i].x * GFX_FOCAL) / out[i].z);
		sy[i] = 120 - (int)(((int64_t)out[i].y * GFX_FOCAL) / out[i].z);
		if (sx[i] < minx) minx = sx[i];
		if (sx[i] > maxx) maxx = sx[i];
		if (sy[i] < miny) miny = sy[i];
		if (sy[i] > maxy) maxy = sy[i];
	}
	if (maxx < 0 || minx >= 320 || maxy < 0 || miny >= 240) return;
	if (maxx - minx >= 1023 || maxy - miny >= 511) return;

	for (int i = 1; i + 1 < n; i++) {
		POLY_G3 *t = gfx_alloc(sizeof(POLY_G3));
		if (!t) return;
		setPolyG3(t);
		setRGB0(t, out[0].r, out[0].g, out[0].b);
		setRGB1(t, out[i].r, out[i].g, out[i].b);
		setRGB2(t, out[i + 1].r, out[i + 1].g, out[i + 1].b);
		setXY3(t, sx[0], sy[0], sx[i], sy[i], sx[i + 1], sy[i + 1]);
		addPrim(gfx_ot(gfx_depth_index((out[0].z + out[i].z + out[i + 1].z) / 48)), t);
	}
}

void gfx_text_scaled(int x, int y, const char *s, uint32_t rgb, int scale) {
	for (; *s; s++, x += 6 * scale) {
		int i = font_index(*s);
		if (i < 0) continue;
		POLY_FT4 *q = gfx_alloc(sizeof(POLY_FT4));
		if (!q) return;
		int u = (i % GLYPHS_PER_ROW) * 6, v = (i / GLYPHS_PER_ROW) * 8;
		setPolyFT4(q);
		setRGB0(q, (rgb >> 16 & 255) >> 1, (rgb >> 8 & 255) >> 1, (rgb & 255) >> 1);
		q->tpage = getTPage(2, 0, FONT_X, FONT_Y);
		q->clut = 0;
		setXY4(q, x, y, x + 5 * scale, y, x, y + 7 * scale, x + 5 * scale, y + 7 * scale);
		setUV4(q, u, v, u + 5, v, u, v + 7, u + 5, v + 7);
		addPrim(gfx_ot(0), q);
	}
}

void gfx_text(int x, int y, const char *s, uint32_t rgb) { gfx_text_scaled(x, y, s, rgb, 1); }
