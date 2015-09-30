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

#define PALETTE_INDEX	0x03c8
#define PALETTE_DATA	0x03c9

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

void vga13h_setpalette(uint16_t start, const Color *colors, size_t count) {
	unsigned i;

	if (start == 0 && count == 256) {
		outp(PALETTE_INDEX, 0);
		for (i = start; i < count; i++) {
			outp(PALETTE_DATA, colors[i].red);
			outp(PALETTE_DATA, colors[i].green);
			outp(PALETTE_DATA, colors[i].blue);
		}
	}
	else {
		for (i = 0; i < count; i++) {
			outp(PALETTE_INDEX, start + i);
			outp(PALETTE_DATA, colors[i].red);
			outp(PALETTE_DATA, colors[i].green);
			outp(PALETTE_DATA, colors[i].blue);
		}
	}
}

void draw_rect(Buffer *buf, Rect *rect, uint8_t color) {
	int i;

	if (!rect->w || !rect->h) return; /* nothing to draw */
	rect_norm(rect);

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
	rect_norm(rect);

	if (buf == &vga)
		for (i = rect->y; i < rect->y + rect->h; i++)
			memset(VGA_PX(rect->x, i), color, rect->w);
	else
		for (i = rect->y; i < rect->y + rect->h; i++)
			memset(BUF_PX(buf, rect->x, i), color, rect->w);
}

void rect_clip(Rect *rect, const Rect *bound) {
	/* assumes normalised input rectangles ok */
	rect->w = min(rect->w, bound->w - rect->x);
	rect->h = min(rect->h, bound->h - rect->y);

	if (rect->x < bound->x) {
		rect->w -= bound->x - rect->x;
		rect->x = bound->x;
	}
	if (rect->y < bound->y) {
		rect->h -= bound->y - rect->y;
		rect->y = bound->y;
	}

	if (rect->w < 0) rect->w = 0;
	if (rect->h < 0) rect->h = 0;
}

void blit_buf(Buffer *d_buf, const Point *d_pt,
			 const Buffer *s_buf, const Rect *s_rect,
			 int trans0) {
	Rect d_clip;
	Rect s_clip;
	Rect bound;
	int x, y;
	int c;

	if (d_pt->x >= d_buf->width || d_pt->y >= d_buf->height)
		return; /* nowhere to draw */

	/* clip source rect to source buf: can't render pixels i don't have */
	if (s_rect) {
		memcpy(&s_clip, s_rect, sizeof(s_clip));
		rect_norm(&s_clip);
		rect_set(&bound, 0, 0, s_buf->width, s_buf->height);
		rect_clip(&s_clip, &bound);
	}
	else {
		rect_set(&s_clip, 0, 0, s_buf->width, s_buf->height);
	}

	if (s_clip.w <= 0 || s_clip.h <= 0)
		return; /* nothing to draw */

	/* clip dest rect to dest buf: can't render to pixels that don't exist */
	rect_set(&d_clip, d_pt->x, d_pt->y, s_clip.w, s_clip.h);
	rect_set(&bound, 0, 0, d_buf->width, d_buf->height);
	rect_clip(&d_clip, &bound);

	if (d_clip.w <= 0 || d_clip.h <= 0)
		return; /* nowhere to draw */

	/* clip source rect to dest rect: only render as much as i can */
	rect_set(&bound, 0, 0, d_clip.w, d_clip.h);
	rect_clip(&s_clip, &bound);

	if (s_clip.w <= 0 || s_clip.h <= 0)
		return; /* nothing to draw */

	for (y = 0; y < s_clip.h; y++) {
		if (trans0) {
			int d_yoff, s_yoff;
			s_yoff = (s_clip.y + y) * s_buf->width;
			d_yoff = (d_clip.y + y) * d_buf->width;
			for (x = 0; x < s_clip.w; x++) {
				c = s_buf->pixels[s_yoff + s_clip.x + x];
				if (c)
					d_buf->pixels[d_yoff + d_clip.x + x] = c;
			}
		}
		else {
			memcpy(BUF_PX(d_buf, d_clip.x, d_clip.y + y),
				   BUF_PX(s_buf, s_clip.x, s_clip.y + y),
				   s_clip.w);
		}
	}
}