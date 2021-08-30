#include <math.h>

#include "rmods.h"
#include "codgen_random.h"
#include "primitives.h"

static int RMRT_UNIT=264;
static int RMRT_ALLEY=42;
static int RMRT_WALKWAY=62;
static int RMRT_HEIGHT=160;
static int RMRT_HEIGHT_2ND=100;
static int RMRT_HEIGHT_3RD=80;
static int RMRT_WALL_THICKNESS=10;
static int RMRT_RAIL_HEIGHT=40;
static int RMRT_RAIL_WIDTH=4;
static int RMRT_RAIL_THICKNESS=4;
static int RMRT_HEIGHT_ROOF_THING=30;
static int RMRT_LADDER_WIDTH=40;
static int RMRT_LADDER_THICKNESS=4;
static int RMRT_LADDER_LIP=2;

static char *rtb_texture_ladder(DOMNODE *node_ladders)
{
	if (node_ladders) {
		return XMLDomGetAttribute(pick_child_random(node_ladders), "name");
	} else {
		return "caulk";
	}
}

static char *rtb_texture(DOMNODE *node_bldg)
{
	return XMLDomGetAttribute(pick_child_random(node_bldg), "name");
}

static void shuffle(int *a, int num)
{
	int tmp=0;
	int i=0;
	int j=0;

	for (i=0; i<num; i++) {
		j = genrand(num);
		tmp = a[j];
		a[j] = a[i];
		a[i] = tmp;
	}
}

static void rtb_find(int *pts, int num_points, int **grid, int grid_x, int grid_y, int dim_x, int dim_y, int *idx)
{	
	int i=0;
	int x=0;
	int y=0;
	int j=0;
	int i2=0;
	int j2=0;
	int avail=0;

	for (i=0; i<num_points; i++) {

		x = pts[i] / grid_y;
		y = pts[i] % grid_y;

		avail = 0;

		for (i2=x; i2<=x+dim_x && i2<grid_x; i2++) {
			for (j2=y; j2<=y+dim_y && j2<grid_y; j2++) {
				if (grid[i2][j2] == -1) {
					avail++;
				}
			}
		}		

		if (avail == (dim_x+1) * (dim_y+1)) {
			for (i2=x; i2<=x+dim_x && i2<grid_x; i2++) {
				for (j2=y; j2<=y+dim_y && j2<grid_y; j2++) {
					if (grid[i2][j2] == -1) {
						grid[i2][j2] = *idx;
					}
				}
			}
			(*idx)++;
		}		
	}	
}

static void make_ladder(MAP *m, IntPoint3 *p, int width, int length, int height, char *texture)
{
	int sep = randrange(18, 32);
	int i=0;
	IntPoint3 cp;
	int w_off=0;
	int l_off=0;
	
	if (width > length) {
		w_off = RMRT_LADDER_THICKNESS;
	} else {
		l_off = RMRT_LADDER_THICKNESS;
	}	

	// rails
	cp.x = p->x;
	cp.y = p->y;
	cp.z = p->z;
	box_easy(m->map_file, &cp, RMRT_LADDER_THICKNESS, RMRT_LADDER_THICKNESS, height, texture);
	cp.x = p->x + width - RMRT_LADDER_THICKNESS;
	cp.y = p->y + length - RMRT_LADDER_THICKNESS;
	box_easy(m->map_file, &cp, RMRT_LADDER_THICKNESS, RMRT_LADDER_THICKNESS, height, texture);

	// rungs
	cp.x = p->x + w_off + ((w_off==0)?1:0);
	cp.y = p->y + l_off + ((l_off==0)?1:0);
	cp.z = p->z;
	for (i=height-sep; i>0; i-=sep) {		
		cp.z = p->z + i;
		box_easy(m->map_file, &cp, width - 2*w_off - ((w_off==0)?2:0), length - 2*l_off - ((l_off==0)?2:0), RMRT_LADDER_THICKNESS, texture);
	}

	// make climable
	box_easy(m->map_file, p, width, length, height, "ladder");
}


