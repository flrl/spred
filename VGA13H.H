#ifndef VGA13H_H
#define VGA13H_H

#include <dos.h>
#include <mem.h>
#include <stdlib.h>

#include "stdint.h"
#include "sprite.h"

typedef struct buffer {
	uint16_t width;
	uint16_t height;
	uint16_t n_pixels;
	uint8_t  *pixels;
} Buffer;

typedef struct rect {
	int16_t x;
	int16_t y;
	int16_t w;
	int16_t h;
} Rect;

typedef struct point {
	int16_t x;
	int16_t y;
} Point;

extern Buffer vga_real;
extern Buffer vga;

#define VGA_PX(x,y) &vga.pixels[((y) << 8) + ((y) << 6) + x]
#define BUF_PX(buf,x,y) &buf->pixels[(y) * buf->width + (x)]

int vga13h_init(void);
void vga13h_done(void);

#define rect_set(_r,_x,_y,_w,_h)                               \
do {                                                           \
	Rect *__r = (_r);                                          \
	__r->x = (_x); __r->y = (_y);                              \
	__r->w = (_w); __r->h = (_h);                              \
} while(0)

#define norm_rect(_r)                                          \
do {                                                           \
	Rect *__r = (_r);                                          \
	if (__r->w < 0) { __r->x -= __r->w; __r->w = -__r->w; }    \
	if (__r->h < 0) { __r->y -= __r->h; __r->h = -__r->h; }    \
} while (0)

void draw_rect(Buffer *buf, Rect *rect, uint8_t color);
void fill_rect(Buffer *buf, Rect *rect, uint8_t color);

void vsync(void);

void vga13h_setpalette(uint16_t start, const Color *colors, size_t count);

#endif