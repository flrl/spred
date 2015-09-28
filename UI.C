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

enum {
	RECT_ZOOM = 0,
	RECT_PREVIEW,
	RECT_PALETTE,
	RECT_TOOLBOX,
	RECT_SHEET,
	RECT_STATUS,
};

Rect ui_rects[] = {
	{ 0, 0, 192, 192 },    /* RECT_ZOOM    */
	{ 192, 0, 64, 64 },    /* RECT_PREVIEW */
	{ 256, 0, 32, 64 },    /* RECT_PALETTE */
	{ 288, 0, 32, 64 },    /* RECT_TOOLBOX */
	{ 192, 64, 128, 128 }, /* RECT_SHEET   */
	{ 0, 192, 320, 8 },    /* RECT_STATUS  */
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
	Rect swatch, *window;
	int rows, cols;
	int x, y, z;

	if (pal->size > 32) {
		rows = 16; cols = 8;
		rect_set(&swatch, 0, 0, 4, 4);
	}
	else if (pal->size > 8) {
		rows = 8; cols = 4;
		rect_set(&swatch, 0, 0, 8, 8);
	}
	else if (pal->size > 2) {
		rows = 4; cols = 2;
		rect_set(&swatch, 0, 0, 16, 16);
	}
	else if (pal->size == 2) {
		rows = 2; cols = 1;
		rect_set(&swatch, 0, 0, 32, 32);
	}

	window = &ui_rects[RECT_PALETTE];
	fill_rect(&vga, window, 8);

	for (y = 0; y < rows; y++) {
		for (x = 0; x < cols; x++) {
			/* TODO compensate for drawing pal being offset from ui pal */
			z = y * cols + x;
			if (z >= pal->size) return;

			swatch.x = window->x + (x * swatch.w);
			swatch.y = window->y + (y * swatch.h);

			/* draw the color swatch */
			fill_rect(&vga, &swatch, z);

			/* draw a border around the selected color */
			if (z == state->sel.color)
				draw_rect(&vga, &swatch, 15);
		}
	}
}

static void show_zoom(Sprite *sprite, struct ui_state *state) {
	Rect pixel, *window;
	Point offset;
	int dim, scale, color;
	int x, y, z;

	/* make it black */
	window = &ui_rects[RECT_ZOOM];
	fill_rect(&vga, window, 0);

	/* FIXME can probably optimise this some... */
	dim = max(sprite->width, sprite->height);
	scale = window->w / dim;
	offset.x = (window->w - sprite->width * scale) / 2;
	offset.y = (window->w - sprite->height * scale) / 2;

	rect_set(&pixel, 0, 0, scale, scale);
	for (y = 0; y < sprite->height; y++) {
		pixel.y = offset.y + y * scale;

		for (x = 0; x < sprite->width; x++) {
			pixel.x = offset.x + x * scale;
			z = y * sprite->width + x;
			color = sprite->pixels[z];

			if (color || !state->trans0)
				fill_rect(&vga, &pixel, color);
			else if (state->transpink)
				fill_rect(&vga, &pixel, 13);

			if (z == state->sel.px)
				draw_rect(&vga, &pixel, 15);
		}
	}
}

void show_preview(Sprite *sprite, struct ui_state *state) {
	Rect *window;
	Point offset;
	int color;
	int x, y, z;

	/* background */
	window = &ui_rects[RECT_PREVIEW];
	if (state->trans0 && state->transpink)
		fill_rect(&vga, window, 13);
	else
		fill_rect(&vga, window, 0);

	offset.x = window->x + (window->w - sprite->width) / 2;
	offset.y = window->y + (window->h - sprite->height) / 2;

	for (y = 0; y < sprite->height; y++) {
		for (x = 0; x < sprite->width; x++) {
			z = y * sprite->width + x;
			color = sprite->pixels[z];

			if (color || !state->trans0)
				*VGA_PX(offset.x + x, offset.y + y) = color;
		}
	}
}

void do_keyevent(Sheet *sheet, struct ui_state *state, int key) {
	switch (key) {
	case 27:						/* esc */
		state->finished = 1;
		break;
	case 12:						/* ctrl-l */
		state->redraw |= REDRAW_ALL;
		break;
	case 'p':						/* p */
		state->sel.color ++;
		if (state->sel.color >= sheet->palette.size)
			state->sel.color = 0;
		state->redraw |= REDRAW_PALETTE;
		break;
	case 'P':						/* P */
		if (state->sel.color <= 0)
			state->sel.color = sheet->palette.size;
		state->sel.color --;
		state->redraw |= REDRAW_PALETTE;
		break;
	case 't':
		state->trans0 = !state->trans0;
		state->redraw |= REDRAW_ZOOM | REDRAW_PREVIEW;
		break;
	case 'T':
		state->transpink = !state->transpink;
		state->redraw |= REDRAW_ZOOM | REDRAW_PREVIEW;
		break;
	case 'z':
		memset(sheet->sprites[0].pixels, 0,
			sheet->sprites[0].width * sheet->sprites[0].height);
		state->redraw |= REDRAW_ZOOM | REDRAW_PREVIEW;
		break;
	case 'e':
		state->sel.px ++;
		if (state->sel.px >=
			sheet->sprites[0].width * sheet->sprites[0].height)
				state->sel.px = 0;
		state->sel.px_p = &sheet->sprites[0].pixels[state->sel.px];
		state->redraw |= REDRAW_ZOOM;
		break;
	case 'E':
		state->sel.px --;
		if (state->sel.px < -1) {
			state->sel.px = -1;
			state->sel.px_p = NULL;
		}
		else
			state->sel.px_p =
				&sheet->sprites[0].pixels[state->sel.px];
		state->redraw |= REDRAW_ZOOM;
		break;
	case ' ':
		if (NULL != state->sel.px_p)
			*state->sel.px_p = state->sel.color;
		state->redraw |= REDRAW_ZOOM | REDRAW_PREVIEW;
		break;
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

	if (!mouse_init(&mouse)) return;

	while (!state.finished) {
		if (state.redraw) {
			if (state.redraw == (uint16_t) REDRAW_ALL)
				memset(vga.pixels, 8, vga.n_pixels);

			if (state.redraw & REDRAW_STATUS)
				fill_rect(&vga, &ui_rects[RECT_STATUS], 8);

			if (state.redraw & REDRAW_TOOLBOX)
				fill_rect(&vga, &ui_rects[RECT_TOOLBOX], 8);

			if (state.redraw & REDRAW_ZOOM)
				show_zoom(&sheet->sprites[0], &state);

			if (state.redraw & REDRAW_PREVIEW)
				show_preview(&sheet->sprites[0], &state);

			if (state.redraw & REDRAW_PALETTE) {
				show_palette(&sheet->palette, &state);
			}

			if (state.redraw & REDRAW_SHEET)
				fill_rect(&vga, &ui_rects[RECT_SHEET], 5);

			if (state.redraw & REDRAW_CURSOR) {
				*VGA_PX(mouse.x, mouse.y) = state.mouse_under;
				mouse_update(&mouse);
				state.mouse_under = *VGA_PX(mouse.x, mouse.y);
				*VGA_PX(mouse.x, mouse.y) = 15;
			}

			vsync();

			state.redraw = 0;
		}

		/* wait for some input */
		while (!mouse_poll(&mouse) && !kbhit())
			;

		if (mouse.events)
			state.redraw |= REDRAW_CURSOR;

		if (kbhit())
			do_keyevent(sheet, &state, getch());
	}
}