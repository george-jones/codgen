
#include <math.h>

#include "map.h"
#include "rmods.h"
#include "codgen_random.h"
#include "primitives.h"
#include "terrain.h"
#include "codgen.h"
#include "output_terrain.h"
#include "region.h"

static void rmod_mound_set_edges(MAP *m, MAP *moundmap, REGION *r, int grid_size, short *drill)
{
	TERRAIN *t = NULL;
	float h,h2;
	int under=10;
	int i=0,j=0,x=0,y=0;
	FloatPoint3 p;

	t = moundmap->terrain;

	// flatten drilled points
	if (drill) {
		for (i=0; i<grid_size; i++) {
			for (j=0; j<grid_size; j++) {
				if (drill[i + j*grid_size]) {
					h = terrain_height_at_point(m->terrain, (float)i*moundmap->grid_x_unit+moundmap->x_offset,
												(float)j*moundmap->grid_y_unit+moundmap->y_offset,
												m->x_size, m->y_size, m->height) - under;
					h2 = h / (float)moundmap->height;
					terrain_set_point(t,i,j,h2);
				}
			}
		}
	}

	// flatten edges to make sure you can't see under the mesh.  no peeking!
	j=0;
	for (i=0; i<grid_size; i++) {
		h = terrain_height_at_point(m->terrain, (float)i*moundmap->grid_x_unit+moundmap->x_offset,
									(float)j*moundmap->grid_y_unit+moundmap->y_offset,
									m->x_size, m->y_size, m->height) - under;
		h2 = h / (float)moundmap->height;
		terrain_set_point(t,i,j,h2);
	}

	j=grid_size-1;
	for (i=0; i<grid_size; i++) {
		h = terrain_height_at_point(m->terrain, (float)i*moundmap->grid_x_unit+moundmap->x_offset,
									(float)j*moundmap->grid_y_unit+moundmap->y_offset,
									m->x_size, m->y_size, m->height) - under;
		h2 = h / (float)moundmap->height;
		terrain_set_point(t,i,j,h2);
	}

	i=0;
	for (j=0; j<grid_size; j++) {
		h = terrain_height_at_point(m->terrain, (float)i*moundmap->grid_x_unit+moundmap->x_offset,
									(float)j*moundmap->grid_y_unit+moundmap->y_offset,
									m->x_size, m->y_size, m->height) - under;
		h2 = h / (float)moundmap->height;
		terrain_set_point(t,i,j,h2);
	}

	i=grid_size-1;
	for (j=0; j<grid_size; j++) {
		h = terrain_height_at_point(m->terrain, (float)i*moundmap->grid_x_unit+moundmap->x_offset,
									(float)j*moundmap->grid_y_unit+moundmap->y_offset,
									m->x_size, m->y_size, m->height) - under;
		h2 = h / (float)moundmap->height;
		terrain_set_point(t,i,j,h2);
	}	

	// flatten points that aren't actually in the region.
	for (i=0; i<grid_size; i++) {
		x = i * moundmap->x_size / grid_size;
		p.x = moundmap->x_offset + x;
		for (j=0; j<grid_size; j++) {
			y = j * moundmap->y_size / grid_size;
			p.y = moundmap->y_offset + y;
			if (!point_in_region(m, r, &p)) {
				h = terrain_height_at_point(m->terrain, p.x, p.y, m->x_size, m->y_size, m->height) - under;
				h2 = h / (float)moundmap->height;
				terrain_set_point(t,i,j,h2);
			}
		}
	}	
}