// returns height at the top of the base ( not including rail )
static int rtb_building_level(MAP *m, REGION *r, char *txtr, char *ladder_txtr,
							  int start_h, int gmin_x, int gmin_y, int gmax_x, int gmax_y,
							  int x_off, int y_off, int ladder, int level)
{
	IntPoint3 p;
	int h;
	int side=0;
	int width,length;
	int x_s;
	int y_s;
	int z_s;
	int base_height = 0;

	base_height = (level == 1)? RMRT_HEIGHT : (level == 2)? RMRT_HEIGHT_2ND : RMRT_HEIGHT_3RD;

	if (level == 1) {
		int h1,h2,h3,h4 = 0;

		p.x = x_off + gmin_x * RMRT_UNIT + RMRT_ALLEY;
		p.y = y_off + gmin_y * RMRT_UNIT + RMRT_ALLEY;
		h1 = terrain_height_at_point(m->terrain, p.x, p.y, m->x_size, m->y_size, m->height);
	
		p.x = x_off + gmin_x * RMRT_UNIT + RMRT_ALLEY;
		p.y = y_off + (gmax_y+1) * RMRT_UNIT - RMRT_ALLEY;
		h2 = terrain_height_at_point(m->terrain, p.x, p.y, m->x_size, m->y_size, m->height);
	
		p.x = x_off + (gmax_x+1) * RMRT_UNIT - RMRT_ALLEY;
		p.y = y_off + gmin_y * RMRT_UNIT + RMRT_ALLEY;
		h3 = terrain_height_at_point(m->terrain, p.x, p.y, m->x_size, m->y_size, m->height);
	
		p.x = x_off + (gmax_x+1) * RMRT_UNIT - RMRT_ALLEY;
		p.y = y_off + (gmax_y+1) * RMRT_UNIT - RMRT_ALLEY;
		h4 = terrain_height_at_point(m->terrain, p.x, p.y, m->x_size, m->y_size, m->height);
	
		// min(h1,h2,h3,h4)
		h = (h1 < h2)?h1:h2;
		h = (h < h3)?h:h3;
		h = (h < h4)?h:h4;
	} else {
		h = start_h;
	}

	// base
	p.x = x_off + gmin_x * RMRT_UNIT + RMRT_ALLEY + (level - 1) * RMRT_WALKWAY;
	p.y = y_off + gmin_y * RMRT_UNIT + RMRT_ALLEY + (level - 1) * RMRT_WALKWAY;
	p.z = h;
	x_s = (gmax_x - gmin_x + 1)*RMRT_UNIT - 2 * (RMRT_ALLEY + (level - 1) * RMRT_WALKWAY);
	y_s = (gmax_y - gmin_y + 1)*RMRT_UNIT - 2 * (RMRT_ALLEY + (level - 1) * RMRT_WALKWAY);
	hollowbox(m->map_file, &p, x_s, y_s, base_height, RMRT_WALL_THICKNESS, txtr, HOLLOW_DEFAULT);

	z_s = base_height + RMRT_RAIL_HEIGHT;
	TakenArea_AddBox(m->ta, p.x+x_s/2, p.y+y_s/2, p.z, x_s/2, y_s/2, z_s, "Rooftop", 1);

	// rail
	p.z = h + base_height - RMRT_RAIL_THICKNESS;
	hollowbox(m->map_file, &p, x_s, y_s, RMRT_RAIL_HEIGHT, RMRT_RAIL_THICKNESS, txtr, HOLLOW_BOTTOMLESS|HOLLOW_TOPLESS);

	// ladder
	if (ladder) {
		side = genrand(4);
		switch (side) {
		case 0:
			p.x = x_off + gmin_x * RMRT_UNIT + RMRT_ALLEY + (level - 1) * RMRT_WALKWAY - RMRT_LADDER_THICKNESS;
			p.y = y_off + ((float)(gmin_y + gmax_y + 1))/2 * RMRT_UNIT - RMRT_LADDER_WIDTH/2;
			width = RMRT_LADDER_THICKNESS;
			length = RMRT_LADDER_WIDTH;
			break;
		case 1:			
			p.x = x_off + ((float)(gmin_x + gmax_x + 1))/2 * RMRT_UNIT - RMRT_LADDER_WIDTH/2;
			p.y = y_off + gmin_y * RMRT_UNIT + RMRT_ALLEY + (level - 1) * RMRT_WALKWAY - RMRT_LADDER_THICKNESS;
			width = RMRT_LADDER_WIDTH;
			length = RMRT_LADDER_THICKNESS;
			break;
		case 2:			
			p.x = x_off + (gmax_x+1) * RMRT_UNIT - (RMRT_ALLEY + (level - 1) * RMRT_WALKWAY);
			p.y = y_off + ((float)(gmin_y + gmax_y + 1))/2 * RMRT_UNIT - RMRT_LADDER_WIDTH/2;
			width = RMRT_LADDER_THICKNESS;
			length = RMRT_LADDER_WIDTH;
			break;
		case 3:
			p.x = x_off + ((float)(gmin_x + gmax_x + 1))/2 * RMRT_UNIT - RMRT_LADDER_WIDTH/2;
			p.y = y_off + (gmax_y+1) * RMRT_UNIT - (RMRT_ALLEY + (level - 1) * RMRT_WALKWAY);
			width = RMRT_LADDER_WIDTH;
			length = RMRT_LADDER_THICKNESS;
			break;
		}

		p.x--;
		p.y--;
	
		p.z = h;

		make_ladder(m, &p, width, length, base_height + RMRT_RAIL_HEIGHT +
					RMRT_LADDER_LIP + RMRT_RAIL_THICKNESS/2 - RMRT_WALL_THICKNESS, ladder_txtr);
	}

	return h + base_height;
}

