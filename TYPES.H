#ifndef TYPES_H
#define TYPES_H

#include "stdint.h"

typedef struct point {
	int16_t x;
	int16_t y;
} Point;

typedef struct rect {
	int16_t x;
	int16_t y;
	int16_t w;
	int16_t h;
} Rect;

/* a buffer is a rectangle (usually with 0,0 origin) */
typedef struct buffer {
	int16_t x;
	int16_t y;
	int16_t w;
	int16_t h;
	uint16_t n_pixels;
	uint8_t *pixels;
} Buffer;

/* a sprite is a pixel buffer */
typedef Buffer Sprite;

typedef struct color {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} Color;

typedef struct palette {
	uint16_t size;
	Color *colors;
} Palette;

typedef struct sheet {
	uint16_t width;
	uint16_t height;
	Palette palette;
	Sprite *sprites;
} Sheet;

#endif