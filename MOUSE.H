#ifndef MOUSE_H
#define MOUSE_H

enum {
	MOUSE_LBUTTON = 0,
	MOUSE_RBUTTON,
	MOUSE_MBUTTON,

	MOUSE_NBUTTONS /* leave last */
};

typedef struct mouse {
	int x;
	int y;
	int dx;
	int dy;
	int events;
	unsigned char n_buttons;
	unsigned char button[MOUSE_NBUTTONS];
	unsigned char pressed[MOUSE_NBUTTONS];
	unsigned char released[MOUSE_NBUTTONS];
} Mouse;

int mouse_init(Mouse *mouse);
int mouse_poll(Mouse *mouse);
void mouse_update(Mouse *mouse);

#endif