// returns height above base, including the rail.
static int rtb_building_all(MAP *m, REGION *r, int gmin_x, int gmin_y, int gmax_x, int gmax_y, int x_off, int y_off,
							 int use_2nd_level, int use_3rd_level)
{
	DOMNODE *node_bldg = XMLDomGetChildNamed(XMLDomGetChildNamed(m->conf, "regions"), "buildings");
	DOMNODE *node_ladders = XMLDomGetChildNamed(m->conf, "ladders");
	char *texture = rtb_texture(node_bldg);
	char *ladder_texture = rtb_texture_ladder(node_ladders);
	int base_h = 0;
	int h_2nd = 0;

	base_h = rtb_building_level(m, r, texture, ladder_texture, 0, gmin_x, gmin_y, gmax_x, gmax_y, x_off, y_off, 1, 1);	

	if (use_2nd_level) {
		if (genrand(4) % 4 > 0) {
			h_2nd = rtb_building_level(m, r, texture, ladder_texture, base_h, gmin_x, gmin_y, gmax_x, gmax_y, x_off, y_off, 1, 2);
		} else {
			use_3rd_level = 0;
		}
	}

	if (use_3rd_level) {
		if (genrand(3) % 3 > 0) {
			rtb_building_level(m, r, texture, ladder_texture, h_2nd, gmin_x, gmin_y, gmax_x, gmax_y, x_off, y_off, 1, 3);
		}
	}

	return base_h + RMRT_RAIL_HEIGHT;
}

static void rtb_building(MAP *m, REGION *r, int **grid, int gx, int gy, int idx, int x_off, int y_off)
{
	int gmin_x=-1, gmax_x=-1, gmin_y=-1, gmax_y=-1;
	int i=0;
	int j=0;
	int use_2nd_level = 0;
	int use_3rd_level = 0;

	for (i=0; i<gx; i++) {
		for (j=0; j<gy; j++) {
			if (grid[i][j] == idx) {
				// find building extents
				if (i < gmin_x || gmin_x == -1) gmin_x = i;
				if (j < gmin_y || gmin_y == -1) gmin_y = j;
				if (i > gmax_x || gmax_x == -1) gmax_x = i;
				if (j > gmax_y || gmax_y == -1) gmax_y = j;
			}
		}
	}

	if (gmax_x - gmin_x == 1 && gmax_y - gmin_y == 1) {
		use_2nd_level = 1;
		use_3rd_level = 1;
	} else if (gmax_x - gmin_x == 2 && gmax_y - gmin_y == 0 ||
			   gmax_x - gmin_x == 0 && gmax_y - gmin_y == 2 ||
			   gmax_x - gmin_x == 1 && gmax_y - gmin_y == 0 ||
			   gmax_x - gmin_x == 0 && gmax_y - gmin_y == 1) {
		use_2nd_level = 1;
	}

	rtb_building_all(m, r, gmin_x, gmin_y, gmax_x, gmax_y, x_off, y_off, use_2nd_level, use_3rd_level);
}

