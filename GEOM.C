#include <stdlib.h>

#include "geom.h"

void rect_clip(Rect *rect, const Rect *bound) {
	/* assumes normalised input rectangles ok */
	rect->w = min(rect->w, bound->w - rect->x);
	rect->h = min(rect->h, bound->h - rect->y);

	if (rect->x < bound->x) {
		rect->w -= bound->x - rect->x;
		rect->x = bound->x;
	}
	if (rect->y < bound->y) {
		rect->h -= bound->y - rect->y;
		rect->y = bound->y;
	}

	if (rect->w < 0) rect->w = 0;
	if (rect->h < 0) rect->h = 0;
}