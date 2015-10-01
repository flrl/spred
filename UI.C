#include <conio.h>
#include <dos.h>
#include <mem.h>
#include <stdlib.h>

#include "mouse.h"
#include "stdint.h"
#include "types.h"
#include "ui.h"
#include "util.h"
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
	{ 256, 0, 33, 64 },    /* RECT_PALETTE */
	{ 288, 0, 31, 64 },    /* RECT_TOOLBOX */
	{ 192, 64, 128, 128 }, /* RECT_SHEET   */
	{ 0, 192, 320, 8 },    /* RECT_STATUS  */
};

struct pal_dim {
	int8_t rows;
	int8_t cols;
	int8_t dim;
};

struct ui_state {
	uint16_t redraw;
	int8_t finished;
	int8_t trans0;
	int8_t transpink;
	uint8_t mouse_under;
	struct {
		int8_t  show;
		int8_t  color;
		Point   pixel;
		uint8_t *pixel_p;
	} sel;
	const char *fname;
	struct pal_dim *pal_dim;
};

void init_paldim(struct pal_dim **pal_dimp, const Palette *pal) {
	struct pal_dim *pal_dim = malloc(sizeof(struct pal_dim));
	if (NULL == pal_dim) return;

	if (pal->size > 32) {
		pal_dim->rows = 16;
		pal_dim->cols = 8;
		pal_dim->dim = 3;
	}
	else if (pal->size > 8) {
		pal_dim->rows = 8;
		pal_dim->cols = 4;
		pal_dim->dim = 7;
	}
	else if (pal->size > 2) {
		pal_dim->rows = 4;
		pal_dim->cols = 2;
		pal_dim->dim = 15;
	}
	else if (pal->size == 2) {
		pal_dim->rows = 2;
		pal_dim->cols = 1;
		pal_dim->dim = 31;
	}

	*pal_dimp = pal_dim;
}

