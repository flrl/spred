#include <dos.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>

#include "sprite.h"

const Color defpal_2[] = {
	{ 0,  0,  0  },
	{ 63, 63, 63 },
};

const Color defpal_4[] = {
	{ 0,  0,  0  },
	{ 21, 21, 21 },
	{ 42, 42, 42 },
	{ 63, 63, 63 },
};

const Color defpal_16[] = {
	{ 0,  0,  0  },
	{ 0,  0,  42 },
	{ 0,  42, 0  },
	{ 0,  42, 42 },
	{ 42, 0,  0  },
	{ 42, 0,  42 },
	{ 42, 21, 0  },
	{ 42, 42, 42 },
	{ 21, 21, 21 },
	{ 21, 21, 63 },
	{ 21, 63, 21 },
	{ 21, 63, 63 },
	{ 63, 21, 21 },
	{ 63, 21, 63 },
	{ 63, 63, 21 },
	{ 63, 63, 63 },
};

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

	if (colors < 2) colors = 2;
	if (colors > 128) colors = 128;

	sheet->palette.size = colors;
	sheet->palette.colors = calloc(colors, sizeof(sheet->palette.colors[0]));
	if (colors >= 16)
		memcpy(sheet->palette.colors, defpal_16, sizeof(defpal_16));
	else if (colors >= 4)
		memcpy(sheet->palette.colors, defpal_4, sizeof(defpal_4));
	else
		memcpy(sheet->palette.colors, defpal_2, sizeof(defpal_2));

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