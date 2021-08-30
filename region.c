#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "primitives.h"
#include "region.h"
#include "stat.h"

#define REGION_STEP_ALLOC 10

REGION *region_create()
{
	REGION *r = NULL;

	r = (REGION *)malloc(sizeof(REGION));
	if (r) {
		memset(r, 0, sizeof(REGION));
		r->allocated = REGION_STEP_ALLOC;
		r->points = (MixedPoint3 *)malloc(r->allocated * sizeof(MixedPoint3));
		if (!r->points) {
			free(r);
			r = NULL;
		}
	}

	return r;
}

void region_destroy(REGION *r)
{
	if (r->points) free(r->points);
	free(r);
}

void region_add_point(REGION *r, MixedPoint3 *p)
{
	if (r->num_points >= r->allocated) {
		r->allocated += REGION_STEP_ALLOC;
		r->points = (MixedPoint3 *)realloc(r->points, r->allocated * sizeof(MixedPoint3));
		if (!r->points) {
			fprintf(stderr, "Error, out of memory!\n");
			return;
		}
	}

	if (r->points) {	
		MixedPoint3 *f = NULL;
		f = &r->points[r->num_points];
		f->x = p->x;
		f->y = p->y;
		f->z = p->z;
	
		r->num_points++;

		// recalculate midpoint
		r->midpoint.x = (int)((float)(p->x + r->midpoint.x * (r->num_points-1)) / (float)(r->num_points));
		r->midpoint.y = (int)((float)(p->y + r->midpoint.y * (r->num_points-1)) / (float)(r->num_points));
		r->midpoint.z = (float)((p->z + r->midpoint.z * (r->num_points-1)) / (float)(r->num_points));
	}
}

void region_remove_point(REGION *r, MixedPoint3 *p)
{
	int i=0;
	int idx=-1;

	for (i=0; i<r->num_points; i++) {
		if (r->points[i].x == p->x && r->points[i].y == p->y && r->points[i].z == p->z) {
			idx = i;
			break;
		}
	}

	if (idx != -1) {
		int x = p->x;
		int y = p->y;
		float z = p->z;
		int j = 0;

        memmove(&r->points[idx], &r->points[idx+1], sizeof(MixedPoint3) * (r->num_points - idx));

		r->num_points--;

		// recalculate midpoint
		r->midpoint.x = (int)((float)(-1 * p->x + r->midpoint.x * (r->num_points+1)) / (float)(r->num_points));
		r->midpoint.y = (int)((float)(-1 * p->y + r->midpoint.y * (r->num_points+1)) / (float)(r->num_points));
		r->midpoint.z = (float)((-1 * p->z + r->midpoint.z * (r->num_points+1)) / (float)(r->num_points));
	}
}

int region_contains(REGION *r, IntPoint2 *p)
{
	MixedPoint3 *rp = NULL;
	int i=0;

	for (i=0; i<r->num_points; i++) {
		rp = &r->points[i];
		if (rp->x == p->x && rp->y == p->y) {
			return 1;
		}
	}

	return 0;
}

void region_absorb(REGION *dest, REGION *src)
{
	int i=0;
	for (i=0; i<src->num_points; i++) {
		region_add_point(dest, &src->points[i]);
	}
	region_destroy(src);
}

int region_adjacent(REGION *a, REGION *b)
{
	int ai, bi;
	int xdiff, ydiff;

	for (ai=0; ai<a->num_points; ai++) {
		for (bi=0; bi<b->num_points; bi++) {
			xdiff = a->points[ai].x - b->points[bi].x;
			ydiff = a->points[ai].y - b->points[bi].y;
			if ((xdiff == 0 || xdiff == -1 || xdiff == 1)
				&& (ydiff == 0 || ydiff == -1 || ydiff == 1)
				&& (xdiff == 0 || ydiff == 0)) {
				return 1;
			}
		}
	}

	return 0;
}

static double round_normal(double v)
{
	double f = floor(v);
	if (v - f > 0.5) {
		return f + 1.0;
	} else {
		return f;
	}
}

