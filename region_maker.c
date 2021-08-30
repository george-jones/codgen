#include <math.h>

#include "primitives.h"
#include "region.h"
#include "terrain.h"
#include "region_maker.h"
#include "rmods.h"
#include "stat.h"

// distance squared
#ifdef WIN32
#define XYZDIST2(x1, y1, z1, x2, y2, z2) (pow((float)x1 - (float)x2, 2.0) + pow((float)y1 - (float)y2, 2.0) + pow(z1 - z2, 2.0))
#else
#define XYZDIST2(x1, y1, z1, x2, y2, z2) (powf((float)x1 - (float)x2, 2.0) + powf((float)y1 - (float)y2, 2.0) + powf(z1 - z2, 2.0))
#endif
#define XYZDIST(x1, y1, z1, x2, y2, z2) sqrt(XYZDIST2(x1, y1, z1, x2, y2, z2))

#define REGION_EMPTY (REGION *)0xdeadbeef

// find the adjacent neighbor that is best candidate for merging
static int find_neighbor(REGION **rlist, int idx, float *least_unfitness, int total_points, int prune)
{
	REGION *a = NULL;
	REGION *b = NULL;
	int best_idx = -1;
	int i=0;

	a = rlist[idx];

	for (i=0; rlist[i]; i++) {
		if (i != idx) {
			b = rlist[i];
			if (b != REGION_EMPTY && region_adjacent(a, b)) {
				float f = XYZDIST(a->midpoint.x, a->midpoint.y, 250*a->midpoint.z, b->midpoint.x, b->midpoint.y, 250*b->midpoint.z);				
				float unfit = 0;

				unfit = f * (float)a->num_points * (float)b->num_points / (float)(total_points * total_points);
				if (prune) {
					float x_stdev = 0.0;
					float y_stdev = 0.0;
					float diff_stdev = 0.0;
					int num = a->num_points + b->num_points;
					int *x_array = (int *) malloc(sizeof(int)*num);
					int *y_array = (int *) malloc(sizeof(int)*num);
					int c=0;

					for (c=0; c<a->num_points; c++) {
						x_array[c] = a->points[c].x;
						y_array[c] = a->points[c].y;
					}

					for (c=0; c<b->num_points; c++) {
						x_array[c+a->num_points] = b->points[c].x;
						y_array[c+a->num_points] = b->points[c].y;
					}

					x_stdev = stdev_int(x_array, num);
					y_stdev = stdev_int(y_array, num);

					if (y_stdev > 0) {					
						diff_stdev = (x_stdev - y_stdev) / y_stdev;
					} else if (x_stdev > 0) {
						diff_stdev = (y_stdev - x_stdev) / x_stdev;
					} else {
						diff_stdev = 0.0;
					}

					if (diff_stdev < 0) diff_stdev *= -1;
					unfit *= (1 + 0.04*diff_stdev);
					free(x_array);
					free(y_array);
				}				

				if (best_idx == -1 || unfit < *least_unfitness) {
					best_idx = i;
					*least_unfitness = unfit;
				}
			}
		}
	}

	return best_idx;
}

REGION **get_regions(MAP *m, int num, int prune)
{
	TERRAIN *t = m->terrain;
	REGION **ret = NULL;
	int total = t->x_res * t->y_res;
	int remaining = total;
	REGION **rlist = (REGION **)malloc(sizeof(REGION *) * (total + 1));
	REGION *r = NULL;
	FloatPoint3 mult;

	// first, make a region for every single point
	if (rlist) {
		float unfit=0.0;
		int n=0;
		int i=0;
		int j=0;
		int k=0;

		// make single-point regions
		for (i=0; i<t->x_res; i++) {
			for (j=0; j<t->y_res; j++) {
				r = region_create();
				if (r) {
					MixedPoint3 mp;
					mp.x = i;
					mp.y = j;
					mp.z = terrain_get_point(t, i, j);
					region_add_point(r, &mp);
					rlist[k++] = r;
				}
			}
		}
		rlist[k] = NULL;

		while (remaining > num) {
			for (i=0; (r = rlist[i]) != NULL && remaining > num; i++) {
				if (r != REGION_EMPTY) {
					n = find_neighbor(rlist, i, &unfit, total, prune);
					if (n != -1) {
						region_absorb(r, rlist[n]);
						rlist[n] = REGION_EMPTY;
						remaining--;						
					}
				}
			}
		}

		ret = (REGION **)malloc(sizeof(REGION *) * (num+1));
		if (ret) {
			k=0;
			for (i=0; (r = rlist[i]) != NULL && i < total; i++) {
				if (r != REGION_EMPTY) {
					ret[k++] = r;
					if (prune==2) region_trim(r);
				}
			}
			ret[k] = NULL;
		}

		free(rlist);

		// scale all the regions for the convenience of RMODs
		mult.x = (float)m->x_size / (float)(t->x_res-1);
		mult.y = (float)m->y_size / (float)(t->y_res-1);
		mult.z = (float)m->height;
		for (i=0; (r = ret[i]) != NULL; i++) {
			region_scale(r, &mult);
			region_calc_values(r);
		}

	}

	return ret;
}

void destroy_regions(REGION **rlist)
{
	int i=0;
	for (i=0; rlist[i]; i++) {
		region_destroy(rlist[i]);		
	}
	free(rlist);	
}

