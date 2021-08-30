#include <stdlib.h>
#include <math.h>

#include "map.h"
#include "terrain.h"
#include "primitives.h"
#include "output_terrain.h"
#include "rmod_buildings.h"

typedef struct {
	float *x_list;
	int num_x;
	float *y_list;	
	int num_y;
} TERRAIN_POINTS;

typedef struct __terrain_mesh_quad {
	FloatPoint3 p1;
	FloatPoint3 p2;
	FloatPoint3 p3;
	FloatPoint3 p4;
	struct __terrain_mesh_quad  *gobbler;
	short active;
} TERRAIN_MESH_QUAD;

typedef struct {
	FloatPoint3 bottomLeft;
	FloatPoint3 topRight;
	FloatPoint3 *points;
	int num_x;
	int num_y;
	int num_quads;
} TERRAIN_MESH;

// check if midpoint of terrain sub-patch is in any building
static int in_building(MAP *m, float x1, float y1, float x2, float y2)
{
	IntPoint3 p;

	if (!m->buildings) return 0;	

	p.x = (int)((x1 + x2)/2.0) + m->x_offset;
	p.y = (int)((y1 + y2)/2.0) + m->y_offset;
	p.z = 0;	

	if (building_contains(m->buildings, &p)) {
		return 1;
	}

	return 0;
}

#define place_in_range(val, highval, convert_low, convert_high) ((convert_low) + ((convert_high)-(convert_low)) * ((float)(val) / (float)(highval)))
static void bogus_texture_values(float x, float x_max, float y, float y_max, float *a, float *b, float *c, float *d)
{	
	//
	// These observations are values taken from looking at an example,
	// and I really have no clue what they represent, but they are
	// probably used for offsets and scaling of the texture
	//
	// increasing x
	// a = 0 - 1920
	// c = -15 - 15
	// 
	// increasing y
	// b = 0 - 2880
	// d = 21.5 - -23.5	
	//

	*a = place_in_range(x, x_max, 0.0, 1920.0);
	*b = place_in_range(y, y_max, 0.0, 2880.0);
	*c = place_in_range(x, x_max, -15.0, 15.0);
	*d = place_in_range(y, y_max, 21.5, -23.5);
}

// because I can't figure out why memmove doesn't seem to work.  I hate windows programming.
static void bump_floats(float *pts, int front, int num)
{
	int i=0;

	for (i=num-1; i>front; i--) {
		pts[i] = pts[i-1];
	}
}

// makes assumption that the extents are already included, and that the list is pre-ordered ascending
static float *add_terrain_point(float *pts, int *num_pts, float p)
{
	int i=0;
	int j=0;

	for (i=1; i<*num_pts; i++) {
		if (pts[i] > p) {
			(*num_pts)++;
			pts = (float *)realloc(pts, (*num_pts) * sizeof(float));
			if (pts) {
				bump_floats(pts, i, *num_pts);
				pts[i] = p;
				return pts;
			}
		} else if (pts[i] == p) { // no duplicates allowed
			return pts;
		}
	}

	return pts;
}

static void get_terrain_points(MAP *m, TERRAIN_POINTS *tp)
{
	BLDG *b = NULL;
	int i=0;
	int j=0;

	tp->num_x = m->terrain->x_res;
	tp->num_y = m->terrain->y_res;

	tp->x_list = (float *)malloc(sizeof(float) * tp->num_x);
	tp->y_list = (float *)malloc(sizeof(float) * tp->num_y);

	if (!tp->x_list || !tp->y_list) {
		fprintf(stderr, "ERROR: Out of memory!");
		return;
	}

	for (i=0; i<tp->num_x; i++) {
		tp->x_list[i] = m->grid_x_unit * i;
	}

	for (i=0; i<tp->num_y; i++) {
		tp->y_list[i] = m->grid_y_unit * i;
	}

	if (m->buildings) {
		// get all the points from building edges	
		for (i=0; i<m->buildings->num; i++) {
			b = &(m->buildings->b[i]);
			for (j=0; j<4; j++) {
				float x = b->points[j].x - m->x_offset;
				float y = b->points[j].y - m->y_offset;
				if (x >= tp->x_list[0] && x <= tp->x_list[tp->num_x-1] &&
					y >= tp->y_list[0] && y <= tp->y_list[tp->num_y-1]) {				
					tp->x_list = add_terrain_point(tp->x_list, &tp->num_x, x);
					tp->y_list = add_terrain_point(tp->y_list, &tp->num_y, y);
				}
			}
		}
	}
}

