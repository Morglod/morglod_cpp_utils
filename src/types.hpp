#pragma once

#include <string>

typedef double price_t;

// quantity
typedef double qty_t;

typedef float half_qty_t;

typedef float pos_coord_t;

typedef pos_coord_t mouse_coord_t;

#ifndef NANOVG_H
struct color_t {
	union {
		float rgba[4];
		struct {
			float r,g,b,a;
		};
	};
};
#else

typedef NVGcolor color_t;

#endif
