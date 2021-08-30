#include <math.h>

#include "rmods.h"
#include "codgen_random.h"
#include "primitives.h"
#include "rmod_buildings.h"
#include "codgen.h"

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
static int RMRT_STAIR_HEIGHT=6;
static int RMRT_STAIR_LENGTH=8;
static int RMRT_DOOR_WIDTH=100;
static int RMRT_DOOR_HEIGHT=100;
static int RMRT_WINDOW_WIDTH=40;
static int RMRT_WINDOW_HEIGHT=40;
static int RMRT_WINDOW_BOTTOM=30;

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

static int rtb_terrain_height(MAP *m, FloatPoint3 *fp, int use_min)
{
	TERRAIN *t = NULL;
	float x_mult=0.0;
	float y_mult=0.0;
	float i1,i2,j1,j2;
	float i,j;
	int extreme_h=-1;
	int h=0;

	extreme_h = fp[0].z; // just a starting point

	t = m->terrain;
	
	j = fp[0].y;
	for (i=fp[0].x; i<=fp[3].x; i++) {   
		h = terrain_height_at_point(t,i,j,m->x_size,m->y_size,m->height);
		if ((use_min && h<extreme_h) || (!use_min && h>extreme_h)) {
			extreme_h = h;
		}
	}

	j = fp[3].y;
	for (i=fp[0].x; i<=fp[3].x; i++) {		
		h = terrain_height_at_point(t,i,j,m->x_size,m->y_size,m->height);
		if ((use_min && h<extreme_h) || (!use_min && h>extreme_h)) {
			extreme_h = h;
		}
	}

	i = fp[0].x;
	for (j=fp[0].y; j<=fp[3].y; j++) {
		h = terrain_height_at_point(t,i,j,m->x_size,m->y_size,m->height);
		if ((use_min && h<extreme_h) || (!use_min && h>extreme_h)) {
			extreme_h = h;
		}
	}

	i = fp[3].x;
	for (j=fp[0].y; j<=fp[3].y; j++) {
		h = terrain_height_at_point(t,i,j,m->x_size,m->y_size,m->height);
		if ((use_min && h<extreme_h) || (!use_min && h>extreme_h)) {
			extreme_h = h;
		}
	}

	return extreme_h;
}

static void highlow(MAP *m, int i1, int i2, int j, int x_0_or_y_1, int *high, int *low)
{
	int i=0;
	int inc = 1;
	float hi = 0;
	float lo = 0;
	float x;
	float y;
	float h = 0.0;

	// this way caller doesn't need to worry about putting i1 and i2 in any particular order
	if (i1 > i2) {
		int tmp = i1;
		i1 = i2;
		i2 = tmp;
	}

	if (x_0_or_y_1 == 0) {
		y = (float)j;
	} else {
		x = (float)j;
	}

	for (i=i1; i<=i2; i++) {		
		if (x_0_or_y_1 == 0) {
			x = (float)i;			
		} else {
			y = (float)i;
		}
		h = map_height_at_point(m, x, y, NULL);
		if (i==i1) {
			hi = h;
			lo = h;
		} else {
			if (h > hi) {
				hi = h;
			}
			if (h < lo) {
				lo = h;
			}
		}
	}

	*high = (int)ceil(hi);
	*low = (int)floor(lo);
}

static void rtb_ow_solid(MAP *m, char *txtr, int i, IntPoint3 *p, int base_height)
{
	box(m->map_file, p, base_height-RMRT_WALL_THICKNESS, txtr);
}


static void rtb_ow_two_windows(MAP *m, char *txtr, int i, IntPoint3 *p, int base_height)
{
	rtb_ow_solid(m,txtr,i,p,base_height);
}