void region_trim(REGION *r) // l is for leftovers
{
	float x_stdev = 0.0;
	float y_stdev = 0.0;
	int x_size = 0;
	int y_size = 0;
	int size = 0;
	int n = r->num_points;
	int *x_array = malloc(n*sizeof(int));
	int *y_array = malloc(n*sizeof(int));
	int *to_use =  malloc(n*sizeof(int));
	int to_use_total=n;
	short *points = NULL;
	int x_min = 0;
	int x_max = 0;
	int y_min = 0;
	int y_max = 0;
	int x_total = 0;
	int y_total = 0;
	int x = 0;
	int y = 0;
	int i = 0;
	int j = 0;

	for (i=0; i<n; i++) {
		x_array[i] = r->points[i].x;
		y_array[i] = r->points[i].y;
	}

	size = 2; // this should be based on some factors like terrain size and map size

	// find bounds
	for (i=0; i<n; i++) {
		MixedPoint3 *p = &r->points[i];
		if (i==0 || p->x < x_min) x_min = p->x;
		if (i==0 || p->y < y_min) y_min = p->y;
		if (i==0 || p->x > x_max) x_max = p->x;
		if (i==0 || p->y > y_max) y_max = p->y;
	}	

	x_total = (1 + x_max - x_min);
	y_total = (1 + y_max - y_min);
	points = (short *)calloc(x_total * y_total, sizeof(short));
	if (!points) {
		fprintf(stderr, "ERROR! Out of memory.\n");
		goto abort;
	}

	for (i=0; i<n; i++) {
		x = r->points[i].x - x_min;
		y = r->points[i].y - y_min;
		points[x + x_total * y] = 1;
		to_use[i] = 1;		
	}

	// now, look at every point and see if there is any x_size by y_size area that contains this point, and only points
	// that are inside this region.  If not, this point will not be used.
	// then, the region will be tested for connectedness.  only the largest connected portion of the region will be used.

	// check for x clearance
	for (i=0; i<n; i++) {
		int in_region = 1;
		x = r->points[i].x - x_min;
		y = r->points[i].y - y_min;

		for (j=x+1; j<x_total; j++) {
			if (points[j + x_total * y]) {
				in_region++;
			} else {
				break;
			}
		}

		for (j=x-1; j>=0; j--) {
			if (points[j + x_total * y]) {
				in_region++;
			} else {
				break;
			}
		}

		if (in_region < size) { // x_size
			to_use[i] = 0;
			to_use_total--;
		}
	}

	// check for y clearance
	for (i=0; i<n; i++) {
		int in_region = 1;
		if (to_use[i]) { // if already x-whacked don't bother y-whacking	
			x = r->points[i].x - x_min;
			y = r->points[i].y - y_min;
			for (j=y+1; j<y_total; j++) {
				if (points[x + x_total * j]) {
					in_region++;
				} else {
					break;
				}
			}
	
			for (j=y-1; j>=0; j--) {
				if (points[x + x_total * j]) {
					in_region++;
				} else {
					break;
				}
			}
	
			if (in_region < size) { // y_size
				to_use[i] = 0;
				to_use_total--;
			}
		}
	}

	if (to_use_total != n) {
		REGION **sub_regions = NULL;
		int merged = 1;

		for (i=n-1; i>=0; i--) {
			if (!to_use[i]) {
				region_remove_point(r, &r->points[i]);								
			}
		}

		// find largest sub-region.  that is the only part that survives.
		sub_regions = (REGION **)malloc(sizeof(REGION *) * n);
		if (!sub_regions) fprintf(stderr, "ERROR! Out of memory.");

		// make every point a sub-region
		for (i=0; i<n; i++) {
			REGION *nr = region_create();
			if (nr) {
				sub_regions[i] = nr;
				region_add_point(nr, &r->points[i]);
			}
		}

		// loop until all merging has ceased
		while (merged) {
			REGION *a = NULL;
			REGION *b = NULL;

			merged = 0;
			// if this sub-region has any adjacent sub-regions, merge them
			for (i=0; i<n; i++) {
				a = sub_regions[i];
				if (a) {
					for (j=0; j<n; j++) {
						if (i!=j) {
							b = sub_regions[j];
							if (b && region_adjacent(a, b)) {
								region_absorb(a, b);
								sub_regions[j] = NULL;
								merged = 1;
							}
						}
					}
				}
			}
		}

		if (1) {
			int t = 0;
			int largest = -1;
			int largest_size = 0;			

			// find largest sub-region
			for (i=0; i<n; i++) {
				REGION *sub = sub_regions[i];
				if (sub) {
					t++;
					if (i==0 || sub->num_points > largest_size) {
						largest = i;
						largest_size = sub->num_points;
					}
				}
			}

			// remove all points from the original region that correspond to points
			// in all sub-regions except for the largest one			
			for (i=0; i<n; i++) {
				REGION *sub = sub_regions[i];
				if (sub) {
					if (i != largest) {
						for (j=0; j<sub->num_points; j++) {
							region_remove_point(r, &sub->points[j]);
						}
					}
					region_destroy(sub);
				}				
				sub_regions[i] = NULL;
			}
			
		}

		free(sub_regions);

	}
abort:
	free(x_array);
	free(y_array);
	free(to_use);
	if (points) free(points);
	return;
}

