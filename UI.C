#include <conio.h>
#include <dos.h>
#include <mem.h>
#include <stdlib.h>

#include "mouse.h"
#include "stdint.h"
#include "ui.h"

Mouse mouse;

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

void show_palette(int x, int y, Palette *pal, uint8_t sel) {
	int r, c;
	int w, h;
	int j, i;
	int z;

	if (pal->size > 32) {
		r = 16; c = 8;
		w = 4; h = 4;
	}
	else if (pal->size > 8) {
		r = 8; c = 4;
		w = 8; h = 8;
	}
	else if (pal->size > 2) {
		r = 4; c = 2;
		w = 16; h = 16;
	}
	else if (pal->size == 2) {
		r = 2; c = 1;
		w = 32; h = 32;
	}

	for (j = 0; j < r; j++) {
		for (i = 0; i < c; i++) {
			/* TODO compensate for drawing pal being offset from ui pal */
			z = j * c + i;
			if (z >= pal->size) return;

			fill_rect(
				x + (i * w),
				y + (j * h),
				w, h,
				z);

			/* draw a border around the selected color */
			if (z == sel) {
				draw_rect(
					x + (i * w),
					y + (j * h),
					w, h,
					15);
			}
		}
	}
}

static void show_zoom(Sprite *sprite, int trans0, int sel_px) {
	int ox, oy;
	int scale;
	int sz;
	int i, j, k;
	int x, y;
	int c;
	int px;

	/* make it black */
	fill_rect(0,0,192,192,0);

	/* FIXME can probably optimise this some... */
	sz = (sprite->width > sprite->height) ? sprite->width : sprite->height;
	scale = 192 / sz;
	ox = (192 - sprite->width * scale) / 2;
	oy = (192 - sprite->height * scale) / 2;

	for (j = 0; j < sprite->height; j++) {
		y = oy + j * scale;

		for (i = 0; i < sprite->width; i++) {
			x = ox + i * scale;
			px = j * sprite->width + i;
			c = sprite->pixels[px];

			if (c || !trans0) {
				fill_rect(x,y,scale,scale,c);
			}
			else {
				for (k = 0; k < scale; k++) {
					*PX(vga, x + k, y + k) = 8;
					*PX(vga, x + k, y + scale-k-1) = 8;
				}
			}

			if (px == sel_px)
				draw_rect(x,y,scale,scale,15);
		}
	}

	/* FIXME draw the actual pixels */
}

static void vsync(void) {
	while ((inp(0x03da) & 0x08));
	while (!(inp(0x03da) & 0x08));
}

enum {
	REDRAW_CURSOR  = 0x01,
	REDRAW_ZOOM    = 0x02,
	REDRAW_PREVIEW = 0x04,
	REDRAW_PALETTE = 0x08,
	REDRAW_TOOLBOX = 0x10,
	REDRAW_SHEET   = 0x20,
	REDRAW_STATUS  = 0x40,
	REDRAW_ALL     = 0xffff
};

void ui_13(Sheet *sheet) {
	int finished = 0;
	uint16_t redraw = REDRAW_ALL;
	uint8_t mouse_under = 0;
	uint8_t palette_selection = 0;
	int trans0 = 1;
	int sel_px = -1;

	vga = malloc(64000);
	if (NULL == vga) return;

	if (!mouse_init(&mouse)) return;

	while (!finished) {
		if (redraw) {
			if (REDRAW_ALL == redraw)
				memset(vga, 8, 64000);

			if (redraw & REDRAW_STATUS)
				fill_rect(0,192,320,8,8);

			if (redraw & REDRAW_TOOLBOX)
				fill_rect(288,0,32,64,8);

			if (redraw & REDRAW_ZOOM)
				show_zoom(&sheet->sprites[0], trans0, sel_px);

			if (redraw & REDRAW_PREVIEW)
				fill_rect(192,0,64,64,0);

			if (redraw & REDRAW_PALETTE) {
				fill_rect(256,0,32,64,8);
				show_palette(256, 0, &sheet->palette, palette_selection);
			}

			if (redraw & REDRAW_SHEET)
				fill_rect(192,64,128,128,5);

			if (redraw & REDRAW_CURSOR) {
				*PX(vga, mouse.x, mouse.y) = mouse_under;
				mouse_update(&mouse);
				mouse_under = *PX(vga, mouse.x, mouse.y);
				*PX(vga, mouse.x, mouse.y) = 15;
			}

			vsync();
			memcpy(VGA, vga, 64000);

			redraw = 0;
		}

		/* wait for some input */
		while (!mouse_poll(&mouse) && !kbhit())
			;

		if (mouse.events)
			redraw |= REDRAW_CURSOR;

		if (kbhit()) {
			switch (getch()) {
				case 27:						/* esc */
					finished = 1;
					break;
				case 12:						/* ctrl-l */
					redraw |= REDRAW_ALL;
					break;
				case 'p':						/* p */
					palette_selection ++;
					if (palette_selection >= sheet->palette.size)
						palette_selection = 0;
					redraw |= REDRAW_PALETTE;
					break;
				case 'P':						/* P */
					if (palette_selection == 0)
						palette_selection = sheet->palette.size;
					palette_selection --;
					redraw |= REDRAW_PALETTE;
					break;
				case 't':
					trans0 = !trans0;
					redraw |= REDRAW_ZOOM;
					break;
				case 'z':
					memset(sheet->sprites[0].pixels, 0,
						sheet->sprites[0].width * sheet->sprites[0].height);
					redraw |= REDRAW_ZOOM | REDRAW_PREVIEW;
				case 'e':
					sel_px ++;
					if (sel_px >=
						sheet->sprites[0].width * sheet->sprites[0].height)
						sel_px = 0;
					redraw |= REDRAW_ZOOM;
					break;
				case 'E':
					sel_px--;
					if (sel_px < -1) sel_px = -1;
					redraw |= REDRAW_ZOOM;
					break;
				case ' ':
					if (sel_px >= 0)
						sheet->sprites[0].pixels[sel_px] = palette_selection;
					redraw |= REDRAW_ZOOM | REDRAW_PREVIEW;
			}
		}
	}

	memset(vga, 0, 64000);
	vsync();
	memcpy(VGA, vga, 64000);
	vsync();
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