static void rtb_ow_doorway(MAP *m, char *txtr, int i, IntPoint3 *p, int base_height, int windows)
{
	IntPoint3 q[4]; // doorway
	int i1,i2;
	int high, low;
	int wall_len = 0;

	switch (i) {
	case 0:

		wall_len = p[0].y - p[3].y;

		i1 = (p[0].y + p[3].y)/2 - RMRT_DOOR_WIDTH/2;
		i2 = i1 + RMRT_DOOR_WIDTH;
		highlow(m,i1,i2,p[0].x,1,&high,&low);

		if (high < p[0].z) high = p[0].z;		
		
		if (base_height - RMRT_WALL_THICKNESS - (high - p[0].z) >= RMRT_DOOR_HEIGHT) {
			q[0].x = q[3].x = p[0].x;
			q[1].x = q[2].x = p[1].x;
			q[0].y = q[1].y = i2;
			q[2].y = q[3].y = i1;
			q[0].z = q[1].z = q[2].z = q[3].z = high;
			portal(m->map_file, q, RMRT_DOOR_HEIGHT, CUBE_FACE_LEFT);
		} else {
			rtb_ow_solid(m,txtr,i,p,base_height);
			return;
		}

		break;
	case 1:

		wall_len = p[1].x - p[0].x;

		i1 = (p[1].x + p[0].x)/2 - RMRT_DOOR_WIDTH/2;
		i2 = i1 + RMRT_DOOR_WIDTH;
		highlow(m,i1,i2,p[0].y,0,&high,&low);

		if (high < p[0].z) high = p[0].z;		
		
		if (base_height - RMRT_WALL_THICKNESS - (high - p[0].z) >= RMRT_DOOR_HEIGHT) {
			q[0].x = q[3].x = i1;
			q[1].x = q[2].x = i2;
			q[0].y = q[1].y = p[0].y;
			q[2].y = q[3].y = p[2].y;
			q[0].z = q[1].z = q[2].z = q[3].z = high;
			portal(m->map_file, q, RMRT_DOOR_HEIGHT, CUBE_FACE_BACK);
		} else {
			rtb_ow_solid(m,txtr,i,p,base_height);
			return;
		}
		
		break;
	case 2:

		wall_len = p[0].y - p[3].y;

		i1 = (p[0].y + p[3].y)/2 - RMRT_DOOR_WIDTH/2;
		i2 = i1 + RMRT_DOOR_WIDTH;
		highlow(m,i1,i2,p[0].x,1,&high,&low);

		if (high < p[0].z) high = p[0].z;		
		
		if (base_height - RMRT_WALL_THICKNESS - (high - p[0].z) >= RMRT_DOOR_HEIGHT) {
			q[0].x = q[3].x = p[0].x;
			q[1].x = q[2].x = p[2].x;
			q[0].y = q[1].y = i2;
			q[2].y = q[3].y = i1;
			q[0].z = q[1].z = q[2].z = q[3].z = high;
			portal(m->map_file, q, RMRT_DOOR_HEIGHT, CUBE_FACE_RIGHT);
		} else {
			rtb_ow_solid(m,txtr,i,p,base_height);
			return;
		}

		break;
	case 3:

		wall_len = p[1].x - p[0].x;

		i1 = (p[1].x + p[0].x)/2 - RMRT_DOOR_WIDTH/2;
		i2 = i1 + RMRT_DOOR_WIDTH;
		highlow(m,i1,i2,p[0].y,0,&high,&low);

		if (high < p[0].z) high = p[0].z;		

		if (base_height - RMRT_WALL_THICKNESS - (high - p[0].z) >= RMRT_DOOR_HEIGHT) {
			q[0].x = q[3].x = i1;
			q[1].x = q[2].x = i2;
			q[0].y = q[1].y = p[0].y;
			q[2].y = q[3].y = p[2].y;
			q[0].z = q[1].z = q[2].z = q[3].z = high;
			portal(m->map_file, q, RMRT_DOOR_HEIGHT, CUBE_FACE_FRONT);
		} else {
			rtb_ow_solid(m,txtr,i,p,base_height);
			return;
		}

		break;
	}	

	// above doorway
	q[0].z = q[1].z = q[2].z = q[3].z = high + RMRT_DOOR_HEIGHT;
	box(m->map_file, q, p[0].z + base_height - q[0].z - RMRT_WALL_THICKNESS, txtr);

	q[0].z = q[1].z = q[2].z = q[3].z = p[0].z;

	// below doorway
	if (high > p[0].z) {
		box(m->map_file, q, high - p[0].z, txtr);
	}

	// stairs down into house
	if (high - p[0].z >= RMRT_STAIR_HEIGHT) {
		int staircase_len = 0;
		int num_steps = 0;
		int j = 0;
		int staircase_start = 0;		

		num_steps = (high - p[0].z) / RMRT_STAIR_HEIGHT;
		if ((high - p[0].z) % RMRT_STAIR_HEIGHT) num_steps++; 
		staircase_len = num_steps * RMRT_STAIR_LENGTH;
		
		switch (i) {
		case 0:
			staircase_start = p[0].x + RMRT_WALL_THICKNESS + staircase_len;
			break;
		case 1:
			staircase_start = p[0].y - RMRT_WALL_THICKNESS - staircase_len;
			break;
		case 2:
			staircase_start = p[1].x - RMRT_WALL_THICKNESS - staircase_len;	
			break;
		case 3:
			staircase_start = p[3].y + RMRT_WALL_THICKNESS + staircase_len;
			break;
		}

	    for (j=0; j<num_steps; j++) {

			q[0].z = q[1].z = q[2].z = q[3].z = p[0].z + j*RMRT_STAIR_HEIGHT;

			switch (i) {		
			case 0:
				q[0].x = q[3].x = staircase_start - (j+1)*RMRT_STAIR_LENGTH;
				q[1].x = q[2].x = staircase_start - j*RMRT_STAIR_LENGTH;
				break;
			case 1:
				q[0].y = q[1].y = staircase_start + (j+1)*RMRT_STAIR_LENGTH;
				q[2].y = q[3].y = staircase_start + j*RMRT_STAIR_LENGTH;
				break;
			case 2:
				q[0].x = q[3].x = staircase_start + j*RMRT_STAIR_LENGTH;
				q[1].x = q[2].x = staircase_start + (j+1)*RMRT_STAIR_LENGTH;
				break;
			case 3:
				q[0].y = q[1].y = staircase_start - j*RMRT_STAIR_LENGTH;
				q[2].y = q[3].y = staircase_start - (j+1)*RMRT_STAIR_LENGTH;
				break;		
			}
			box(m->map_file, q, RMRT_STAIR_HEIGHT, txtr);
		}
	}

	switch (i) {
	case 0:
		i1 = p[0].y;
		i2 = p[3].y;
		highlow(m,i1,i2,p[0].x,1,&high,&low);
		break;
	case 1:
		i1 = p[1].x;
		i2 = p[0].x;
		highlow(m,i1,i2,p[0].y,0,&high,&low);
		break;
	case 2:
		i1 = p[0].y;
		i2 = p[3].y;
		highlow(m,i1,i2,p[1].x,1,&high,&low);
		break;
	case 3:
		i1 = p[1].x;
		i2 = p[0].x;
		highlow(m,i1,i2,p[2].y,0,&high,&low);
		break;
	}

	if (!windows ||
		high + RMRT_WINDOW_HEIGHT + RMRT_WALL_THICKNESS >= p[0].z + base_height ||
		RMRT_WINDOW_WIDTH*2 + RMRT_DOOR_WIDTH + RMRT_WALL_THICKNESS*4 >= wall_len) {	
		IntPoint3 q2[4];

		q2[0].z = q2[1].z = q2[2].z = q2[3].z = p[0].z;
		q2[0].x = q2[3].x = p[0].x;
		q2[1].x = q2[2].x = (q[0].x > p[0].x)? q[0].x : p[1].x;
		q2[0].y = q2[1].y = p[0].y;
		q2[2].y = q2[3].y = (q[0].y < p[0].y)? q[0].y : p[2].y;
		box(m->map_file, q2, base_height-RMRT_WALL_THICKNESS, txtr);

		q2[0].x = q2[3].x = (q[1].x < p[2].x)? q[1].x : p[0].x;
		q2[1].x = q2[2].x = p[1].x;
		q2[0].y = q2[1].y = (q[2].y > p[2].y)? q[2].y : p[0].y;
		q2[2].y = q2[3].y = p[2].y;
		box(m->map_file, q2, base_height-RMRT_WALL_THICKNESS, txtr);
	} else {		
		IntPoint3 q2[4];
		int w_bottom=0;
		
		w_bottom = high-p[0].z;
		if (w_bottom < RMRT_WINDOW_BOTTOM) w_bottom = RMRT_WINDOW_BOTTOM;

		// top
		q2[0].z = q2[1].z = q2[2].z = q2[3].z = p[0].z + w_bottom + RMRT_WINDOW_HEIGHT;
		q2[0].x = q2[3].x = p[0].x;
		q2[1].x = q2[2].x = (q[0].x > p[0].x)? q[0].x : p[1].x;
		q2[0].y = q2[1].y = p[0].y;
		q2[2].y = q2[3].y = (q[0].y < p[0].y)? q[0].y : p[2].y;
		box(m->map_file, q2, base_height - RMRT_WALL_THICKNESS - w_bottom - RMRT_WINDOW_HEIGHT, txtr);

		// bottom
		q2[0].z = q2[1].z = q2[2].z = q2[3].z = p[0].z;
		q2[0].x = q2[3].x = p[0].x;
		q2[1].x = q2[2].x = (q[0].x > p[0].x)? q[0].x : p[1].x;
		q2[0].y = q2[1].y = p[0].y;
		q2[2].y = q2[3].y = (q[0].y < p[0].y)? q[0].y : p[2].y;
		box(m->map_file, q2, w_bottom, txtr);

		// side 1
		q2[0].z = q2[1].z = q2[2].z = q2[3].z = p[0].z + w_bottom;
		switch (i) {
		case 0:
		case 2:
			{			
				int h = q2[2].y - q2[0].y;
				int new_y = q2[0].y + h/2 + RMRT_WINDOW_WIDTH/2;			
				q2[2].y = q2[3].y = new_y;				
			}
			break;
		case 1:
		case 3:
			{
				int w = q2[1].x - q2[0].x;
				int new_x = q2[0].x + w/2 - RMRT_WINDOW_WIDTH/2;			
				q2[1].x = q2[2].x = new_x;
			}
			break;
		}
		box(m->map_file, q2, RMRT_WINDOW_HEIGHT, txtr);

		// portal for window
		switch (i) {
		case 0:
		case 2:
			q2[0].y = q2[1].y = q2[2].y;
			q2[2].y = q2[3].y = q2[0].y - RMRT_WINDOW_WIDTH;
			portal(m->map_file, q2, RMRT_WINDOW_HEIGHT, (i==0)?CUBE_FACE_LEFT:CUBE_FACE_RIGHT);
			break;
		case 1:
		case 3:
			q2[0].x = q2[3].x = q2[2].x;
			q2[1].x = q2[2].x = q2[0].x + RMRT_WINDOW_WIDTH;
			portal(m->map_file, q2, RMRT_WINDOW_HEIGHT, (i==3)?CUBE_FACE_FRONT:CUBE_FACE_BACK);
			break;
		}

		// side 2
		switch (i) {
		case 0:
		case 2:
			{			
				q2[0].y = q2[1].y = q2[2].y;
				q2[2].y = q2[3].y = q[2].y + RMRT_DOOR_WIDTH;
			}
			break;
		case 1:
		case 3:
			{
				q2[0].x = q2[3].x = q2[2].x;
				q2[1].x = q2[2].x = q[2].x - RMRT_DOOR_WIDTH;
			}
			break;
		}
		box(m->map_file, q2, RMRT_WINDOW_HEIGHT, txtr);

		// the other side

		// top
		q2[0].z = q2[1].z = q2[2].z = q2[3].z = p[0].z + w_bottom + RMRT_WINDOW_HEIGHT;
		q2[0].x = q2[3].x = (q[1].x < p[2].x)? q[1].x : p[0].x;
		q2[1].x = q2[2].x = p[1].x;
		q2[0].y = q2[1].y = (q[2].y > p[2].y)? q[2].y : p[0].y;
		q2[2].y = q2[3].y = p[2].y;
		box(m->map_file, q2, base_height - RMRT_WALL_THICKNESS - w_bottom - RMRT_WINDOW_HEIGHT, txtr);

		// bottom
		q2[0].z = q2[1].z = q2[2].z = q2[3].z = p[0].z;
		q2[0].x = q2[3].x = (q[1].x < p[2].x)? q[1].x : p[0].x;
		q2[1].x = q2[2].x = p[1].x;
		q2[0].y = q2[1].y = (q[2].y > p[2].y)? q[2].y : p[0].y;
		q2[2].y = q2[3].y = p[2].y;
		box(m->map_file, q2, w_bottom, txtr);

		// side 1
		q2[0].z = q2[1].z = q2[2].z = q2[3].z = p[0].z + w_bottom;
		switch (i) {
		case 0:
		case 2:
			{
				int h = q2[2].y - q2[0].y;
				int new_y = q2[0].y + h/2 + RMRT_WINDOW_WIDTH/2;			
				q2[2].y = q2[3].y = new_y;				
			}
			break;
		case 1:
		case 3:
			{
				int w = q2[1].x - q2[0].x;
				int new_x = q2[0].x + w/2 - RMRT_WINDOW_WIDTH/2;			
				q2[1].x = q2[2].x = new_x;
			}
			break;
		}
		box(m->map_file, q2, RMRT_WINDOW_HEIGHT, txtr);

		// portal for window
		switch (i) {
		case 0:
		case 2:
			q2[0].y = q2[1].y = q2[2].y;
			q2[2].y = q2[3].y = q2[0].y - RMRT_WINDOW_WIDTH;
			portal(m->map_file, q2, RMRT_WINDOW_HEIGHT, (i==0)?CUBE_FACE_LEFT:CUBE_FACE_RIGHT);
			break;
		case 1:
		case 3:
			q2[0].x = q2[3].x = q2[2].x;
			q2[1].x = q2[2].x = q2[0].x + RMRT_WINDOW_WIDTH;
			portal(m->map_file, q2, RMRT_WINDOW_HEIGHT, (i==3)?CUBE_FACE_FRONT:CUBE_FACE_BACK);
			break;
		}

		// side 2		
		switch (i) {
		case 0:
		case 2:
			{			
				q2[0].y = q2[1].y = q2[2].y;
				q2[2].y = q2[3].y = p[2].y;				
			}
			break;
		case 1:
		case 3:
			{
				q2[0].x = q2[3].x = q2[2].x;
				q2[1].x = q2[2].x = p[2].x;				
			}
			break;
		}
		
		box(m->map_file, q2, RMRT_WINDOW_HEIGHT, txtr);
	}

}