void show_palette(Palette *pal, struct ui_state *state) {
	struct pal_dim *pal_dim;
	Rect swatch, *window;
	int x, y;

	if (NULL == state->pal_dim)
		init_paldim(&state->pal_dim, pal);

	pal_dim = state->pal_dim;

	window = &ui_rects[RECT_PALETTE];
	fill_rect(&vga, window, 248);

	for (y = 0; y < pal_dim->rows; y++) {
		for (x = 0; x < pal_dim->cols; x++) {
			int color, is_sel;

			color = y * pal_dim->cols + x;
			if (color >= pal->size) return;
			is_sel = (color == state->sel.color);
			if (color == 0 && state->trans0 && state->transpink)
				color = 253;

			rect_set(&swatch,
				window->x + (x * (pal_dim->dim + 1)),
				window->y + (y * (pal_dim->dim + 1)),
				pal_dim->dim,
				pal_dim->dim);

			/* adjust size for selection status */
			if (is_sel) {
				swatch.w += 2;
				swatch.h += 2;
			}
			else {
				swatch.x += 1;
				swatch.y += 1;
			}

			/* draw the color swatch */
			fill_rect(&vga, &swatch, color);
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
	fill_rect(&vga, window, 240);

	/* FIXME can probably optimise this some... */
	dim = max(sprite->w, sprite->h);
	scale = window->w / dim;
	offset.x = (window->w - sprite->w * scale) / 2;
	offset.y = (window->w - sprite->h * scale) / 2;

	rect_set(&pixel, 0, 0, scale, scale);
	for (y = 0; y < sprite->h; y++) {
		pixel.y = offset.y + y * scale;

		for (x = 0; x < sprite->w; x++) {
			pixel.x = offset.x + x * scale;
			z = y * sprite->w + x;
			color = sprite->pixels[z];

			if (color || !state->trans0)
				fill_rect(&vga, &pixel, color);
			else if (state->transpink)
				fill_rect(&vga, &pixel, 253);

			if (state->sel.show
				&& x == state->sel.pixel.x
				&& y == state->sel.pixel.y)
					draw_rect(&vga, &pixel, 255);
		}
	}
}

void show_preview(Sprite *sprite, struct ui_state *state) {
	Rect *window;
	Point offset;

	/* background */
	window = &ui_rects[RECT_PREVIEW];
	if (state->trans0 && state->transpink)
		fill_rect(&vga, window, 253);
	else
		fill_rect(&vga, window, 240);

	offset.x = window->x + (window->w - sprite->w) / 2;
	offset.y = window->y + (window->h - sprite->h) / 2;

	blit_buf(&vga, &offset, sprite, NULL, state->trans0);
}

void show_sheet(Sheet *sheet, struct ui_state *state) {
	Rect *window;
	Point offset;
	Sprite *sprite;
	int x, y, z;

	window = &ui_rects[RECT_SHEET];
	if (state->trans0 && state->transpink)
		fill_rect(&vga, window, 253);
	else
		fill_rect(&vga, window, 240);

	/* FIXME compensate for small sheets */

	for (y = 0; y < sheet->height; y++) {
		for (x = 0; x < sheet->width; x++) {
			z = y * sheet->width + x;
			sprite = &sheet->sprites[z];

			offset.x = window->x + x * sprite->w;
			offset.y = window->y + y * sprite->h;

			blit_buf(&vga, &offset, sprite, NULL, state->trans0);

			/* FIXME show selection blip around selected sprite */
		}
	}
}

static void select_pixel(Sprite *sprite,
						struct ui_state *state, int x, int y) {


	badd(&state->sel.pixel.x, x, 0, sprite->w - 1);
	badd(&state->sel.pixel.y, y, 0, sprite->h - 1);

	state->sel.pixel_p = BUF_PX(sprite,
							 state->sel.pixel.x, state->sel.pixel.y);

	state->redraw |= REDRAW_ZOOM;
}

void do_keyevent(Sheet *sheet, struct ui_state *state, unsigned key) {
	switch (key) {
	case 0:
	case 224:
		/* recurse for special keys, with key value in high byte
		 * and the escape value that got us here in low byte */
		do_keyevent(sheet, state, (getch() << 8) | key);
		return;
	case 27:						/* esc */
		state->finished = 1;
		break;
	case 12:						/* ctrl-l */
		state->redraw |= REDRAW_ALL;
		break;
	case 'p':						/* p */
		wadd(&state->sel.color, 1, 0, sheet->palette.size - 1);
		state->redraw |= REDRAW_PALETTE;
		break;
	case 'P':						/* P */
		wadd(&state->sel.color, -1, 0, sheet->palette.size - 1);
		state->redraw |= REDRAW_PALETTE;
		break;
	case 't':
		state->trans0 = !state->trans0;
		state->redraw |= REDRAW_ZOOM | REDRAW_PREVIEW
						| REDRAW_SHEET | REDRAW_PALETTE;
		break;
	case 'T':
		state->transpink = !state->transpink;
		state->redraw |= REDRAW_ZOOM | REDRAW_PREVIEW
						| REDRAW_SHEET | REDRAW_PALETTE;
		break;
	case 'z':
		memset(sheet->sprites[0].pixels, 0,
			sheet->sprites[0].w * sheet->sprites[0].h);
		state->redraw |= REDRAW_ZOOM | REDRAW_PREVIEW | REDRAW_SHEET;
		break;
	case 'w':
		select_pixel(&sheet->sprites[0], state, 0, -1);
		break;
	case 'a':
		select_pixel(&sheet->sprites[0], state, -1, 0);
		break;
	case 's':
		select_pixel(&sheet->sprites[0], state, 0, 1);
		break;
	case 'd':
		select_pixel(&sheet->sprites[0], state, 1, 0);
		break;
	case ' ':
		if (NULL != state->sel.pixel_p) {
			*state->sel.pixel_p = state->sel.color;
			state->redraw |= REDRAW_ZOOM | REDRAW_PREVIEW | REDRAW_SHEET;
		}
		break;
	case '!':
		if (state->fname)
			sheet_write(state->fname, sheet);
		break;
	case 'r':
		badd(&sheet->palette.colors[state->sel.color].red, 1, 0, 63);
		vga13h_setpalette(state->sel.color,
			&sheet->palette.colors[state->sel.color], 1);
		break;
	case 'R':
		badd(&sheet->palette.colors[state->sel.color].red, -1, 0, 63);
		vga13h_setpalette(state->sel.color,
			&sheet->palette.colors[state->sel.color], 1);
		break;
	case 'g':
		badd(&sheet->palette.colors[state->sel.color].green, 1, 0, 63);
		vga13h_setpalette(state->sel.color,
			&sheet->palette.colors[state->sel.color], 1);
		break;
	case 'G':
		badd(&sheet->palette.colors[state->sel.color].green, -1, 0, 63);
		vga13h_setpalette(state->sel.color,
			&sheet->palette.colors[state->sel.color], 1);
		break;
	case 'b':
		badd(&sheet->palette.colors[state->sel.color].blue, 1, 0, 63);
		vga13h_setpalette(state->sel.color,
			&sheet->palette.colors[state->sel.color], 1);
		break;
	case 'B':
		badd(&sheet->palette.colors[state->sel.color].blue, -1, 0, 63);
		vga13h_setpalette(state->sel.color,
			&sheet->palette.colors[state->sel.color], 1);
		break;
	}
}

void ui_13(Sheet *sheet, const char *fname) {
	struct ui_state state;

	memset(&state, 0, sizeof(state));
	state.redraw = REDRAW_ALL;
	state.trans0 = 1;
	state.sel.show = 1;
	state.sel.pixel.x = -1;
	state.sel.pixel.y = -1;
	state.fname = fname;

	/* put the UI palette at the top end */
	vga13h_setpalette(240, defpal_16, 16);

	/* and the sheet palette at 0, for easy blitting */
	vga13h_setpalette(0, sheet->palette.colors, sheet->palette.size);

	if (!mouse_init(&mouse)) return;

	while (!state.finished) {
		if (state.redraw) {
			if (state.redraw == (uint16_t) REDRAW_ALL)
				memset(vga.pixels, 248, vga.n_pixels);

			if (state.redraw & REDRAW_STATUS)
				fill_rect(&vga, &ui_rects[RECT_STATUS], 248);

			if (state.redraw & REDRAW_TOOLBOX)
				fill_rect(&vga, &ui_rects[RECT_TOOLBOX], 248);

			if (state.redraw & REDRAW_ZOOM)
				show_zoom(&sheet->sprites[0], &state);

			if (state.redraw & REDRAW_PREVIEW)
				show_preview(&sheet->sprites[0], &state);

			if (state.redraw & REDRAW_PALETTE)
				show_palette(&sheet->palette, &state);

			if (state.redraw & REDRAW_SHEET)
				show_sheet(sheet, &state);

			if (state.redraw & REDRAW_CURSOR) {
				*VGA_PX(mouse.x, mouse.y) = state.mouse_under;
				mouse_update(&mouse);
				state.mouse_under = *VGA_PX(mouse.x, mouse.y);
				*VGA_PX(mouse.x, mouse.y) = 255;
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