#include "ps1/models.h"
#include "ps1/gfx.h"

#define F(hundredths) ((fx)((hundredths) * FX_ONE / 100)) /* metres * 100 */

typedef struct { Vec3 pos; fx sy, cy, sp, cp; } Frame;

static int anim;

static void make_frame(Frame *f, const Vec3 *pos, int32_t yaw, int32_t pitch) {
	f->pos = *pos;
	f->sy = fx_sin(yaw); f->cy = fx_cos(yaw);
	f->sp = fx_sin(pitch); f->cp = fx_cos(pitch);
}

static Vec3 place(const Frame *f, fx x, fx y, fx z) {
	fx y2 = fx_mul(y, f->cp) - fx_mul(z, f->sp);
	fx z2 = fx_mul(y, f->sp) + fx_mul(z, f->cp);
	return (Vec3){
		f->pos.x + fx_mul(x, f->cy) + fx_mul(z2, f->sy),
		f->pos.y + y2,
		f->pos.z - fx_mul(x, f->sy) + fx_mul(z2, f->cy),
	};
}

static uint32_t shade(uint32_t c, int k) {
	return GFX_RGB((c >> 16 & 255) * k >> 8, (c >> 8 & 255) * k >> 8, (c & 255) * k >> 8);
}

/* Vertex index bits: 1 = +x, 2 = +y, 4 = +z. Faces are top-left, top-right, bottom-left,
   bottom-right as seen from outside the box. */
static const unsigned char FACES[6][4] = {
	{2, 3, 0, 1}, {7, 6, 5, 4}, {6, 2, 4, 0}, {3, 7, 1, 5}, {6, 7, 2, 3}, {5, 4, 1, 0},
};
static const short FACE_LIGHT[6] = {150, 200, 170, 170, 256, 100};

static void box(const Frame *f, fx cx, fx cy, fx cz, fx hx, fx hy, fx hz, uint32_t rgb) {
	Vec3 v[8];
	for (int i = 0; i < 8; i++)
		v[i] = place(f, cx + (i & 1 ? hx : -hx), cy + (i & 2 ? hy : -hy), cz + (i & 4 ? hz : -hz));
	for (int i = 0; i < 6; i++) {
		Vec3 q[4] = {v[FACES[i][0]], v[FACES[i][1]], v[FACES[i][2]], v[FACES[i][3]]};
		gfx_quad(q, shade(rgb, FACE_LIGHT[i]), 1);
	}
}

static int32_t surface_pitch(const Board *b, const Ocean *o) {
	if (b->airborne) return 0;
	Vec3 n;
	ocean_normal(o, b->pos.x, b->pos.z, &n);
	fx along = fx_mul(n.x, fx_sin(b->yaw)) + fx_mul(n.z, fx_cos(b->yaw));
	return fx_atan2(along, n.y);
}

void model_draw_board(const Board *b, const Ocean *o) {
	Frame f;
	make_frame(&f, &b->pos, b->yaw, surface_pitch(b, o));
	box(&f, 0, F(8), 0, F(32), F(5), F(100), GFX_RGB(210, 225, 255));
	box(&f, 0, F(8), F(120), F(17), F(4), F(25), GFX_RGB(255, 90, 200));
	box(&f, 0, F(8), F(-110), F(22), F(4), F(15), GFX_RGB(255, 90, 200));
	box(&f, 0, F(14), 0, F(5), F(2), F(90), GFX_RGB(60, 255, 220));
}

void model_draw_surfer(const Surfer *s, const Ocean *o) {
	const uint32_t SUIT = GFX_RGB(236, 120, 40), TRIM = GFX_RGB(40, 44, 90), VISOR = GFX_RGB(60, 255, 230);
	Frame f;
	int32_t pitch = surface_pitch(&s->board, o);
	fx crouch = 0;
	anim++;
	switch (s->state) {
	case S_LINEUP: case S_PADDLE: pitch += 1024; crouch = F(50); break;
	case S_POPUP: pitch += 512; break;
	case S_AIR: if (s->flipped) pitch += anim * 300; crouch = F(20); break;
	case S_WIPEOUT: pitch += anim * 260; break;
	case S_RESPAWN: return;
	default: crouch = F(25); break;
	}
	Vec3 base = s->board.pos;
	base.y += F(14);
	/* A rider stands sideways to the board; the paddling pose lies along it. */
	int32_t yaw = s->board.yaw;
	if (s->state == S_RIDE || s->state == S_AIR) yaw += 1024;
	make_frame(&f, &base, yaw, s->state == S_RIDE || s->state == S_AIR ? (s->flipped && s->state == S_AIR ? pitch : 0) : pitch);

	box(&f, F(-20), F(45) - crouch / 2, 0, F(12), F(45) - crouch / 2, F(13), TRIM);
	box(&f, F(20), F(45) - crouch / 2, 0, F(12), F(45) - crouch / 2, F(13), TRIM);
	box(&f, 0, F(120) - crouch, 0, F(32), F(36), F(18), SUIT);
	box(&f, 0, F(180) - crouch, 0, F(20), F(20), F(20), TRIM);
	box(&f, 0, F(182) - crouch, F(20), F(16), F(9), F(4), VISOR);
	box(&f, F(-55), F(120) - crouch, 0, F(32), F(8), F(8), SUIT);
	box(&f, F(55), F(120) - crouch, 0, F(32), F(8), F(8), SUIT);
}
