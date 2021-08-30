#ifndef _REGION_H
#define _REGION_H

#include <stdio.h>

//#include "rmods.h" // fixme, should be able to include this file, but causes compilation errors for some reason
#include "primitives.h"

typedef struct {
	MixedPoint3 *points;
	MixedPoint3 midpoint;
	int num_points;
	int allocated;
	void *mod; // fixme, should be RMOD, but that causes compilation errors for some reason
	void *mod_data;

	//
	// the following values are not set until you call region_calc_values
	//
	float x_stdev;
	float y_stdev;
	float z_stdev;
	int x_min;
	int x_max;
	int y_min;
	int y_max;
	int z_min;
	int z_max;
	//
	// end of values set by region_calc_values
	//
} REGION;

REGION *region_create();

void region_destroy();

void region_add_point(REGION *r, MixedPoint3 *p);

void region_remove_point(REGION *r, MixedPoint3 *p);

int region_contains(REGION *r, IntPoint2 *p);

void region_absorb(REGION *dest, REGION *src);

int region_adjacent(REGION *a, REGION *b);

void region_scale(REGION *r, FloatPoint3 *mult);

int region_boundary_point(REGION *r, MixedPoint3 *p);

// remove appendages from region
void region_trim(REGION *r);

// sets some values like x_stdev, y_stdev, z_stdev, and total sizes.  This should be done
// once you have finished adding points to a region and after scaling.
void region_calc_values(REGION *r);

// get a grid of pointers to the points in the region.  has NULLs where the point is not in the region.
// x_size,y_size are the number of game length units per grid point
MixedPoint3 ***region_get_grid(REGION *r, float x_size, float y_size, int *grid_x, int *grid_y);

// cleanup results of region_get_grid
void region_free_grid(MixedPoint3***g, int grid_x);

#endif

