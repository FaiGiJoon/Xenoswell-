#include <stdio.h>
#include "game/game.h"
#include "ps1/gfx.h"

#define WHITE GFX_RGB(255, 255, 255)
#define CYAN GFX_RGB(90, 255, 230)
#define PINK GFX_RGB(255, 120, 210)

static void shadowed(int x, int y, const char *s, uint32_t c, int scale) {
	gfx_text_scaled(x, y, s, c, scale);
	gfx_text_scaled(x + 1, y + 1, s, 0x101010, scale);
}

const char *hud_event_text(uint32_t ev) {
	if (ev & EV_WIPEOUT) return "WIPEOUT";
	if (ev & EV_CLOSEOUT) return "WAVE OVER";
	if (ev & EV_MISSED) return "TOO LATE";
	if (ev & EV_CAUGHT) return "UP AND RIDING";
	if (ev & EV_LAND_SKETCHY) return "SKETCHY";
	if (ev & EV_LAND_CLEAN) return "CLEAN";
	return 0;
}

#define WINDOW_NEAR (6 * FX_ONE)
#define WINDOW_FAR (17 * FX_ONE)

/* Distance back to the nearest swell still behind the board, or -1. */
static fx swell_behind(const Surfer *s, const Ocean *o) {
	fx best = -1;
	for (int i = 0; i < 3; i++) {
		fx d = s->board.pos.z - o->swells[i].z;
		if (o->swells[i].alive && d > 0 && (best < 0 || d < best)) best = d;
	}
	return best;
}

static const char *prompt(const Surfer *s, fx behind) {
	switch (s->state) {
	case S_LINEUP:
		return behind >= WINDOW_NEAR && behind <= WINDOW_FAR ? "SWELL HERE - HOLD X NOW" : "WAIT FOR THE SWELL";
	case S_PADDLE: return "PADDLE WITH X";
	case S_POPUP: return "PRESS O TO STAND";
	default: return 0;
	}
}

void hud_draw(const Heat *h, const Surfer *s, const Ocean *o) {
	char text[32];
	int secs = (h->time_left + FX_ONE - 1) / FX_ONE;

	sprintf(text, "%d:%02d", secs / 60, secs % 60);
	shadowed(136, 10, text, WHITE, 2);

	sprintf(text, "WAVE %d", (int)(h->wave_score + h->chain));
	shadowed(10, 10, text, CYAN, 1);
	if (h->mult > 1) {
		sprintf(text, "X%d", h->mult);
		shadowed(10, 20, text, PINK, 2);
	}

	sprintf(text, "BEST %d", (int)h->best[0]);
	shadowed(240, 10, text, WHITE, 1);
	sprintf(text, "2ND  %d", (int)h->best[1]);
	shadowed(240, 20, text, WHITE, 1);

	sprintf(text, "%d M/S", (int)(board_speed(&s->board) / FX_ONE));
	shadowed(10, 224, text, CYAN, 1);
	sprintf(text, "%d FPS", gfx_fps());
	shadowed(264, 224, text, WHITE, 1);

	fx behind = swell_behind(s, o);
	if ((s->state == S_LINEUP || s->state == S_PADDLE) && behind >= 0) {
		sprintf(text, "SWELL BEHIND %dM", (int)(behind / FX_ONE));
		shadowed(112, 184, text, behind <= WINDOW_FAR ? PINK : CYAN, 1);
	}

	const char *p = prompt(s, behind);
	if (p) {
		int len = 0;
		while (p[len]) len++;
		shadowed(160 - len * 3, 196, p, WHITE, 1);
	}
}
