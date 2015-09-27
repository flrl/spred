#include <conio.h>
#include <dos.h>
#include <mem.h>
#include <stdlib.h>

#include "mouse.h"
#include "stdint.h"
#include "ui.h"
#include "vga13h.h"

Mouse mouse;

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
					break;
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
					break;
			}
		}
	}

	memset(vga, 0, 64000);
	vsync();
	memcpy(VGA, vga, 64000);
	vsync();
}