static void rmod_rooftops_brushes(MAP *m, REGION *r)
{
	DOMNODE *node_bldg = NULL;
	DOMNODE *node_ladders = NULL;
	MixedPoint3 ***g = NULL;
	float x_size;
	float y_size;
	int grid_x;
	int grid_y;

	x_size = m->x_size / (m->terrain->x_res - 1);
	y_size = m->y_size / (m->terrain->y_res - 1);

	fprintf(stderr, "Rooftops\n");

	node_bldg = XMLDomGetChildNamed(XMLDomGetChildNamed(m->conf, "regions"), "buildings");

	RMRT_UNIT = XMLDomGetAttributeInt(node_bldg, "unit_size", RMRT_UNIT);
	RMRT_ALLEY = XMLDomGetAttributeInt(node_bldg, "alley_size", RMRT_ALLEY);
	RMRT_WALL_THICKNESS = XMLDomGetAttributeInt(node_bldg, "wall_thickness", RMRT_WALL_THICKNESS);
	RMRT_WALKWAY = XMLDomGetAttributeInt(node_bldg, "walkway_size", RMRT_WALKWAY);
	RMRT_HEIGHT = XMLDomGetAttributeInt(node_bldg, "first_height", RMRT_HEIGHT);
	RMRT_HEIGHT_2ND = XMLDomGetAttributeInt(node_bldg, "second_height", RMRT_HEIGHT_2ND);
	RMRT_HEIGHT_3RD = XMLDomGetAttributeInt(node_bldg, "third_height", RMRT_HEIGHT_3RD);
	RMRT_RAIL_HEIGHT = XMLDomGetAttributeInt(node_bldg, "rail_height", RMRT_RAIL_HEIGHT);
	RMRT_RAIL_THICKNESS = XMLDomGetAttributeInt(node_bldg, "rail_thickness", RMRT_RAIL_THICKNESS);

	node_ladders = XMLDomGetChildNamed(m->conf, "ladders");
	if (node_ladders) {
		RMRT_LADDER_WIDTH = XMLDomGetAttributeInt(node_ladders, "width", 40);
		RMRT_LADDER_THICKNESS = XMLDomGetAttributeInt(node_ladders, "thickness", 4);
		RMRT_LADDER_LIP = XMLDomGetAttributeInt(node_ladders, "lip", 2);
	}

	g = region_get_grid(r, x_size, y_size, &grid_x, &grid_y);
	if (g) {
		int rx = (r->x_max - r->x_min) / grid_x;
		int ry = (r->y_max - r->y_min) / grid_y;
		float gxf = (float)(r->x_max - r->x_min) / (float)RMRT_UNIT;
		float gyf = (float)(r->y_max - r->y_min) / (float)RMRT_UNIT;
		int gx = (int)floor(gxf);
		int gy = (int)floor(gyf);
		int x_off = RMRT_UNIT*(gxf - gx)/2 + r->x_min;
		int y_off = RMRT_UNIT*(gyf - gy)/2 + r->y_min;
		int **grid = (int **)malloc(sizeof(int *) * gx);
		int i=0;
		int j=0;

		gx--;
		gy--;

		if (grid) {
			int *pts = (int *)malloc(sizeof(int)*gx*gy);
			int x;
			int y;
			int rx1, rx2, ry1, ry2;
			int idx = 0;
			int k=0;

			for (i=0; i<gx; i++) {
				grid[i] = (int *)malloc(sizeof(int) * gy);
				x = RMRT_UNIT * i / rx;				
				for (j=0; j<gy; j++) {
					y = RMRT_UNIT * j / ry;
					if (g[x][y] == NULL) {
						grid[i][j] = -2;
					} else {
						grid[i][j] = -1;
					}
				}
			}

			for (i=0; i<gx; i++) {
				for (j=0; j<gy; j++) {
					pts[k] = i * gy + j;
					k++;
				}
			}

			shuffle(pts, gx * gy);

			// 2x2
			rtb_find(pts, gx * gy, grid, gx, gy, 1, 1, &idx);

			// 1x3
			rtb_find(pts, gx * gy, grid, gx, gy, 0, 2, &idx);

			// 3x1
			rtb_find(pts, gx * gy, grid, gx, gy, 2, 0, &idx);

			// 1x2
			rtb_find(pts, gx * gy, grid, gx, gy, 0, 1, &idx);

			// 2x1
			rtb_find(pts, gx * gy, grid, gx, gy, 1, 0, &idx);

			// 1x1
			rtb_find(pts, gx * gy, grid, gx, gy, 0, 0, &idx);

			for (i=0; i<idx; i++) {
				rtb_building(m, r, grid, gx, gy, i, x_off, y_off);
			}

			for (i=0; i<gx; i++) {
				free(grid[i]);
			}
			free(grid);
		}        

		region_free_grid(g, grid_x);
	}
}

RMOD rmod_rooftops = { "buildings", rmod_rooftops_brushes, NULL, NULL };

