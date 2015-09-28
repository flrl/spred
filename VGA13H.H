#ifndef VGA13H_H
#define VGA13H_H

#include <dos.h>
#include <mem.h>
#include <stdlib.h>

#include "stdint.h"

typedef struct buffer {
	uint16_t width;
	uint16_t height;
	uint16_t n_pixels;
	uint8_t  *pixels;
} Buffer;

extern Buffer vga_real;
extern Buffer vga;

#define VGA_PX(x,y) &vga.pixels[((y) << 8) + ((y) << 6) + x]
#define BUF_PX(buf,x,y) &buf->pixels[(y) * buf->width + (x)]

int vga13h_init(void);
void vga13h_done(void);

void draw_rect(Buffer *buf, int x, int y, int w, int h, uint8_t color);
void fill_rect(Buffer *buf, int x, int y, int w, int h, uint8_t color);
void diag_line(int x, int y, int len, uint8_t color);
void copy_rect(int dx, int dy, int sx, int sy, int w, int h);
void set_mode(unsigned char mode);

void vsync(void);

#endif