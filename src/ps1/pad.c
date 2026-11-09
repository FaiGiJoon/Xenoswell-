#include "ps1/pad.h"
#include <sys/types.h>
#include <psxapi.h>
#include <psxpad.h>

#define DEADZONE 32

static uint8_t pad_buffers[2][34];

void pad_init(void) {
	InitPAD(pad_buffers[0], 34, pad_buffers[1], 34);
	StartPAD();
	ChangeClearPAD(0);
}

void pad_read(SurfInput *in, int *start, int *select) {
	const PADTYPE *p = (const PADTYPE *)pad_buffers[0];
	*in = (SurfInput){0};
	*start = *select = 0;
	if (p->stat != 0) return;
	if (p->type != PAD_ID_DIGITAL && p->type != PAD_ID_ANALOG && p->type != PAD_ID_ANALOG_STICK) return;

	/* Buttons are active low. */
	uint16_t b = ~p->btn;
	if (b & PAD_LEFT) in->steer = -FX_ONE;
	if (b & PAD_RIGHT) in->steer = FX_ONE;
	if (p->type != PAD_ID_DIGITAL) {
		int x = (int)p->ls_x - 128;
		if (x > DEADZONE || x < -DEADZONE) in->steer = x * 32;
	}
	in->paddle = in->pump = (b & PAD_CROSS) != 0;
	in->action = (b & PAD_CIRCLE) != 0;
	in->flip = (b & PAD_TRIANGLE) != 0;
	in->spin = (b & PAD_R1 ? 1 : 0) - (b & PAD_L1 ? 1 : 0);
	*start = (b & PAD_START) != 0;
	*select = (b & PAD_SELECT) != 0;
}
