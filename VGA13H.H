#ifndef VGA13H_H
#define VGA13H_H

#include <dos.h>
#include <mem.h>
#include <stdlib.h>

#include "stdint.h"

extern uint8_t *const VGA;
extern uint8_t *vga;

#define PX(p, x, y) &p[((y) << 8) + ((y) << 6) + x]

void draw_rect(int x, int y, int w, int h, uint8_t color);
void fill_rect(int x, int y, int w, int h, uint8_t color);
void diag_line(int x, int y, int len, uint8_t color);
void copy_rect(int dx, int dy, int sx, int sy, int w, int h);
void set_mode(unsigned char mode);

void vsync(void);

#endif