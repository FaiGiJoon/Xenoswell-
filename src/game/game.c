#include <stdio.h>
#include <sys/types.h>
#include <psxgpu.h>
#include "game/game.h"
#include "ps1/gfx.h"
#include "ps1/pad.h"
#include "ps1/water.h"
#include "ps1/models.h"
#include "ps1/sky.h"
#include "sim/chase.h"

#define MESSAGE_FRAMES 45
#define MODEL_DEPTH_BIAS -24 /* keeps the rider in front of the 5 m water cell under it */
#define WATER_AHEAD (30 * FX_ONE)

static void draw_world(const Ocean *o, const Vec3 *eye, const Vec3 *target) {
	gfx_set_camera(eye, target);
	const Vec3 *f = gfx_forward();
	Vec3 centre = {eye->x + fx_mul(f->x, WATER_AHEAD), 0, eye->z + fx_mul(f->z, WATER_AHEAD)};
	sky_draw();
	water_draw(o, &centre);
}

static void centred(int y, const char *s, uint32_t c, int scale) {
	int len = 0;
	while (s[len]) len++;
	int x = 160 - len * 3 * scale;
	gfx_text_scaled(x, y, s, c, scale);
	gfx_text_scaled(x + scale, y + scale, s, 0x101010, scale);
}

/* Returns 1 to play, 2 to watch the demo. */
static int title_screen(Ocean *o) {
	Vec3 eye = {0, 10 * FX_ONE, -30 * FX_ONE}, target = {0, 5 * FX_ONE, 10 * FX_ONE};
	SurfInput in;
	int start, select, held = 1, frame = 0;
	for (;;) {
		pad_read(&in, &start, &select);
		if (!held && start) return 1;
		if (!held && select) return 2;
		if (!start && !select) held = 0;
		ocean_step(o, FX_DT);
		gfx_begin();
		draw_world(o, &eye, &target);
		centred(50, "XENOSWELL", GFX_RGB(110, 255, 225), 4);
		centred(90, "SURF THE ALIEN SEA", GFX_RGB(255, 200, 240), 1);
		if (frame++ & 16) centred(170, "PRESS START", GFX_RGB(255, 255, 255), 2);
		centred(200, "SELECT FOR DEMO", GFX_RGB(200, 200, 230), 1);
		gfx_end();
	}
}

/* Simple rider for attract mode and automated testing. */
static void demo_input(const Surfer *s, const Ocean *o, SurfInput *in, int frame) {
	*in = (SurfInput){0};
	switch (s->state) {
	case S_LINEUP:
		in->paddle = ocean_swell_at(o, s->board.pos.z) != 0;
		break;
	case S_PADDLE:
		in->paddle = 1;
		break;
	case S_POPUP:
		in->action = 1;
		break;
	case S_RIDE: {
		int32_t err = ((700 - s->board.yaw + 2048) & 4095) - 2048;
		fx steer = err * 8;
		in->steer = steer > FX_ONE ? FX_ONE : steer < -FX_ONE ? -FX_ONE : steer;
		in->pump = 1;
		in->action = (frame % 75) == 0;
		break;
	}
	case S_AIR:
		in->spin = s->air_spin < 3800 ? 1 : 0;
		in->flip = (frame & 64) != 0;
		break;
	default:
		break;
	}
}

static const char *state_name(SurfState s) {
	static const char *const NAMES[] = {"LINEUP", "PADDLE", "POPUP", "RIDE", "AIR", "WIPEOUT", "RESPAWN"};
	return NAMES[s];
}