static void rtb_ow_two_doorways(MAP *m, char *txtr, int i, IntPoint3 *p, int base_height)
{
	rtb_ow_solid(m,txtr,i,p,base_height);
}

static void rtb_ow_window_strip(MAP *m, char *txtr, int i, IntPoint3 *p, int base_height)
{
	rtb_ow_solid(m,txtr,i,p,base_height);
}

static void rtb_ow_get_coords(IntPoint3 *pt, int x_s, int y_s, int base_height, int i, IntPoint3 *p)
{
	p[0].z = p[1].z = p[2].z = p[3].z = pt->z + RMRT_WALL_THICKNESS;
	switch (i) {
	case 0:
		p[0].x = p[3].x = pt->x;
		p[1].x = p[2].x = pt->x + RMRT_WALL_THICKNESS;
		p[0].y = p[1].y = pt->y + y_s;
		p[2].y = p[3].y = pt->y;
		break;
	case 1:
		p[0].x = p[3].x = pt->x + RMRT_WALL_THICKNESS;
		p[1].x = p[2].x = pt->x + x_s - RMRT_WALL_THICKNESS;
		p[0].y = p[1].y = pt->y + y_s;
		p[2].y = p[3].y = pt->y + y_s - RMRT_WALL_THICKNESS;
		break;
	case 2:
		p[0].x = p[3].x = pt->x + x_s - RMRT_WALL_THICKNESS;
		p[1].x = p[2].x = pt->x + x_s;
		p[0].y = p[1].y = pt->y + y_s;
		p[2].y = p[3].y = pt->y;		
		break;
	case 3:
		p[0].x = p[3].x = pt->x + RMRT_WALL_THICKNESS;;
		p[1].x = p[2].x = pt->x + x_s - RMRT_WALL_THICKNESS;
		p[0].y = p[1].y = pt->y + RMRT_WALL_THICKNESS;
		p[2].y = p[3].y = pt->y;
		break;
	}
}