static void rmod_mound_get_drill_pts(short *pts, int grid_size)
{
	int i=0;
	int j=0;

	// set points to 0 initially
	for (i=0; i<grid_size; i++) {
		for (j=0; j<grid_size; j++) {
			pts[i+j*grid_size] = 0;
		}
	}

	// moving up and to the right, starting at the center
	i = grid_size / 2;
	j = grid_size / 2;
	while (i < grid_size && j < grid_size) {
		pts[i+j*grid_size] = 1;
		if (genrand(2) == 0) {
			i++;
		} else {
			j++;
		}
	}

	i = grid_size / 2;
	j = grid_size / 2;

	// moving down and to the left, starting at the center
	while (i > 0 && j > 0) {
		pts[i+j*grid_size] = 1;
		if (genrand(2) == 0) {
			i--;
		} else {
			j--;
		}
	}

}

static void rmod_mound_terrain(MAP *m, REGION *r)
{
	DOMNODE *node_mound = NULL;
	MAP *moundmap;
	TERRAIN *t = NULL;
	int grid_size = 0;
	int i=0,j=0,x=0,y=0;
	FloatPoint3 p;
	int over = 125;
	int num_blur = 7;
	short *drill_pts = NULL;	

	// fixes problem with bogus meshes being produced.  although I can't really explain
	// how this would happen in the first place.
	if (r->x_max - r->x_min == 0 || r->y_max - r->y_min == 0) {
		return;
	}

	moundmap = (MAP *)calloc(1, sizeof(MAP));
	if (!moundmap) {
		fprintf(stderr, "ERROR: Out of memory making mound.");
	}

	node_mound = XMLDomGetChildNamed(XMLDomGetChildNamed(m->conf, "regions"), "mound");
	if (strlen(m->mound_texture) == 0) {
		char *txt = XMLDomGetAttribute(pick_child_random(node_mound), "name");
		strncpy(m->mound_texture, txt, sizeof(m->mound_texture));
	}

	fprintf(stderr, "Mounds\n");

	moundmap->map_file = m->map_file;
	moundmap->height = r->z_max + over;
	moundmap->x_offset = r->x_min;
	moundmap->x_size = r->x_max - r->x_min;
	moundmap->y_offset = r->y_min;
	moundmap->y_size = r->y_max - r->y_min;
	moundmap->terrain_texture = m->mound_texture;	
	moundmap->buildings = NULL;	

	grid_size = (int)(sqrt((double)r->num_points));
	if (grid_size < 12) grid_size = 12;

	moundmap->grid_x_unit = (float)moundmap->x_size / ((float)grid_size-1);
	moundmap->grid_y_unit = (float)moundmap->y_size / ((float)grid_size-1);

	t = terrain_create(grid_size, grid_size);
	if (!t) {
		// barf
		fprintf(stderr, "ERROR: Out of memory making mound terrain.");
		return;
	}
	moundmap->terrain = t;	

	for (i=0; i<grid_size; i++) {
		for (j=0; j<grid_size; j++) {
			terrain_set_point(t, i, j, 1.0);
		}
	}

	// drill thru	
	drill_pts = (short *)malloc(sizeof(short) * grid_size * grid_size);
	if (drill_pts) {
		rmod_mound_get_drill_pts(drill_pts, grid_size);		
	} else {
		fprintf(stderr, "ERROR: Out of memory making mound points.");
	}

	// set edges and smooth
	rmod_mound_set_edges(m, moundmap, r, grid_size, drill_pts);

	for (i=0; i<num_blur; i++) {	
		terrain_blur(t);
		rmod_mound_set_edges(m, moundmap, r, grid_size, (i < 3*num_blur/4)?drill_pts:NULL);
	}

	free(drill_pts);

	// now go!
	moundmap->buildings = m->buildings;
	output_terrain(moundmap);

	r->mod_data = moundmap;
}

static void rmod_mound_cleanup(REGION *r)
{
	MAP *region_map = NULL;

	region_map = (MAP *)r->mod_data;
	if (region_map) {
		if (region_map->terrain) terrain_destroy(region_map->terrain);
		free(region_map);
	}
}

RMOD rmod_mound = { "mound", NULL, rmod_mound_terrain, NULL, rmod_mound_cleanup };

