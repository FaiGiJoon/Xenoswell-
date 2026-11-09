#ifndef GAME_GAME_H
#define GAME_GAME_H
#include "sim/score.h"

void game_run(void);
void hud_draw(const Heat *h, const Surfer *s, const Ocean *o);
/* Message shown after an event; lasts until the timer in game.c expires. */
const char *hud_event_text(uint32_t ev);

#endif