static void rtb_outside_wall(MAP *m, IntPoint3 *pt, int x_s, int y_s, int base_height, char *txtr, int i)
{
	IntPoint3 p[4];
	int walltype = 0;	

	walltype = genrand(2);

	rtb_ow_get_coords(pt,x_s,y_s,base_height,i,p);
	rtb_ow_doorway(m,txtr,i,p,base_height,1);// walltype
	//rtb_ow_solid(m,txtr,i,p,base_height);

	/*
	switch (walltype) {
	case 0:
		rtb_ow_one_doorway(m,txtr,i,p,base_height,0);		
		break;
	case 1:
		//rtb_ow_one_doorway(m,txtr,i,p,base_height);
		rtb_ow_one_doorway(m,txtr,i,p,base_height,0);
		break;
	case 2:
		rtb_ow_one_doorway(m,txtr,i,p,base_height,0);
		//rtb_ow_two_doorways(m,txtr,i,p,base_height);
		break;
	case 3:
		rtb_ow_one_doorway(m,txtr,i,p,base_height,0);
		//rtb_ow_two_windows(m,txtr,i,p,base_height);
		break;
	case 4:
		rtb_ow_one_doorway(m,txtr,i,p,base_height,0);
		//rtb_ow_window_strip(m,txtr,i,p,base_height);
		break;
	}
	*/
}

