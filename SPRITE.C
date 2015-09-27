#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>

/* for header */

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
/**************/

int sheet_write(const char *filename, const Sheet *sheet) {
	int fd;
	unsigned i;

	fd = open(filename,
		O_CREAT|O_TRUNC|O_BINARY|O_WRONLY,
		S_IREAD|S_IWRITE);
	if (fd < 0) return -1;

	/* write palette data */
	write(fd, &sheet->palette.size, sizeof(sheet->palette.size));
	write(fd, sheet->palette.colors,
		sheet->palette.size * sizeof(sheet->palette.colors[0]));

	/* write sheet metadata */
	write(fd, &sheet->width, sizeof(sheet->width));
	write(fd, &sheet->height, sizeof(sheet->height));

	/* write sprites */
	for (i=0; i < sheet->width * sheet->height; i++) {
		Sprite *s = &sheet->sprites[i];
		write(fd, &s->width, sizeof(s->width));
		write(fd, &s->height, sizeof(s->height));
		write(fd, s->pixels, sizeof(s->pixels[0]) * s->width * s->height);
	}

	close(fd);
	return 0;
}

int sheet_read(const char *filename, Sheet *sheet) {
	int fd;
	unsigned i;

	fd = open(filename, O_RDONLY|O_BINARY, 0);
	if (fd < 0) return -1;

	/* load palette */
	read(fd, &sheet->palette.size, sizeof(sheet->palette.size));
	sheet->palette.colors =
		malloc(sheet->palette.size * sizeof(sheet->palette.colors[0]));
	read(fd, sheet->palette.colors,
		sheet->palette.size * sizeof(sheet->palette.colors[0]));

	/* load sheet metadata */
	read(fd, &sheet->width, sizeof(sheet->width));
	read(fd, &sheet->height, sizeof(sheet->height));

	/* load sprites */
	sheet->sprites =
		malloc(sheet->width * sheet->height * sizeof(sheet->sprites[0]));
	for (i=0; i < sheet->width * sheet->height; i++) {
		Sprite *s = &sheet->sprites[i];
		read(fd, &s->width, sizeof(s->width));
		read(fd, &s->height, sizeof(s->height));
		s->pixels = malloc(s->width * s->height * sizeof(s->pixels[0]));
		read(fd, s->pixels, s->width * s->height * sizeof(s->pixels[0]));
	}

	close(fd);
	return 0;
}

int sheet_init(Sheet *sheet,
				unsigned short sh_width, unsigned short sh_height,
				unsigned short sp_width, unsigned short sp_height,
				unsigned short colors) {

	unsigned i;

	sheet->palette.size = colors;
	/* FIXME generate default palette here */
	sheet->palette.colors = calloc(colors, sizeof(sheet->palette.colors[0]));

	sheet->width = sh_width;
	sheet->height = sh_height;
	sheet->sprites = malloc(sh_width * sh_height * sizeof(sheet->sprites[0]));

	for (i=0; i < sh_width * sh_height; i++) {
		Sprite *s = &sheet->sprites[i];
		s->width = sp_width;
		s->height = sp_height;
		s->pixels = calloc(sp_width * sp_height, sizeof(s->pixels[0]));
	}

	return 0;
}