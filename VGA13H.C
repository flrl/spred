#include <conio.h>
#include <dos.h>
#include <mem.h>
#include <stdlib.h>

#include "stdint.h"
#include "vga13h.h"

#define VIDEO_INT		0x10
#define SET_MODE		0x00
#define VGA_256_MODE	0x13
#define TEXT_MODE   	0x03

#define INPUT_STATUS	0x03da
#define VRETRACE		0x08

#define SCREEN_WIDTH	320
#define SCREEN_HEIGHT	200

Buffer vga_real = {
	SCREEN_WIDTH, SCREEN_HEIGHT,
	SCREEN_WIDTH * SCREEN_HEIGHT,
	(uint8_t *) 0xA0000000L
};

Buffer vga = {
	SCREEN_WIDTH, SCREEN_HEIGHT,
	SCREEN_WIDTH * SCREEN_HEIGHT,
	NULL
};

int vga13h_init(void) {
	union REGS regs;

	if (NULL == vga.pixels)
		vga.pixels = calloc(vga.n_pixels, sizeof(vga.pixels[0]));
	if (NULL == vga.pixels)
		return -1;

	regs.h.ah = SET_MODE;
	regs.h.al = VGA_256_MODE;
	int86(VIDEO_INT, &regs, &regs);

	return 0;
}

void vga13h_done(void) {
	union REGS regs;

	memset(vga.pixels, 0, vga.n_pixels);
	vsync();

	free(vga.pixels);
	vga.pixels = NULL;

	regs.h.ah = SET_MODE;
	regs.h.al = TEXT_MODE;
	int86(VIDEO_INT, &regs, &regs);
}

static void rect_test(void) {
	Rect rect;

	memset(vga_real.pixels, 0, vga_real.n_pixels);

	rect_set(&rect, 0, 16, 320, 16);
	fill_rect(&vga_real, &rect, 4);

	rect_set(&rect, 16, 0, 16, 200);
	fill_rect(&vga_real, &rect, 3);

	rect_set(&rect, 64, 0, 16, 200);
	fill_rect(&vga_real, &rect, 6);

	rect_set(&rect, 0, 64, 320, 16);
	fill_rect(&vga_real, &rect, 5);

	rect_set(&rect, 32, 32, 32, 32);
	draw_rect(&vga_real, &rect, 15);

	rect_set(&rect, 128, 128, 32, 32);
	fill_rect(&vga_real, &rect, 2);

	while(27 != getch());
}

void vsync(void) {
	while ((inp(INPUT_STATUS) & VRETRACE));
	while (!(inp(INPUT_STATUS) & VRETRACE));

	memcpy(vga_real.pixels, vga.pixels, vga_real.n_pixels);
}

void draw_rect(Buffer *buf, Rect *rect, uint8_t color) {
	int i;

	if (!rect->w || !rect->h) return; /* nothing to draw */
	norm_rect(rect);

	if (buf == &vga) {
		/* draw top and bottom lines */
		memset(VGA_PX(rect->x, rect->y), color, rect->w);
		memset(VGA_PX(rect->x, rect->y + rect->h - 1), color, rect->w);

		/* draw left and right lines */
		for (i = rect->y + 1; i < rect->y + rect->h - 1; i++) {
			*VGA_PX(rect->x, i) = color;
			*VGA_PX(rect->x + rect->w - 1, i) = color;
		}
	}
	else {
		/* draw top and bottom lines */
		memset(BUF_PX(buf, rect->x, rect->y), color, rect->w);
		memset(BUF_PX(buf, rect->x, rect->y + rect->h - 1), color, rect->w);

		/* draw left and right lines */
		for (i = rect->y + 1; i < rect->y + rect->h - 1; i++) {
			*BUF_PX(buf, rect->x, i) = color;
			*BUF_PX(buf, rect->x + rect->w - 1, i) = color;
		}
	}
}

void fill_rect(Buffer *buf, Rect *rect, uint8_t color) {
	int i;

	if (!rect->w || !rect->h) return; /* nothing to draw */
	norm_rect(rect);

	if (buf == &vga)
		for (i = rect->y; i < rect->y + rect->h; i++)
			memset(VGA_PX(rect->x, i), color, rect->w);
	else
		for (i = rect->y; i < rect->y + rect->h; i++)
			memset(BUF_PX(buf, rect->x, i), color, rect->w);
}