static TERRAIN_MESH_QUAD *get_quads(MAP *m, TERRAIN_POINTS *tp)
{
	TERRAIN_MESH_QUAD *quads = NULL;
	TERRAIN_MESH_QUAD *q = NULL;
	TERRAIN_MESH_QUAD *q2 = NULL;
	TERRAIN_MESH_QUAD *q_above = NULL;
	TERRAIN_MESH_QUAD *q_below = NULL;
	TERRAIN_MESH_QUAD *q_right = NULL;
	TERRAIN_MESH_QUAD *q_left = NULL;
	float x_size = 0.0;
	float y_size = 0.0;
	float tol = 0.01;
	short all_active = 0;
	int num_x = 0;
	int num_y = 0;

	x_size = m->grid_x_unit;
	y_size = m->grid_y_unit;
	num_x = tp->num_x;
	num_y = tp->num_y;

	quads = (TERRAIN_MESH_QUAD *)calloc(num_x * num_y, sizeof(TERRAIN_MESH_QUAD));
	if (quads) {
		int i = 0;
		int i2 = 0;
		int i_end = 0;
		int j = 0;
		int j2 = 0;
		int j_end = 0;

		for (i=0; i<num_x-1; i++) {
			for (j=0; j<num_y-1; j++) {
				q = &quads[i + j * num_x];
				q->p1.x = tp->x_list[i];
				q->p1.y = tp->y_list[j];
				q->p1.z = terrain_height_at_point(m->terrain, q->p1.x, q->p1.y, m->x_size, m->y_size, m->height);

				q->p2.x = tp->x_list[i];
				q->p2.y = tp->y_list[j+1];
				q->p2.z = terrain_height_at_point(m->terrain, q->p2.x, q->p2.y, m->x_size, m->y_size, m->height);

				q->p3.x = tp->x_list[i+1];
				q->p3.y = tp->y_list[j+1];
				q->p3.z = terrain_height_at_point(m->terrain, q->p3.x, q->p3.y, m->x_size, m->y_size, m->height);

				q->p4.x = tp->x_list[i+1];
				q->p4.y = tp->y_list[j];
				q->p4.z = terrain_height_at_point(m->terrain, q->p4.x, q->p4.y, m->x_size, m->y_size, m->height);

				if (in_building(m, q->p1.x, q->p1.y, q->p3.x, q->p3.y)) {
					q->active = -1;
				} else {
					q->active = 1;
				}
			}
		}

		if (m->buildings) {		
			// merge on rows
			for (j=0; j<num_y-1; j++) {
				i=0;
				while (i<num_x) {
	
					all_active = 1;
	
					q = &quads[i + j * num_x];
					q2 = q;
					if (q->active != 1) all_active = 0;
					i_end = i;
	
					for (i2=i; x_size - (q2->p3.x - q->p1.x) > tol && i2<num_x; i2++) {
						q2 = &quads[i2 + j * num_x];
						if (q2->active != 1) all_active = 0;
						i_end = i2;
					}
	
					if (all_active && i_end > i) {
						q2 = &quads[i_end + j * num_x];
	
						q->p3.x = q2->p3.x;
						q->p3.y = q2->p3.y;
						q->p3.z = q2->p3.z;
	
						q->p4.x = q2->p4.x;
						q->p4.y = q2->p4.y;
						q->p4.z = q2->p4.z;
	
						for (i2=i+1; i2 <= i_end; i2++) {
							q2 = &quads[i2 + j * num_x];
							q2->active = -1;
							q2->gobbler = q;
						}
					}
	
					i = i_end+1;
				}
			}
	
			// merge on columns
			for (i=0; i<num_x-1; i++) {
				j=0;
				while (j<num_y) {
	
					all_active = 1;
	
					q = &quads[i + j * num_x];
					q2 = q;
					if (q->active != 1) all_active = 0;
					j_end = j;
	
					for (j2=j; y_size - (q2->p3.y - q->p1.y) > tol && j2<num_y; j2++) {
						q2 = &quads[i + j2 * num_x];
						if (q2->active != 1 || q->p3.x != q2->p3.x) all_active = 0;
						j_end = j2;
					}
	
					if (all_active && j_end > j) {
						q2 = &quads[i + j_end * num_x];
	
						q->p2.x = q2->p2.x;
						q->p2.y = q2->p2.y;
						q->p2.z = q2->p2.z;
	
						q->p3.x = q2->p3.x;
						q->p3.y = q2->p3.y;
						q->p3.z = q2->p3.z;
	
						for (j2=j+1; j2 <= j_end; j2++) {
							q2 = &quads[i + j2 * num_x];
							q2->active = -1;
							q2->gobbler = q;
						}
					}
	
					j = j_end+1;
				}
			}
	
			// fix the heights of the corners of mini-meshes.
			// this gets a little messed up because we're actually dealing with pairs of triangles, and
			// the diagonals are off when sub-meshes are produced.
	
	
			// top and bottom edges
			for (i=0; i<num_x-1; i++) {
				for (j=1; j<num_y-2; j++) {				
					q = &quads[i + j * num_x];
					if (q->active == 1) {
						q_above = &quads[i + (j+1) * num_x];
						q_below = &quads[i + (j-1) * num_x];
	
						if (q_above->active == -1) q_above = q_above->gobbler;
						if (q_above && q_above->active == -1) q_above = q_above->gobbler;
						if (q_above && q_above->active == -1) q_above = NULL;
	
						if (q_below->active == -1) q_below = q_below->gobbler;
						if (q_below && q_below->active == -1) q_below = q_below->gobbler;
						if (q_below && q_below->active == -1) q_below = NULL;
	
						if (q_above && q->p2.x >= q_above->p1.x && q->p3.x <= q_above->p4.x) {
							if (q->p2.x > q_above->p1.x) {
								q->p2.z = interpolate(q_above->p1.x, q_above->p1.z, q_above->p4.x, q_above->p4.z, q->p2.x);
							}
							if (q->p3.x < q_above->p4.x) {
								q->p3.z = interpolate(q_above->p1.x, q_above->p1.z, q_above->p4.x, q_above->p4.z, q->p3.x);
							}
						}
	
						if (q_below && q->p1.x >= q_below->p2.x && q->p4.x <= q_below->p3.x) {
							if (q->p1.x > q_below->p2.x) {
								q->p1.z = interpolate(q_below->p2.x, q_below->p2.z, q_below->p3.x, q_below->p3.z, q->p1.x);
							}
							if (q->p4.x < q_below->p3.x) {
								q->p4.z = interpolate(q_below->p2.x, q_below->p2.z, q_below->p3.x, q_below->p3.z, q->p4.x);
							}
						}
					}
				}
			}
	
			// left and right edges
			for (j=0; j<num_y-1; j++) {
				for (i=1; i<num_x-2; i++) {				
					q = &quads[i + j * num_x];
					if (q->active == 1) {
						q_right = &quads[(i+1) + j * num_x];
						q_left = &quads[(i-1) + j * num_x];
	
						if (q_right->active == -1) q_right = q_right->gobbler;
						if (q_right && q_right->active == -1) q_right = q_right->gobbler;
						if (q_right && q_right->active == -1) q_right = NULL;
	
						if (q_left->active == -1) q_left = q_left->gobbler;
						if (q_left && q_left->active == -1) q_left = q_left->gobbler;
						if (q_left && q_left->active == -1) q_left = NULL;
	
						if (q_right && q->p4.y >= q_right->p1.y && q->p3.y <= q_right->p2.y) {
							if (q->p4.y > q_right->p1.y) {
								q->p4.z = interpolate(q_right->p1.y, q_right->p1.z, q_right->p2.y, q_right->p2.z, q->p4.y);
							}
							if (q->p3.y < q_right->p2.y) {
								q->p3.z = interpolate(q_right->p1.y, q_right->p1.z, q_right->p2.y, q_right->p2.z, q->p3.y);
							}
						}
	
						if (q_left && q->p1.y >= q_left->p4.y && q->p2.y <= q_left->p3.y) {
							if (q->p1.y > q_left->p4.y) {
								q->p1.z = interpolate(q_left->p3.y, q_left->p3.z, q_left->p4.y, q_left->p4.z, q->p1.y);
							}
							if (q->p2.y < q_left->p3.y) {
								q->p2.z = interpolate(q_left->p3.y, q_left->p3.z, q_left->p4.y, q_left->p4.z, q->p2.y);
							}
						}
					}
				}
			}
		}
	}	

	return quads;
}

