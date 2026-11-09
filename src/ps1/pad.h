#ifndef PS1_PAD_H
#define PS1_PAD_H
#include "sim/surfer.h"

void pad_init(void);
/* Fills in from port 1. start and select report held state. */
void pad_read(SurfInput *in, int *start, int *select);

#endif
