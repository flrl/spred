#ifndef SPRITE_H
#define SPRITE_H

#include "stdint.h"
#include "types.h"

extern const Color defpal_2[2];
extern const Color defpal_4[4];
extern const Color defpal_16[16];

int sheet_write(const char *filename, const Sheet *sheet);
int sheet_read(const char *filename, Sheet *sheet);
int sheet_init(Sheet *sheet,
				unsigned short sh_width, unsigned short sh_height,
				unsigned short sp_width, unsigned short sp_height,
				unsigned short colors);
void sheet_free(Sheet *sheet);

#endif
