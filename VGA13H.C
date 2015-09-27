#include <conio.h>
#include <dos.h>
#include <mem.h>
#include <stdlib.h>

#include "stdint.h"

uint8_t *const VGA = (uint8_t *const) 0xA0000000L;
uint8_t *vga = NULL;

#define PX(p, x, y) &p[((y) << 8) + ((y) << 6) + x]

void draw_rect(int x, int y, int w, int h, uint8_t color);
void fill_rect(int x, int y, int w, int h, uint8_t color);
void diag_line(int x, int y, int len, uint8_t color);

void set_mode(unsigned char mode) {
	union REGS regs;
	regs.h.ah = 0x00;
	regs.h.al = mode;
	int86(0x10, &regs, &regs);
}

static void rect_test(void) {
	set_mode(0x13);
	memset(VGA, 0, 64000);

	fill_rect(0,16,320,16,4);
	fill_rect(16,0,16,200,3);

	fill_rect(64,0,16,200,6);
	fill_rect(0,64,320,16,5);

	draw_rect(32,32,32,32,15);

	fill_rect(128,128,32,32,2);
	diag_line(128,128,32,15);

	while(27 != getch());
}

void vsync(void) {
	while ((inp(0x03da) & 0x08));
	while (!(inp(0x03da) & 0x08));
}

void draw_rect(int x, int y, int w, int h, uint8_t color) {
	int i;

	/* TODO compensate for negative/zero dimensions here */

	/* draw top and bottom lines */
	memset(PX(vga,x,y), color, w);
	memset(PX(vga,x,y+h-1), color, w);

	/* draw left and right lines */
	for (i = y+1; i < y+h-1; i++) {
		*PX(vga,x, i) = color;
		*PX(vga,x+w-1, i) = color;
	}
}

void fill_rect(int x, int y, int w, int h, uint8_t color) {
	int i;

	/* TODO compensate for negative dimensions here */

	for (i = y; i < y+h; i++) {
		memset(PX(vga,x, i), color, w);
	}
}

void copy_rect(int dx, int dy, int sx, int sy, int w, int h) {
	int i;
	uint8_t *buffer;

	/* TODO compensate for negative dimensions here */

	buffer = malloc(w * h);
	if (NULL == buffer) return;

	/* copy into buffer */
	for (i = 0; i < h; i++) {
		memcpy(PX(buffer,0,i), PX(vga,sx,sy+i), w);
	}

	/* copy back to vga */
	for (i = 0; i < h; i++) {
		memcpy(PX(vga,dx,dy+i), PX(buffer,0,i), w);
	}

	free(buffer);
}

void diag_line(int x, int y, int len, uint8_t color) {
	int i;

	for (i = 0; i < len; i++) {
		*PX(vga, x+i, y+i) = color;
	}
}