#ifndef SPRITE_H
#define SPRITE_H

#include "stdint.h"

typedef struct color {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} Color;

typedef struct palette {
	unsigned short size;
	Color *colors;
} Palette;

typedef struct buffer {
	uint16_t width;
	uint16_t height;
	uint16_t n_pixels;
	uint8_t *pixels;
} Buffer;

/* a sprite is just a pixel buffer */
typedef Buffer Sprite;

typedef struct sheet {
	unsigned short width;
	unsigned short height;
	Palette palette;
	Sprite *sprites;
} Sheet;

extern const Color defpal_2[];
extern const Color defpal_4[];
extern const Color defpal_16[];

int sheet_write(const char *filename, const Sheet *sheet);
int sheet_read(const char *filename, Sheet *sheet);
int sheet_init(Sheet *sheet,
				unsigned short sh_width, unsigned short sh_height,
				unsigned short sp_width, unsigned short sp_height,
				unsigned short colors);
void sheet_free(Sheet *sheet);
#endif