// walls: 0=left, 1=top, 2=right, 3=bottom
// arranged:
// 
// 01111112
// 0      2
// 0      2
// 03333332
//
static void building_first_floor(MAP *m, IntPoint3 *pt, int x_s, int y_s, int base_height, char *txtr)
{
	IntPoint3 p[4];
	int stair_side = 0;
	int num_steps = 0;
	int staircase_len = 0;
	int staircase_start = 0;
	int i = 0;

	// floor
	p[0].x = pt->x;
	p[0].y = pt->y + y_s;

	p[1].x = pt->x + x_s;
	p[1].y = pt->y + y_s;

	p[2].x = pt->x + x_s;
	p[2].y = pt->y;

	p[3].x = pt->x;
	p[3].y = pt->y;

	p[0].z = p[1].z = p[2].z = p[3].z = pt->z;
	box(m->map_file, p, RMRT_WALL_THICKNESS, txtr);

	// figure out which side will have stairs
	if (x_s > y_s) {
		stair_side = (genrand(2)==1)?1:3;
	} else if (y_s > x_s) {
		stair_side = (genrand(2)==1)?0:2;
	} else {
		stair_side = genrand(4);
	}

	p[0].x = pt->x + RMRT_WALL_THICKNESS;
	p[0].y = pt->y + y_s - RMRT_WALL_THICKNESS;

	p[1].x = pt->x + x_s - RMRT_WALL_THICKNESS;
	p[1].y = pt->y + y_s - RMRT_WALL_THICKNESS;

	p[2].x = pt->x + x_s - RMRT_WALL_THICKNESS;
	p[2].y = pt->y + RMRT_WALL_THICKNESS;

	p[3].x = pt->x + RMRT_WALL_THICKNESS;
	p[3].y = pt->y + RMRT_WALL_THICKNESS;

	// ceiling
	p[0].z = p[1].z = p[2].z = p[3].z = pt->z + base_height - RMRT_WALL_THICKNESS;
	
	switch (stair_side) {
	case 0:
		p[0].x = p[3].x = pt->x + RMRT_WALKWAY;
		break;
	case 1:
		p[0].y = p[1].y = pt->y + y_s - RMRT_WALKWAY;
		break;
	case 2:
		p[1].x = p[2].x = pt->x + x_s - RMRT_WALKWAY;
		break;
	case 3:
		p[2].y = p[3].y = pt->y + RMRT_WALKWAY;
		break;
	}
	box(m->map_file, p, RMRT_WALL_THICKNESS, txtr);

	// landing at top of stairs
	switch (stair_side) {
	case 0:
		p[0].x = p[3].x = pt->x + RMRT_WALL_THICKNESS;
		p[1].x = p[2].x = pt->x + RMRT_WALKWAY;
		p[2].y = p[3].y = pt->y + y_s - RMRT_WALKWAY;		
		break;
	case 1:
		p[0].y = p[1].y = pt->y + y_s - RMRT_WALL_THICKNESS;
		p[2].y = p[3].y = pt->y + y_s - RMRT_WALKWAY;
		p[0].x = p[3].x = pt->x + x_s - RMRT_WALKWAY;
		break;
	case 2:
		p[0].x = p[3].x = pt->x + x_s - RMRT_WALKWAY;
		p[1].x = p[2].x = pt->x + x_s - RMRT_WALL_THICKNESS;
		p[0].y = p[1].y = pt->y + RMRT_WALKWAY;
		break;
	case 3:
		p[2].y = p[3].y = pt->y + RMRT_WALL_THICKNESS;
		p[0].y = p[1].y = pt->y + RMRT_WALKWAY;
		p[1].x = p[2].x = pt->x + RMRT_WALKWAY;
		break;
	}
	box(m->map_file, p, RMRT_WALL_THICKNESS, txtr);

	num_steps = (base_height-RMRT_WALL_THICKNESS) / RMRT_STAIR_HEIGHT;
	if ((base_height-RMRT_WALL_THICKNESS) % RMRT_STAIR_HEIGHT) num_steps++;	
	staircase_len = num_steps * RMRT_STAIR_LENGTH;

	// portal at top of staircase
	switch (stair_side) {
	case 0:
		p[0].y = p[1].y = pt->y + y_s - RMRT_WALKWAY - RMRT_STAIR_LENGTH;
		p[2].y = p[3].y = pt->y + y_s - RMRT_WALKWAY - staircase_len;
		break;
	case 1:
		p[0].x = p[3].x = pt->x + x_s - RMRT_WALKWAY - staircase_len;
		p[1].x = p[2].x = pt->x + x_s - RMRT_WALKWAY - RMRT_STAIR_LENGTH;
		break;
	case 2:
		p[0].y = p[1].y = pt->y + RMRT_WALKWAY + staircase_len;
		p[2].y = p[3].y = pt->y + RMRT_WALKWAY + RMRT_STAIR_LENGTH;
		break;
	case 3:
		p[0].x = p[3].x = pt->x + RMRT_WALKWAY + RMRT_STAIR_LENGTH;
		p[1].x = p[2].x = pt->x + RMRT_WALKWAY + staircase_len;
		break;
	}
	portal(m->map_file, p, RMRT_WALL_THICKNESS, CUBE_FACE_TOP);

	// ceiling behind the staircase
	switch (stair_side) {
	case 0:
		p[0].y = p[1].y = pt->y + y_s - RMRT_WALKWAY - staircase_len;
		p[2].y = p[3].y = pt->y + RMRT_WALL_THICKNESS;
		break;
	case 1:
		p[0].x = p[3].x = pt->x + RMRT_WALL_THICKNESS;
		p[1].x = p[2].x = pt->x + x_s - RMRT_WALKWAY - staircase_len;
		break;
	case 2:
		p[0].y = p[1].y = pt->y + y_s - RMRT_WALL_THICKNESS;;
		p[2].y = p[3].y = pt->y + RMRT_WALKWAY + staircase_len;
		break;
	case 3:
		p[0].x = p[3].x = pt->x + RMRT_WALKWAY + staircase_len;
		p[1].x = p[2].x = pt->x + x_s - RMRT_WALL_THICKNESS;
		break;
	}
	box(m->map_file, p, RMRT_WALL_THICKNESS, txtr);

	// stairs
	switch (stair_side) {
	case 0:
		staircase_start = p[0].y;
		break;
	case 1:
		staircase_start = p[1].x;
		break;
	case 2:
		staircase_start = p[2].y;
		break;
	case 3:
		staircase_start = p[0].x;
		break;
	}	

	for (i=0; i<num_steps; i++) {

		p[0].z = p[1].z = p[2].z = p[3].z = pt->z + i*RMRT_STAIR_HEIGHT + RMRT_WALL_THICKNESS;

		switch (stair_side) {		
		case 0:
			p[0].y = p[1].y = staircase_start + (i+1) * RMRT_STAIR_LENGTH;
			p[2].y = p[3].y = staircase_start + i * RMRT_STAIR_LENGTH;
			break;
		case 1:
			p[0].x = p[3].x = staircase_start + i * RMRT_STAIR_LENGTH;
			p[1].x = p[2].x = staircase_start + (i+1) * RMRT_STAIR_LENGTH;
			break;
		case 2:
			p[0].y = p[1].y = staircase_start - i * RMRT_STAIR_LENGTH;
			p[2].y = p[3].y = staircase_start - (i+1) * RMRT_STAIR_LENGTH;
			break;
		case 3:
			p[0].x = p[3].x = staircase_start - (i+1) * RMRT_STAIR_LENGTH;
			p[1].x = p[2].x = staircase_start - i * RMRT_STAIR_LENGTH;
			break;		
		}
		box(m->map_file, p, RMRT_STAIR_HEIGHT, txtr);
	}	

	// solid wall on the stairway side.  maybe.
	if (genrand(2) == 0) {
		rtb_ow_get_coords(pt,x_s,y_s,base_height,stair_side,p);
		box(m->map_file, p, base_height-RMRT_WALL_THICKNESS, txtr);
	}

	// put out the other 3 walls
	for (i=0; i<4; i++) {
		if (i != stair_side) rtb_outside_wall(m, pt, x_s, y_s, base_height, txtr, i);
	}

}