// not used yet
static TERRAIN_MESH *get_terrain_meshes(MAP *m, TERRAIN_MESH_QUAD *quads, int num_x, int num_y)
{	
	TERRAIN_MESH *tm = NULL;
	TERRAIN_MESH *mesh = NULL;

	tm = (TERRAIN_MESH *)malloc(sizeof(TERRAIN_MESH) * num_x * num_y);
	if (tm) {
		int i=0;
		int j=0;

		for (i=0; i<num_x-1; i++) {
			for (j=0; j<num_y-1; j++) {
				mesh = &tm[i + j * num_x];
			}
		}
	}
}

static void output_point(MAP *m, FloatPoint3 *p)
{
	float a,b,c,d; // I haven't the faintest idea what these numbers mean

	bogus_texture_values(p->x, (float)m->x_size, p->y, (float)m->y_size, &a, &b, &c, &d);		
	fprintf(m->map_file, "      v %.2f %.2f %.2f t %.2f %.2f %.2f %.2f\n",
			p->x + (float)m->x_offset, p->y + (float)m->y_offset, p->z, a, b, c, d);
}

void output_terrain(MAP *m)
{	
	FILE *f = NULL;	
	TERRAIN *t = NULL;
	TERRAIN_POINTS tp;
	TERRAIN_MESH_QUAD *quads = NULL;
	TERRAIN_MESH_QUAD *q = NULL;
	int num_meshes = 0;
	int i=0;
	int j=0;
	int n=0;

	f = m->map_file;
	t = m->terrain;

	memset(&tp, 0, sizeof(TERRAIN_POINTS));

	get_terrain_points(m, &tp);
	quads = get_quads(m, &tp);

	//
	// output the meshes
	// 

	for (i=0; i < (tp.num_x) * (tp.num_y); i++) {
		q = &quads[i];

		if (q->active == 1) {
			fprintf(f, " {\n");
			fprintf(f, "  mesh\n");
			fprintf(f, "  {\n");
			fprintf(f, "   %s\n", m->terrain_texture);
			fprintf(f, "   lightmap_gray\n");
		
			// I have no clue what the 16 and 2 are.
			fprintf(f, "   2 2 16 2\n");
			fprintf(f, "   (\n");

			output_point(m, &q->p1);
			output_point(m, &q->p2);

			fprintf(f, "   )\n");
			fprintf(f, "   (\n");

			output_point(m, &q->p4);
			output_point(m, &q->p3);

			fprintf(f, "   )\n");
			fprintf(f, "  }\n");
			fprintf(f, " }\n");
		}
	}

	free(quads);
	free(tp.x_list);
	free(tp.y_list);
}

