#include <conio.h>
#include <dos.h>
#include <mem.h>
#include <stdlib.h>

#include "mouse.h"
#include "stdint.h"
#include "ui.h"
#include "vga13h.h"

Mouse mouse;

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

struct ui_state {
	uint16_t redraw;
	int8_t finished;
	int8_t trans0;
	int8_t transpink;
	uint8_t mouse_under;
	struct {
		uint8_t color;
		int px;
		uint8_t *px_p;
	} sel;
};

void show_palette(Palette *pal, struct ui_state *state) {
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
				256 + (i * w),
				0 + (j * h),
				w, h,
				z);

			/* draw a border around the selected color */
			if (z == state->sel.color) {
				draw_rect(
					256 + (i * w),
					0 + (j * h),
					w, h,
					15);
			}
		}
	}
}

static void show_zoom(Sprite *sprite, struct ui_state *state) {
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

			if (c || !state->trans0) {
				fill_rect(x,y,scale,scale,c);
			}
			else if (state->transpink) {
				fill_rect(x,y,scale,scale,13);
			}

			if (px == state->sel.px)
				draw_rect(x,y,scale,scale,15);
		}
	}
}


void ui_13(Sheet *sheet) {
	struct ui_state state;

	state.redraw = REDRAW_ALL;
	state.finished = 0;
	state.trans0 = 1;
	state.transpink = 0;
	state.mouse_under = 0;
	state.sel.color = 0;
	state.sel.px = -1;
	state.sel.px_p = NULL;

	vga = malloc(64000);
	if (NULL == vga) return;

	if (!mouse_init(&mouse)) return;

	while (!state.finished) {
		if (state.redraw) {
			if (REDRAW_ALL == state.redraw)
				memset(vga, 8, 64000);

			if (state.redraw & REDRAW_STATUS)
				fill_rect(0,192,320,8,8);

			if (state.redraw & REDRAW_TOOLBOX)
				fill_rect(288,0,32,64,8);

			if (state.redraw & REDRAW_ZOOM)
				show_zoom(&sheet->sprites[0], &state);

			if (state.redraw & REDRAW_PREVIEW)
				fill_rect(192,0,64,64,0);

			if (state.redraw & REDRAW_PALETTE) {
				fill_rect(256,0,32,64,8);
				show_palette(&sheet->palette, &state);
			}

			if (state.redraw & REDRAW_SHEET)
				fill_rect(192,64,128,128,5);

			if (state.redraw & REDRAW_CURSOR) {
				*PX(vga, mouse.x, mouse.y) = state.mouse_under;
				mouse_update(&mouse);
				state.mouse_under = *PX(vga, mouse.x, mouse.y);
				*PX(vga, mouse.x, mouse.y) = 15;
			}

			vsync();
			memcpy(VGA, vga, 64000);

			state.redraw = 0;
		}

		/* wait for some input */
		while (!mouse_poll(&mouse) && !kbhit())
			;

		if (mouse.events)
			state.redraw |= REDRAW_CURSOR;

		if (kbhit()) {
			switch (getch()) {
				case 27:						/* esc */
					state.finished = 1;
					break;
				case 12:						/* ctrl-l */
					state.redraw |= REDRAW_ALL;
					break;
				case 'p':						/* p */
					state.sel.color ++;
					if (state.sel.color >= sheet->palette.size)
						state.sel.color = 0;
					state.redraw |= REDRAW_PALETTE;
					break;
				case 'P':						/* P */
					if (state.sel.color <= 0)
						state.sel.color = sheet->palette.size;
					state.sel.color --;
					state.redraw |= REDRAW_PALETTE;
					break;
				case 't':
					state.trans0 = !state.trans0;
					state.redraw |= REDRAW_ZOOM | REDRAW_PREVIEW;
					break;
				case 'z':
					memset(sheet->sprites[0].pixels, 0,
						sheet->sprites[0].width * sheet->sprites[0].height);
					state.redraw |= REDRAW_ZOOM | REDRAW_PREVIEW;
					break;
				case 'e':
					state.sel.px ++;
					if (state.sel.px >=
						sheet->sprites[0].width * sheet->sprites[0].height)
						state.sel.px = 0;
					state.sel.px_p = &sheet->sprites[0].pixels[state.sel.px];
					state.redraw |= REDRAW_ZOOM;
					break;
				case 'E':
					state.sel.px --;
					if (state.sel.px < -1) {
						state.sel.px = -1;
						state.sel.px_p = NULL;
					}
					else
						state.sel.px_p =
							&sheet->sprites[0].pixels[state.sel.px];
					state.redraw |= REDRAW_ZOOM;
					break;
				case ' ':
					if (NULL != state.sel.px_p)
						*state.sel.px_p = state.sel.color;
					state.redraw |= REDRAW_ZOOM | REDRAW_PREVIEW;
					break;
			}
		}
	}

	memset(vga, 0, 64000);
	vsync();
	memcpy(VGA, vga, 64000);
	vsync();
}