// returns height at the top of the base ( not including rail )
static int rtb_building_level(MAP *m, REGION *r, char *txtr, char *ladder_txtr,
							  int start_h, int gmin_x, int gmin_y, int gmax_x, int gmax_y,
							  int x_off, int y_off, int ladder, int level)
{
	FloatPoint3 four_points[4];	
	IntPoint3 p;
	int h;
	int side=0;
	int width,length;
	int x_s;
	int y_s;
	int z_s;
	int base_height = 0;
	int hollow_options = HOLLOW_DEFAULT;

	if (level > 1) hollow_options = HOLLOW_BOTTOMLESS;	
	base_height = (level == 1)? RMRT_HEIGHT : (level == 2)? RMRT_HEIGHT_2ND : RMRT_HEIGHT_3RD;

	if (level == 1) {
		BLDG b;
		int h1,h2,h3,h4 = 0;

		p.x = x_off + gmin_x * RMRT_UNIT + RMRT_ALLEY;
		p.y = y_off + gmin_y * RMRT_UNIT + RMRT_ALLEY;
		four_points[0].x = p.x;
		four_points[0].y = p.y;
		four_points[0].z = terrain_height_at_point(m->terrain, p.x, p.y, m->x_size, m->y_size, m->height);
	
		p.x = x_off + gmin_x * RMRT_UNIT + RMRT_ALLEY;
		p.y = y_off + (gmax_y+1) * RMRT_UNIT - RMRT_ALLEY;
		four_points[1].x = p.x;
		four_points[1].y = p.y;
		four_points[1].z = terrain_height_at_point(m->terrain, p.x, p.y, m->x_size, m->y_size, m->height);

		p.x = x_off + (gmax_x+1) * RMRT_UNIT - RMRT_ALLEY;
		p.y = y_off + gmin_y * RMRT_UNIT + RMRT_ALLEY;
		four_points[2].x = p.x;
		four_points[2].y = p.y;
		four_points[2].z = terrain_height_at_point(m->terrain, p.x, p.y, m->x_size, m->y_size, m->height);
	
		p.x = x_off + (gmax_x+1) * RMRT_UNIT - RMRT_ALLEY;
		p.y = y_off + (gmax_y+1) * RMRT_UNIT - RMRT_ALLEY;
		four_points[3].x = p.x;
		four_points[3].y = p.y;
		four_points[3].z = terrain_height_at_point(m->terrain, p.x, p.y, m->x_size, m->y_size, m->height);
	
		h = rtb_terrain_height(m, four_points, 1);

		four_points[0].z = h;
		four_points[1].z = h;
		four_points[2].z = h;
		four_points[3].z = h;

		if (TakenArea_BoxIn(m->ta, four_points, base_height)) {
			return -1;
		}

		b.points[0].z = b.points[1].z = b.points[2].z = b.points[3].z = 0;

		b.points[0].x = four_points[1].x;
		b.points[0].y = four_points[1].y;

		b.points[1].x = four_points[3].x;
		b.points[1].y = four_points[3].y;

		b.points[2].x = four_points[2].x;
		b.points[2].y = four_points[2].y;

		b.points[3].x = four_points[0].x;
		b.points[3].y = four_points[0].y;

		building_add(m->buildings, &b);

	} else {
		h = start_h;
	}

	// base
	p.x = x_off + gmin_x * RMRT_UNIT + RMRT_ALLEY + (level - 1) * RMRT_WALKWAY;
	p.y = y_off + gmin_y * RMRT_UNIT + RMRT_ALLEY + (level - 1) * RMRT_WALKWAY;
	p.z = h;
	x_s = (gmax_x - gmin_x + 1)*RMRT_UNIT - 2 * (RMRT_ALLEY + (level - 1) * RMRT_WALKWAY);
	y_s = (gmax_y - gmin_y + 1)*RMRT_UNIT - 2 * (RMRT_ALLEY + (level - 1) * RMRT_WALKWAY);

	if (ladder || level != 1) {
		if (level == 3) {			
			hollowbox(m->map_file, &p, RMRT_WALL_THICKNESS, RMRT_WALL_THICKNESS, base_height, RMRT_WALL_THICKNESS, txtr, hollow_options);
			p.x += x_s - RMRT_WALL_THICKNESS;
			hollowbox(m->map_file, &p, RMRT_WALL_THICKNESS, RMRT_WALL_THICKNESS, base_height, RMRT_WALL_THICKNESS, txtr, hollow_options);
			p.y += y_s - RMRT_WALL_THICKNESS;
			hollowbox(m->map_file, &p, RMRT_WALL_THICKNESS, RMRT_WALL_THICKNESS, base_height, RMRT_WALL_THICKNESS, txtr, hollow_options);
			p.x -= (x_s - RMRT_WALL_THICKNESS);
			hollowbox(m->map_file, &p, RMRT_WALL_THICKNESS, RMRT_WALL_THICKNESS, base_height, RMRT_WALL_THICKNESS, txtr, hollow_options);
		} else {
			hollowbox(m->map_file, &p, x_s, y_s, base_height, RMRT_WALL_THICKNESS, txtr, hollow_options);
		}
	} else {
		// make a more interesting first floor with stairs to second floor
		building_first_floor(m, &p, x_s, y_s, base_height, txtr);
	}

	p.x = x_off + gmin_x * RMRT_UNIT + RMRT_ALLEY + (level - 1) * RMRT_WALKWAY;
	p.y = y_off + gmin_y * RMRT_UNIT + RMRT_ALLEY + (level - 1) * RMRT_WALKWAY;
	p.z = h;
	x_s = (gmax_x - gmin_x + 1)*RMRT_UNIT - 2 * (RMRT_ALLEY + (level - 1) * RMRT_WALKWAY);
	y_s = (gmax_y - gmin_y + 1)*RMRT_UNIT - 2 * (RMRT_ALLEY + (level - 1) * RMRT_WALKWAY);

	z_s = base_height + RMRT_RAIL_HEIGHT;
	TakenArea_AddBoxPoint(m->ta, &p, x_s, y_s, z_s, "building", 1);

	// rail
	p.z = h + base_height - RMRT_RAIL_THICKNESS;
	hollowbox(m->map_file, &p, x_s, y_s, RMRT_RAIL_HEIGHT, RMRT_RAIL_THICKNESS, txtr, ((level<3)?HOLLOW_BOTTOMLESS:0)|HOLLOW_TOPLESS);

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
	
		p.z = h+1;

		make_ladder(m, &p, width, length, base_height + RMRT_RAIL_HEIGHT +
					RMRT_LADDER_LIP + RMRT_RAIL_THICKNESS/2 - RMRT_WALL_THICKNESS -1, ladder_txtr);
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
	int ladder = 0;

	if (gmax_x - gmin_x == 0 && gmax_y - gmin_y == 0) {
		ladder = 0;
	}

	base_h = rtb_building_level(m, r, texture, ladder_texture, 0, gmin_x, gmin_y, gmax_x, gmax_y, x_off, y_off, ladder, 1);
	if (base_h == -1) return -1; // unable to place building - bail out

	if (use_2nd_level) {
		base_h -= RMRT_WALL_THICKNESS; // because we use "bottomless" 2nd stories
		if (genrand(4) % 4 > 0) {
			h_2nd = rtb_building_level(m, r, texture, ladder_texture, base_h, gmin_x, gmin_y, gmax_x, gmax_y, x_off, y_off, 1, 2);
		} else {
			use_3rd_level = 0;
		}
	}

	if (use_3rd_level) {
		h_2nd -= RMRT_WALL_THICKNESS; // because we use "bottomless" 3rd stories
		if (genrand(3) % 3 > 0) {
			rtb_building_level(m, r, texture, ladder_texture, h_2nd, gmin_x, gmin_y, gmax_x, gmax_y, x_off, y_off, 1, 3);
		}
	}

	return base_h + RMRT_RAIL_HEIGHT;
}

