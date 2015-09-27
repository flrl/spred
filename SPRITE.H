#ifndef SPRITE_H
#define SPRITE_H

typedef struct color {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char __unused;
} Color;

typedef struct palette {
	unsigned short size;
	Color *colors;
} Palette;

typedef struct sprite {
	unsigned short width;
	unsigned short height;
	unsigned char *pixels;
} Sprite;

typedef struct sheet {
	unsigned short width;
	unsigned short height;
	Palette palette;
	Sprite *sprites;
} Sheet;

int sheet_write(const char *filename, const Sheet *sheet);
int sheet_read(const char *filename, Sheet *sheet);
int sheet_init(Sheet *sheet,
				unsigned short sh_width, unsigned short sh_height,
				unsigned short sp_width, unsigned short sp_height,
				unsigned short colors);

#endif
