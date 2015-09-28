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
	memset(vga_real.pixels, 0, vga_real.n_pixels);

	fill_rect(&vga_real, 0,16,320,16,4);
	fill_rect(&vga_real, 16,0,16,200,3);

	fill_rect(&vga_real, 64,0,16,200,6);
	fill_rect(&vga_real, 0,64,320,16,5);

	draw_rect(&vga_real, 32,32,32,32,15);

	fill_rect(&vga_real, 128,128,32,32,2);

	while(27 != getch());
}

void vsync(void) {
	while ((inp(INPUT_STATUS) & VRETRACE));
	while (!(inp(INPUT_STATUS) & VRETRACE));

	memcpy(vga_real.pixels, vga.pixels, vga_real.n_pixels);
}

void draw_rect(Buffer *buf, int x, int y, int w, int h, uint8_t color) {
	int i;

	/* TODO compensate for negative/zero dimensions here */

	if (buf == &vga) {
		/* draw top and bottom lines */
		memset(VGA_PX(x,y), color, w);
		memset(VGA_PX(x,y+h-1), color, w);

		/* draw left and right lines */
		for (i = y+1; i < y+h-1; i++) {
			*VGA_PX(x, i) = color;
			*VGA_PX(x+w-1, i) = color;
		}
	}
	else {
		/* draw top and bottom lines */
		memset(BUF_PX(buf,x,y), color, w);
		memset(BUF_PX(buf,x,y+h-1), color, w);

		/* draw left and right lines */
		for (i = y+1; i < y+h-1; i++) {
			*BUF_PX(buf,x, i) = color;
			*BUF_PX(buf,x+w-1, i) = color;
		}
	}
}

void fill_rect(Buffer *buf, int x, int y, int w, int h, uint8_t color) {
	int i;

	/* TODO compensate for negative dimensions here */

	if (buf == &vga) {
		for (i = y; i < y + h; i++) {
			memset(VGA_PX(x, i), color, w);
		}
	}
	else {
		for (i = y; i < y+h; i++) {
			memset(BUF_PX(buf,x, i), color, w);
		}
	}
}