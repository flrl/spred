#include <dos.h>
#include <mem.h>

#include "mouse.h"

#define MOUSE_INT			0x33
#define MOUSE_RESET			0x00
#define MOUSE_GETPRESS		0x05
#define MOUSE_GETRELEASE	0x06
#define MOUSE_GETMOTION		0x0b

int mouse_init(Mouse *mouse) {
	union REGS regs;

	memset(mouse, 0, sizeof(*mouse));

	regs.x.ax = MOUSE_RESET;
	int86(MOUSE_INT, &regs, &regs);

	mouse->n_buttons = regs.x.bx;

	/* FIXME don't hardcode position */
	mouse->x = 160;
	mouse->y = 100;

	/* discard initial motion */
	regs.x.ax = MOUSE_GETMOTION;
	int86(MOUSE_INT, &regs, &regs);

	return 0 != regs.x.ax;
}

int mouse_poll(Mouse *mouse) {
	union REGS regs;
	int i;

	regs.x.ax = MOUSE_GETMOTION;
	int86(MOUSE_INT, &regs, &regs);
	mouse->dx = regs.x.cx;
	mouse->dy = regs.x.dx;
	mouse->events = mouse->dx | mouse->dy;

	for (i = 0; i < mouse->n_buttons; i++) {
		regs.x.ax = MOUSE_GETPRESS;
		regs.x.bx = i;
		int86(MOUSE_INT, &regs, &regs);
		mouse->pressed[i] = regs.x.bx;
		mouse->events |= regs.x.bx;
	}

	for (i = 0; i < mouse->n_buttons; i++) {
		regs.x.ax = MOUSE_GETRELEASE;
		regs.x.bx = i;
		int86(MOUSE_INT, &regs, &regs);
		mouse->released[i] = regs.x.bx;
		mouse->events |= regs.x.bx;
	}

	return mouse->events;
}

void mouse_update(Mouse *mouse) {
	int i;

	mouse->x += mouse->dx;
	mouse->y += mouse->dy;
    mouse->dx = 0;
	mouse->dy = 0;

	if (mouse->x < 0) mouse->x = 0;
	if (mouse->y < 0) mouse->y = 0;
	if (mouse->x > 319) mouse->x = 319;
	if (mouse->y > 199) mouse->y = 199;

	for (i = 0; i < mouse->n_buttons; i++) {
		if (mouse->pressed[i]) mouse->button[i] = 1;
		if (mouse->released[i]) mouse->button[i] = 0;
		mouse->pressed[i] = 0;
		mouse->released[i] = 0;
	}
}