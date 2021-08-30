#include <math.h>

#include "map.h"
#include "rmods.h"
#include "codgen_random.h"

static RMOD *mods[] = {
	&rmod_forest,
	&rmod_bridge,
	&rmod_rocks,
	&rmod_buildings,
	&rmod_graveyard,
	&rmod_mound,
	&rmod_trench
};

static int num_mods = sizeof(mods) / sizeof(RMOD *);

#ifdef WIN32
#define strcasecmp strcmpi
#endif

void regions_eval(MAP *m)
{	
	DOMNODE *node_regions = NULL;
	DOMNODE *node_chosen = NULL;
	char *rname = NULL;
	int i = 0;
	int j = 0;

	node_regions = XMLDomGetChildNamed(m->conf, "regions");
	if (node_regions) {
		for (i=0; i<m->num_regions; i++) {
			node_chosen = pick_child_random(node_regions);
			if (node_chosen) {			
				rname = node_chosen->tagName;
				for (j=0; j<num_mods; j++) {				
					if (strcasecmp(rname, mods[j]->name) == 0) {
						m->regions[i]->mod = mods[j];
					}
				}
			}
		}
	}	
}

void regions_brushes(MAP *m)
{
	REGION *r = NULL;
	RMOD *mod = NULL;
	int i=0;

	for (i=0; i<m->num_regions; i++) {
		if (i < m->num_regions) {
			r = m->regions[i];
		}
		mod = (RMOD *)r->mod;
		if (mod && mod->brushes) mod->brushes(m, r);
	}
}

void regions_entities(MAP *m)
{
	REGION *r = NULL;
	RMOD *mod = NULL;
	int i=0;

	for (i=0; i<m->num_regions; i++) {
		if (i < m->num_regions) {
			r = m->regions[i];
		}
		mod = (RMOD *)r->mod;
		if (mod && mod->entities) mod->entities(m, r);
	}
}

void regions_terrain(MAP *m)
{
	REGION *r = NULL;
	RMOD *mod = NULL;
	int i=0;

	for (i=0; i<m->num_regions; i++) {
		if (i < m->num_regions) {
			r = m->regions[i];
		}
		mod = (RMOD *)r->mod;
		if (mod && mod->terrain) mod->terrain(m, r);
	}
}

void regions_cleanup(MAP *m)
{
	REGION *r = NULL;
	RMOD *mod = NULL;
	int i = 0;

	for (i=0; i<m->num_regions; i++) {
		if (i < m->num_regions) {
			r = m->regions[i];
		}
		mod = (RMOD *)r->mod;
		if (mod && mod->cleanup) mod->cleanup(r);
	}
}

int point_in_region(MAP *m, REGION *r, FloatPoint3 *p)
{
	float x_mult, y_mult;
	int ret = 0;
	int i1, i2, j1, j2;
	int found = 0;
	int k=0;

	x_mult = (float)m->x_size / (float)(m->terrain->x_res-1);
	y_mult = (float)m->y_size / (float)(m->terrain->y_res-1);

	// find the four surrounding grid points.  Or if we're exactly on a line or point, some of these will be equal
	i1 = m->x_offset + (int)(x_mult * floor(p->x / x_mult));
	i2 = m->x_offset + (int)(x_mult * ceil(p->x / x_mult));
	j1 = m->y_offset + (int)(y_mult * floor(p->y / y_mult));
	j2 = m->y_offset + (int)(y_mult * ceil(p->y / y_mult));

	// see how many of the 4 points are in the region
	for (k=0; k<r->num_points; k++) {	
		if (r->points[k].x == i1 && r->points[k].y == j1) found++;
		if (r->points[k].x == i1 && r->points[k].y == j2) found++;
		if (r->points[k].x == i2 && r->points[k].y == j1) found++;
		if (r->points[k].x == i2 && r->points[k].y == j2) found++;
	}

	// we'll say that it falls in the quad if 2 or more of the points are in the region

	return (found >= 3);
}