static int rtb_building(MAP *m, REGION *r, int **grid, int gx, int gy, int idx, int x_off, int y_off)
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

	if (RMRT_HEIGHT_2ND == 0) use_2nd_level = 0;	
	if (RMRT_HEIGHT_3RD == 0) use_3rd_level = 0;

	return rtb_building_all(m, r, gmin_x, gmin_y, gmax_x, gmax_y, x_off, y_off, use_2nd_level, use_3rd_level);
}

static void rmod_buildings_brushes(MAP *m, REGION *r)
{
	DOMNODE *node_bldg = NULL;
	DOMNODE *node_ladders = NULL;
	MixedPoint3 ***g = NULL;
	float x_size;
	float y_size;
	int grid_x;
	int grid_y;

	x_size = m->grid_x_unit;
	y_size = m->grid_y_unit;

	fprintf(stderr, "Buildings\n");

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

			//shuffle(pts, gx * gy);

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

RMOD rmod_buildings = { "buildings", rmod_buildings_brushes, NULL, NULL, NULL };


//
// Building list functions
// 

BLDG_LIST *building_list_create()
{
	return (BLDG_LIST *)calloc(1, sizeof(BLDG_LIST));
}

void building_add(BLDG_LIST *l, BLDG *b)
{	
	l->num++;
	l->b = (BLDG*)realloc(l->b, sizeof(BLDG) * l->num);
	memcpy(&l->b[l->num-1], b, sizeof(BLDG));
}

void building_list_destroy(BLDG_LIST *l)
{
	free(l->b);
	free(l);
}

BLDG *building_contains(BLDG_LIST *l, IntPoint3 *p)
{
	BLDG *ret = NULL;
	int i=0;

	if (!l) return NULL;

	for (i=0; i<l->num; i++) {				
		ret = &l->b[i];
		if (ret->points[0].x <= p->x && ret->points[0].y >= p->y &&
			ret->points[1].x >= p->x && ret->points[1].y >= p->y &&
			ret->points[2].x >= p->x && ret->points[2].y <= p->y &&
			ret->points[3].x <= p->x && ret->points[3].y <= p->y) {
			break;
		} else {
			ret = NULL;
		}	
	}

	return ret;
}