void region_scale(REGION *r, FloatPoint3 *mult)
{
	MixedPoint3 *p;
	int i=0;

	for (i=0; i<r->num_points; i++) {
		p = &r->points[i];
		p->x *= mult->x;
		p->y *= mult->y;
		p->z *= mult->z;
	}
}

void region_calc_values(REGION *r)
{
	int *vals = malloc(sizeof(int)*r->num_points);
	int i=0;
	int minval=0;
	int maxval=0; 

	// X
	for (i=0; i<r->num_points; i++) {
		vals[i] = r->points[i].x;
		if (i==0 || vals[i] < minval) minval = vals[i];
		if (i==0 || vals[i] > maxval) maxval = vals[i];
	}
	r->x_stdev = stdev_int(vals, r->num_points);
	r->x_min = minval;
	r->x_max = maxval;

	// Y
	for (i=0; i<r->num_points; i++) {
		vals[i] = r->points[i].y;
		if (i==0 || vals[i] < minval) minval = vals[i];
		if (i==0 || vals[i] > maxval) maxval = vals[i];
	}
	r->y_stdev = stdev_int(vals, r->num_points);
	r->y_min = minval;
	r->y_max = maxval;

	// Z
	for (i=0; i<r->num_points; i++) {
		vals[i] = (int)r->points[i].z;
		if (i==0 || vals[i] < minval) minval = vals[i];
		if (i==0 || vals[i] > maxval) maxval = vals[i];
	}
	r->z_stdev = stdev_int(vals, r->num_points);
	r->z_min = minval;
	r->z_max = maxval;	

	free(vals);
}

MixedPoint3 ***region_get_grid(REGION *r, float x_size, float y_size, int *grid_x, int *grid_y)
{
	MixedPoint3 ***ret = NULL;
							  
	*grid_x = 1 + (r->x_max - r->x_min) / x_size;
	*grid_y = 1 + (r->y_max - r->y_min) / y_size;

	if (*grid_x > 0 && *grid_y > 0) {
		ret = (MixedPoint3 ***)calloc(*grid_x, sizeof(MixedPoint3 **));
		if (ret) {
			int i=0;
			int x=0;
			int y=0;

			for (i=0; i<*grid_x; i++) {
				ret[i] = (MixedPoint3 **)calloc(*grid_y, sizeof(MixedPoint3 *));
			}

			for (i=0; i<r->num_points; i++) {
				x = (int)(((float)r->points[i].x - (float)r->x_min) / x_size);
				y = (int)(((float)r->points[i].y - (float)r->y_min) / y_size);
				ret[x][y] = &(r->points[i]);
			}
		}
	}

	return ret;
}

void region_free_grid(MixedPoint3***g, int grid_x)
{
	int i=0;

	for (i=0; i<grid_x; i++) {
		free(g[i]);
	}
	free(g);
}

int region_boundary_point(REGION *r, MixedPoint3 *p)
{
	int i = 0;	
	int above=0;
	int below=0;
	int left=0;
	int right=0;
	int ret = 0;

	// not a perfect method, but should be ok if regions are basically blob shaped and don't have
	// lots of appendages
	for (i=0; i<r->num_points; i++) {
		if (r->points[i].x < p->x && r->points[i].y == p->y) left = 1;
		if (r->points[i].x > p->x && r->points[i].y == p->y) right = 1;
		if (r->points[i].y < p->y && r->points[i].x == p->x) below = 1;
		if (r->points[i].y > p->y && r->points[i].x == p->x) above = 1;
	}

	if (above && below && left && right) {
		ret = 0;
	} else {
		ret = 1;
	}

	return ret;
}