static void play_heat(Heat *h, Ocean *o, int demo) {
	Surfer s;
	SurfInput in;
	Vec3 eye = {0, 8 * FX_ONE, -10 * FX_ONE}, target = {0, FX_ONE, 4 * FX_ONE};
	int start, select, frame = 0, msg_frames = 0, seconds = 0;
	const char *msg = 0;
	SurfState last_state = S_LINEUP;

	ocean_init(o, (uint32_t)VSync(-1) + 1);
	surfer_init(&s);
	heat_init(h);

	while (!h->over) {
		pad_read(&in, &start, &select);
		if (demo && start) h->over = 1; /* Start leaves attract mode */
		if (demo) demo_input(&s, o, &in, frame);
		frame++;

		ocean_step(o, FX_DT);
		SurfState before = s.state;
		int32_t spin = s.air_spin;
		int flipped = s.flipped;
		uint32_t ev = surfer_step(&s, o, &in, FX_DT);
		if (before == S_AIR && (ev & (EV_LAND_CLEAN | EV_LAND_SKETCHY))) heat_on_trick(h, spin < 0 ? -spin : spin, flipped);
		heat_on_events(h, ev);
		heat_step(h, FX_DT, s.state == S_RIDE || s.state == S_AIR, s.state == S_AIR);
		chase_step(&eye, &target, &s.board, o, FX_DT);

		if (ev) {
			const char *t = hud_event_text(ev);
			if (t) { msg = t; msg_frames = MESSAGE_FRAMES; }
			printf("EVENT %s t=%d wave=%d best=%d/%d spin=%d x=%d z=%d v=%d\n", t ? t : "?",
			       frame / 30, (int)h->wave_score, (int)h->best[0], (int)h->best[1], (int)spin,
			       (int)(s.board.pos.x / FX_ONE), (int)(s.board.pos.z / FX_ONE), (int)(board_speed(&s.board) / FX_ONE));
		}
		if (s.state != last_state) {
			printf("STATE %s t=%d\n", state_name(s.state), frame / 30);
			last_state = s.state;
		}
		if (frame / 30 != seconds) {
			seconds = frame / 30;
			if (seconds % 5 == 0) printf("PERF fps=%d cpu_hblanks=%d state=%s\n", gfx_fps(), gfx_cpu_hblanks(), state_name(s.state));
		}

		gfx_begin();
		draw_world(o, &eye, &target);
		gfx_set_depth_bias(MODEL_DEPTH_BIAS);
		model_draw_board(&s.board, o);
		model_draw_surfer(&s, o);
		gfx_set_depth_bias(0);
		hud_draw(h, &s, o);
		if (msg_frames > 0) {
			msg_frames--;
			centred(70, msg, GFX_RGB(255, 240, 120), 2);
		}
		gfx_end();
	}
	printf("HEAT OVER total=%d best=%d/%d\n", (int)heat_total(h), (int)h->best[0], (int)h->best[1]);
}

static void results_screen(const Heat *h, Ocean *o) {
	Vec3 eye = {0, 12 * FX_ONE, -36 * FX_ONE}, target = {0, 4 * FX_ONE, 10 * FX_ONE};
	SurfInput in;
	int start, select, held = 1, frames = 0;
	char line[32];
	for (;;) {
		pad_read(&in, &start, &select);
		if (!held && start) return;
		if (!start) held = 0;
		if (++frames > 900) return;
		ocean_step(o, FX_DT);
		gfx_begin();
		draw_world(o, &eye, &target);
		centred(40, "HEAT OVER", GFX_RGB(110, 255, 225), 3);
		sprintf(line, "BEST WAVE   %d", (int)h->best[0]);
		centred(90, line, GFX_RGB(255, 255, 255), 1);
		sprintf(line, "SECOND WAVE %d", (int)h->best[1]);
		centred(104, line, GFX_RGB(255, 255, 255), 1);
		sprintf(line, "TOTAL %d", (int)heat_total(h));
		centred(126, line, GFX_RGB(255, 220, 120), 3);
		centred(190, "PRESS START", GFX_RGB(255, 255, 255), 1);
		gfx_end();
	}
}

void game_run(void) {
	static Ocean ocean;
	static Heat heat;

	gfx_init();
	pad_init();
	printf("XENOSWELL BOOT OK\n");
	ocean_init(&ocean, 7);
	for (;;) {
		int mode = title_screen(&ocean);
		printf("HEAT START demo=%d\n", mode == 2);
		play_heat(&heat, &ocean, mode == 2);
		results_screen(&heat, &ocean);
	}
}
