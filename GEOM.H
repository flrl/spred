#ifndef GEOM_H
#define GEOM_H

#include "types.h"

#define rect_set(_r,_x,_y,_w,_h)                               \
do {                                                           \
	Rect *__r = (_r);                                          \
	__r->x = (_x); __r->y = (_y);                              \
	__r->w = (_w); __r->h = (_h);                              \
} while (0)

#define rect_norm(_r)                                          \
do {                                                           \
	Rect *__r = (_r);                                          \
	if (__r->w < 0) { __r->x -= __r->w; __r->w = -__r->w; }    \
	if (__r->h < 0) { __r->y -= __r->h; __r->h = -__r->h; }    \
} while (0)

void rect_clip(Rect *rect, const Rect *bound);

#endif