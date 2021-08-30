
#include <math.h>

#include "map.h"
#include "rmods.h"
#include "codgen_random.h"
#include "primitives.h"
#include "terrain.h"
#include "codgen.h"
#include "output_terrain.h"
#include "region.h"
#include "rmod_buildings.h"
#include "taken_area.h"

static void rmod_trench_brushes(MAP *m, REGION *r)
{
	DOMNODE *node_trench = NULL;
	MixedPoint3 ***g = NULL;
	BLDG b;
	IntPoint3 p[4];
	float x_size;
	float y_size;
	int grid_x;
	int grid_y;	
	int start_edge = 0;
	int width = 80;
	int depth = 40;
	int lip = 3;
	char *trench_wall = "dawnville2_roof_underside01";

	if (r->x_max - r->x_min == 0 || r->y_max - r->y_min == 0) {
		return;
	}

	fprintf(stderr, "Trench\n");

	node_trench = XMLDomGetChildNamed(XMLDomGetChildNamed(m->conf, "regions"), "trench");
	if (strlen(m->trench_texture) == 0) {
		char *txt = XMLDomGetAttribute(pick_child_random(node_trench), "name");
		strncpy(m->trench_texture, txt, sizeof(m->trench_texture));
	}

	width = XMLDomGetAttributeInt(node_trench, "width", width);
	depth = XMLDomGetAttributeInt(node_trench, "depth", depth);

	x_size = m->x_size / (m->terrain->x_res - 1);
	y_size = m->y_size / (m->terrain->y_res - 1);

	g = region_get_grid(r, x_size, y_size, &grid_x, &grid_y);
	if (g) {
		int i = 0;
		int j = 0;
		int curr=0;
		int most=0;
		int step=50;
		int floor_depth=10;

		if (r->x_max - r->x_min > r->y_max - r->y_min) {
			int curr_i1=0;
			int curr_i2=0;
			int most_j=0;
			int most_i1=0;
			int most_i2=0;
			int x1=0;
			int x2=0;
			int y=0;
			int prev=0;

			for (j=0; j<grid_y; j++) {
				for (i=0; i<grid_x; i++) {
					if (g[i][j]) {
						if (curr==0) curr_i1 = i;
						curr_i2 = i;
						curr++;
						if (curr > most) {
							most = curr;
							most_j = j;
							most_i1 = curr_i1;
							most_i2 = curr_i2;
						}
					} else {
						curr=0;
					}
				}
			}

			if (most_i1==0 && r->x_min==0) most_i1++;
			if (most_i2==grid_x-1 && r->x_max == m->x_size) most_i2--;			

			y = r->y_min + y_size * most_j;
			x1 = r->x_min + x_size * most_i1;
			x2 = r->x_min + x_size * most_i2;

			if (y - width/2 <= 0) {
				y += width/2;
			} else if (y + width/2 >= m->y_size) {
				y -= width/2;
			}

			b.points[0].z = b.points[1].z = b.points[2].z = b.points[3].z = r->z_min;

			b.points[0].x = x1;
			b.points[0].y = y + width/2;

			b.points[1].x = x2;
			b.points[1].y = y + width/2;

			b.points[2].x = x2;
			b.points[2].y = y - width/2;

			b.points[3].x = x1;
			b.points[3].y = y - width/2;

			building_add(m->buildings, &b);

			TakenArea_AddBoxPoint(m->ta, &b.points[3], x2-x1, width, 500, "trench", 0);

			for (i=x1; i<x2; i+=step) {
				// top wall
				p[0].x = i;
				p[0].y = y + width/2;
				p[0].z = map_height_at_point(m,(float)p[0].x, (float)p[0].y, NULL) - depth - floor_depth;
				p[1].x = i+step;
				p[1].y = y + width/2;
				p[1].z = map_height_at_point(m,(float)p[1].x, (float)p[1].y, NULL) - depth - floor_depth;
				p[2].x = p[1].x;
				p[2].y = y + width/2 - 1;
				p[2].z = map_height_at_point(m,(float)p[2].x, (float)p[2].y, NULL) - depth - floor_depth;
				p[3].x = p[0].x;
				p[3].y = p[2].y;
				p[3].z = map_height_at_point(m,(float)p[3].x, (float)p[3].y, NULL) - depth - floor_depth;
				box(m->map_file, p, depth + lip + floor_depth, trench_wall);

				// bottom wall
				p[0].y = p[1].y = y - width/2 + 1;
				p[2].y = p[3].y = y - width/2;
				p[0].z = map_height_at_point(m,(float)p[0].x, (float)p[0].y, NULL) - depth - floor_depth;
				p[1].z = map_height_at_point(m,(float)p[1].x, (float)p[1].y, NULL) - depth - floor_depth;
				p[2].z = map_height_at_point(m,(float)p[2].x, (float)p[2].y, NULL) - depth - floor_depth;
				p[3].z = map_height_at_point(m,(float)p[3].x, (float)p[3].y, NULL) - depth - floor_depth;
				box(m->map_file, p, depth + lip + floor_depth, trench_wall);

				// floor
				p[0].y = p[1].y = y + width/2;
				p[2].y = p[3].y = y - width/2;
				p[0].z = map_height_at_point(m,(float)p[0].x, (float)p[0].y, NULL) - depth - floor_depth + lip;
				p[1].z = map_height_at_point(m,(float)p[1].x, (float)p[1].y, NULL) - depth - floor_depth + lip;
				p[2].z = map_height_at_point(m,(float)p[2].x, (float)p[2].y, NULL) - depth - floor_depth + lip;
				p[3].z = map_height_at_point(m,(float)p[3].x, (float)p[3].y, NULL) - depth - floor_depth + lip;

				// raise edges so that you can walk in and out the sides				
				if (i-x1 < 2*depth) {
					p[0].z += depth - (i-x1)/2;
					p[3].z += depth - (i-x1)/2;

					if (i+step-x1 < 2*depth) {
						p[1].z += depth - (i+step-x1)/2;
						p[2].z += depth - (i+step-x1)/2;
					}
				} else if (x2-(i+step) < 2*depth) {
					p[1].z += depth - (x2-(i+step))/2;
					p[2].z += depth - (x2-(i+step))/2;
					if (x2-i < 2*depth) {
						p[0].z += depth - (x2-i)/2;
						p[3].z += depth - (x2-i)/2;
					}
				}

				prev = p[2].z;

				box(m->map_file, p, floor_depth, m->trench_texture);
			}
		
		} else {

			int curr_j1=0;
			int curr_j2=0;
			int most_i=0;
			int most_j1=0;
			int most_j2=0;
			int y1=0;
			int y2=0;
			int x=0;

			for (i=0; i<grid_x; i++) {
				for (j=0; j<grid_y; j++) {
					if (g[i][j]) {
						if (curr==0) curr_j1 = j;
						curr_j2 = j;
						curr++;
						if (curr > most) {
							most = curr;
							most_i = i;
							most_j1 = curr_j1;
							most_j2 = curr_j2;
						}
					} else {
						curr=0;
					}
				}
			}

			if (most_j1==0 && r->y_min==0) most_j1++;
			if (most_j2==grid_y-1 && r->y_max == m->y_size) most_j2--;

			x  = r->x_min + x_size * most_i;
			y1 = r->y_min + y_size * most_j1;
			y2 = r->y_min + y_size * most_j2;

			if (x - width/2 <= 0) {
				x += width/2;
			} else if (x + width/2 >= m->x_size) {
				x -= width/2;
			}

			b.points[0].z = b.points[1].z = b.points[2].z = b.points[3].z = r->z_min;

			b.points[0].x = x - width/2;
			b.points[0].y = y2;

			b.points[1].x = x + width/2;
			b.points[1].y = y2;

			b.points[2].x = x + width/2;
			b.points[2].y = y1;

			b.points[3].x = x - width/2;
			b.points[3].y = y1;

			building_add(m->buildings, &b);

			TakenArea_AddBoxPoint(m->ta, &b.points[3], width, y2-y1, 500, "trench", 0);

			for (j=y1; j<y2; j+=step) {
				// top wall
				p[0].y = j+step;
				p[0].x = x + width/2 - 1;
				p[0].z = map_height_at_point(m,(float)p[0].x, (float)p[0].y, NULL) - depth - floor_depth;
				p[1].y = j+step;
				p[1].x = x + width/2;
				p[1].z = map_height_at_point(m,(float)p[1].x, (float)p[1].y, NULL) - depth - floor_depth;
				p[2].y = j;
				p[2].x = x + width/2;
				p[2].z = map_height_at_point(m,(float)p[2].x, (float)p[2].y, NULL) - depth - floor_depth;
				p[3].y = j;
				p[3].x = p[0].x;
				p[3].z = map_height_at_point(m,(float)p[3].x, (float)p[3].y, NULL) - depth - floor_depth;
				box(m->map_file, p, depth + lip + floor_depth, trench_wall);

				// bottom wall
				p[0].x = p[3].x = x - width/2;
				p[1].x = p[2].x = x - width/2 + 1;
				p[0].z = map_height_at_point(m,(float)p[0].x, (float)p[0].y, NULL) - depth - floor_depth;
				p[1].z = map_height_at_point(m,(float)p[1].x, (float)p[1].y, NULL) - depth - floor_depth;
				p[2].z = map_height_at_point(m,(float)p[2].x, (float)p[2].y, NULL) - depth - floor_depth;
				p[3].z = map_height_at_point(m,(float)p[3].x, (float)p[3].y, NULL) - depth - floor_depth;
				box(m->map_file, p, depth + lip + floor_depth, trench_wall);
				
				// floor
				p[0].x = p[3].x = x - width/2;
				p[1].x = p[2].x = x + width/2;
				p[0].z = map_height_at_point(m,(float)p[0].x, (float)p[0].y, NULL) - depth - floor_depth + lip;
				p[1].z = map_height_at_point(m,(float)p[1].x, (float)p[1].y, NULL) - depth - floor_depth + lip;
				p[2].z = map_height_at_point(m,(float)p[2].x, (float)p[2].y, NULL) - depth - floor_depth + lip;
				p[3].z = map_height_at_point(m,(float)p[3].x, (float)p[3].y, NULL) - depth - floor_depth + lip;
				
				// raise edges
				if (j-y1 < 2*depth) {
					p[2].z += depth - (j-y1)/2;
					p[3].z += depth - (j-y1)/2;

					if (j+step-y1 < 2*depth) {
						p[0].z += depth - (j+step-y1)/2;
						p[1].z += depth - (j+step-y1)/2;
					}					
				} else if (y2-(j+step) < 2*depth) {
					p[0].z += depth - (y2-(j+step))/2;
					p[1].z += depth - (y2-(j+step))/2;
					if (y2-j < 2*depth) {
						p[2].z += depth - (y2-j)/2;
						p[3].z += depth - (y2-j)/2;
					}
				}

				box(m->map_file, p, floor_depth, m->trench_texture);
			}

		}

		region_free_grid(g, grid_x);
	}
}

RMOD rmod_trench = { "trench", rmod_trench_brushes, NULL, NULL, NULL };

