
#include <math.h>

#include "map.h"
#include "rmods.h"
#include "codgen_random.h"
#include "primitives.h"
#include "terrain.h"
#include "codgen.h"
#include "output_terrain.h"
#include "region.h"

static void rmod_rocks_set_edges(MAP *m, MAP *rockmap, REGION *r, int grid_size)
{
	TERRAIN *t = NULL;
	float h,h2;
	int under=140;
	int i=0,j=0,x=0,y=0;
	FloatPoint3 p;

	t = rockmap->terrain;

	// flatten edges to make sure you can't see under the mesh.  no peeking!
	j=0;
	for (i=0; i<grid_size; i++) {
		h = terrain_height_at_point(m->terrain, (float)i*rockmap->grid_x_unit+rockmap->x_offset,
									(float)j*rockmap->grid_y_unit+rockmap->y_offset,
									m->x_size, m->y_size, m->height) - under;
		h2 = h / (float)rockmap->height;
		terrain_set_point(t,i,j,h2);
	}

	j=grid_size-1;
	for (i=0; i<grid_size; i++) {
		h = terrain_height_at_point(m->terrain, (float)i*rockmap->grid_x_unit+rockmap->x_offset,
									(float)j*rockmap->grid_y_unit+rockmap->y_offset,
									m->x_size, m->y_size, m->height) - under;
		h2 = h / (float)rockmap->height;
		terrain_set_point(t,i,j,h2);
	}

	i=0;
	for (j=0; j<grid_size; j++) {
		h = terrain_height_at_point(m->terrain, (float)i*rockmap->grid_x_unit+rockmap->x_offset,
									(float)j*rockmap->grid_y_unit+rockmap->y_offset,
									m->x_size, m->y_size, m->height) - under;
		h2 = h / (float)rockmap->height;
		terrain_set_point(t,i,j,h2);
	}

	i=grid_size-1;
	for (j=0; j<grid_size; j++) {
		h = terrain_height_at_point(m->terrain, (float)i*rockmap->grid_x_unit+rockmap->x_offset,
									(float)j*rockmap->grid_y_unit+rockmap->y_offset,
									m->x_size, m->y_size, m->height) - under;
		h2 = h / (float)rockmap->height;
		terrain_set_point(t,i,j,h2);
	}	

	// flatten points that aren't actually in the region.
	for (i=0; i<grid_size; i++) {
		x = i * rockmap->x_size / grid_size;
		p.x = rockmap->x_offset + x;
		for (j=0; j<grid_size; j++) {
			y = j * rockmap->y_size / grid_size;
			p.y = rockmap->y_offset + y;
			if (!point_in_region(m, r, &p)) {
				h = terrain_height_at_point(m->terrain, p.x, p.y, m->x_size, m->y_size, m->height) - under;
				h2 = h / (float)rockmap->height;
				terrain_set_point(t,i,j,h2);
			}
		}
	}	
}

static void rmod_rocks_terrain(MAP *m, REGION *r)
{
	DOMNODE *node_rocks = NULL;
	MAP *rockmap;
	TERRAIN *t = NULL;	
	int grid_size = 0;
	int i=0,j=0,x=0,y=0;
	FloatPoint3 p;

	// fixes problem with bogus meshes being produced.  although I can't really explain
	// how this would happen in the first place.
	if (r->x_max - r->x_min == 0 || r->y_max - r->y_min == 0) {
		return;
	}

	rockmap = (MAP *)calloc(1, sizeof(MAP));
	if (!rockmap) {
		fprintf(stderr, "ERROR: Out of memory making rocks.");
	}

	node_rocks = XMLDomGetChildNamed(XMLDomGetChildNamed(m->conf, "regions"), "rocks");
	if (strlen(m->rock_texture) == 0) {
		char *txt = XMLDomGetAttribute(pick_child_random(node_rocks), "name");
		strncpy(m->rock_texture, txt, sizeof(m->rock_texture));
	}

	fprintf(stderr, "Rocks\n");

	rockmap->map_file = m->map_file;
	rockmap->height = r->z_max + 100;
	rockmap->x_offset = r->x_min;
	rockmap->x_size = r->x_max - r->x_min;
	rockmap->y_offset = r->y_min;
	rockmap->y_size = r->y_max - r->y_min;
	rockmap->terrain_texture = m->rock_texture;
	rockmap->buildings = NULL;	

	grid_size = (int)(sqrt((double)r->num_points));
	if (grid_size < 8) grid_size = 8;

	rockmap->grid_x_unit = (float)rockmap->x_size / ((float)grid_size-1);
	rockmap->grid_y_unit = (float)rockmap->y_size / ((float)grid_size-1);

	t = terrain_generate(grid_size, grid_size, XMLDomGetAttributeFloat(node_rocks, "smoothness", 0.2));
	if (!t) {
		// barf
		fprintf(stderr, "ERROR: Out of memory making rocks terrain.");
		return;
	}
	rockmap->terrain = t;

	// set edges
	rmod_rocks_set_edges(m, rockmap, r, grid_size);

	// now go!
	rockmap->buildings = m->buildings;
	output_terrain(rockmap);

	r->mod_data = rockmap;
}

static void rmod_rocks_cleanup(REGION *r)
{
	MAP *region_map = NULL;

	region_map = (MAP *)r->mod_data;
	if (region_map) {
		if (region_map->terrain) terrain_destroy(region_map->terrain);
		free(region_map);
	}
}

RMOD rmod_rocks = { "rocks", NULL, rmod_rocks_terrain, NULL, rmod_rocks_cleanup };

