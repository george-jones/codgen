
#include "rmods.h"
#include "codgen_random.h"
#include "catenary.h"
#include "primitives.h"

typedef struct {
	CATENARY *eq;
	double x1;
	double x2;
	double z1;
	double z2;
	double y;
	double low_z;
} BRIDGE;

static void rmod_bridge_fill(MAP *m, REGION *r)
{
	BRIDGE *b = NULL;
	DOMNODE *node_bridges = NULL;
	DOMNODE *node_plank = NULL;
	DOMNODE *node_rope = NULL;
	DOMNODE *node_water = NULL;
	char *plank_texture = "caulk";
	char *rope_texture = "caulk";
	char *water_texture = "caulk";
	IntPoint3 a[4];
	double x=0.0;
	double x1=0.0;
	double x2=0.0;
	double z1=0.0;
	double z2=0.0;
	int num_planks = 0;
	double plank_sep = 12;
	double plank_width = 20;
	double step_size = 0.0;
	int plank_length = 50;
	int rope_offset = 4;

	b = (BRIDGE *)r->mod_data;
	if (!b) return;

	fprintf(stderr, "Bridge\n", b->x1, b->x2);

	node_bridges = XMLDomGetChildNamed(XMLDomGetChildNamed(m->conf, "regions"), "bridges");
	if (!node_bridges) return;

	node_rope = XMLDomGetChildNamed(node_bridges, "rope");
	if (node_rope) {
		rope_texture = XMLDomGetAttribute(node_rope, "texture");
		if (!rope_texture) rope_texture = "caulk";
	}

	node_water = XMLDomGetChildNamed(node_bridges, "water");
	if (node_water) {
		water_texture = XMLDomGetAttribute(node_water, "texture");
		if (!water_texture) water_texture = "caulk";
	}

	node_plank = XMLDomGetChildNamed(node_bridges, "plank");
	if (node_plank) {
		plank_length = XMLDomGetAttributeInt(node_plank, "length", plank_length);
		plank_width = XMLDomGetAttributeFloat(node_plank, "width", plank_width);
		plank_sep = XMLDomGetAttributeFloat(node_plank, "separation", plank_sep);
		plank_texture = XMLDomGetAttribute(node_plank, "texture");
		if (!plank_texture) plank_texture = "caulk";		
	}

	plank_sep += plank_width;

	num_planks = (int)((b->x2 - b->x1) / plank_sep);
	step_size = (b->x2 - b->x1) / num_planks;

	for (x = b->x1; x < b->x2; x += step_size) {
		x1 = x + (plank_sep - plank_width)/2;
		x2 = x1 + plank_width;
		z1 = catenary_eval(b->eq, x1);
		z2 = catenary_eval(b->eq, x2);

		// find lowest point of bridge.  this is helpful later
		if (x == b->x1 || z1 < b->low_z) {
			b->low_z = z1;
		}

		a[0].x = x1;
		a[0].y = b->y+plank_length;
		a[0].z = z1;
	
		a[1].x = x2;
		a[1].y = b->y+plank_length;
		a[1].z = z2;
	
		a[2].x = x2;
		a[2].y = b->y-plank_length;
		a[2].z = z2;
	
		a[3].x = x1;
		a[3].y = b->y-plank_length;
		a[3].z = z1;
	
		box(m->map_file, a, 4, plank_texture);

		// rope connectors
		x1 = x;
		x2 = x1 + step_size;
		z1 = catenary_eval(b->eq, x1);
		z2 = catenary_eval(b->eq, x2);

		a[0].x = x1;
		a[0].y = b->y+plank_length-4;
		a[0].z = z1 - rope_offset;
	
		a[1].x = x2;
		a[1].y = b->y+plank_length-4;
		a[1].z = z2 - rope_offset;
	
		a[2].x = x2;
		a[2].y = b->y+plank_length-8;
		a[2].z = z2 - rope_offset;
	
		a[3].x = x1;
		a[3].y = b->y+plank_length-8;
		a[3].z = z1 - rope_offset;
	
		box(m->map_file, a, 4, rope_texture);

		a[0].x = x1;
		a[0].y = b->y-plank_length+8;
		a[0].z = z1 - rope_offset;
	
		a[1].x = x2;
		a[1].y = b->y-plank_length+8;
		a[1].z = z2 - rope_offset;
	
		a[2].x = x2;
		a[2].y = b->y-plank_length+4;
		a[2].z = z2 - rope_offset;
	
		a[3].x = x1;
		a[3].y = b->y-plank_length+4;
		a[3].z = z1 - rope_offset;
	
		box(m->map_file, a, 4, rope_texture);
	}

	if (node_water) {	
		// water level halfway between bottom of bridge and bottom of valley
		z1 = ((-1.0 * (float)m->height) + b->low_z) / 2;
		a[0].z = a[1].z = a[2].z = a[3].z = z1;
	
		a[0].x = r->x_min;
		a[0].y = r->y_max;
	
		a[1].x = r->x_max;
		a[1].y = r->y_max;
	
		a[2].x = r->x_max;
		a[2].y = r->y_min;
	
		a[3].x = r->x_min;
		a[3].y = r->y_min;
	
		box(m->map_file, a, 4, water_texture);
	}

}


static void rmod_bridge_brushes(MAP *m, REGION *r)
{
	MixedPoint3 ***g = NULL;
	float x_size;
	float y_size;
	int grid_x;
	int grid_y;	

	x_size = m->x_size / (m->terrain->x_res - 1);
	y_size = m->y_size / (m->terrain->y_res - 1);

	g = region_get_grid(r, x_size, y_size, &grid_x, &grid_y);
	if (g) {
		int min_size = 0;
		int min_j = 0;
		int s=0;
		int i=0;
		int j=0;
		int i1=0;
		int i2=0;
		int k=0;
		int continuous=1;

		// put bridge in the middle of the gap.
		j = grid_y / 2;

		// find left point
		for (i=0; i<grid_x; i++) {
			if (g[i][j]) {
				i1 = i;
				break;
			}
		}
		i1++;

		// find right point
		for (i=grid_x-1; i>=0; i--) {
			if (g[i][j]) {
				i2 = i;
				break;
			}	
		}
		i2--;

		for (i=i1+1; i<=i2-1; i++) {
			if (g[i][j] == NULL || region_boundary_point(r, g[i][j])) {
				continuous = 0;
				break;
			}
		}

		// bail out early if bridgable gap is too small (<=1), or is not continuous.
		if (i2 - i1 > 2 && continuous) {
			BRIDGE *b = NULL;
			CATENARY *eq = NULL;
			double x1, z1, x2, z2, slope;
			int rx_off = 0;
			int ry_off = 0;
			int r_off_found = 0;

			b = (BRIDGE *)malloc(sizeof(BRIDGE));
			if (!b) return;

			eq = (CATENARY *)malloc(sizeof(CATENARY));
			if (!eq) return;

			// flatten terrain in this region, but only non-boundary points so that we don't screw up
			// stuff on the edges of other regions, or the edge of the map.
			for (k=0; k<r->num_points; k++) {
				if (!region_boundary_point(r, &r->points[k])) {
					int ti = r->points[k].x / x_size;
					int tj = r->points[k].y / y_size;

					if (!r_off_found) {
						int ri = 0;
						int rj = 0;
						for (ri=0; ri<grid_x && !r_off_found; ri++) {
							for (rj=0; rj<grid_y && !r_off_found; rj++) {
								if (&r->points[k] == g[ri][rj]) {
									rx_off = ti - ri;
									ry_off = tj - rj;
									r_off_found = 1;
								}
							}
						}
					}

					if (abs(j + ry_off - tj) > 1 || (ti > i1 + rx_off && ti < i2 + rx_off)) {
						if (ti > 1 && ti < m->terrain->x_res - 2) {						
							terrain_set_point(m->terrain, ti, tj, -1.0);					
						}
					}
				}
			}

			x1 = g[i1][j]->x;
			z1 = g[i1][j]->z;
			x2 = g[i2][j]->x;
			z2 = g[i2][j]->z;
			slope = 0.3;

			catenary_find_equation(eq, x1, z1, x2, z2, slope);

			b->eq = eq;
			b->x1 = x1;
			b->x2 = x2;
			b->z1 = z1;
			b->z2 = z2;
			b->y = x1 = g[i1][j]->y;

			r->mod_data = b;
		}

		region_free_grid(g, grid_x);
	}

	rmod_bridge_fill(m, r);
}

static void rmod_bridge_entities(MAP *m, REGION *r)
{
	BRIDGE *b = NULL;
	DOMNODE *node_bridges = NULL;
	IntPoint3 a[4];
	float z_bottom, z1;	

	node_bridges = XMLDomGetChildNamed(XMLDomGetChildNamed(m->conf, "regions"), "bridges");
	if (!node_bridges) return;

	b = (BRIDGE *)r->mod_data;
	if (!b) return;

	if (XMLDomGetChildNamed(node_bridges, "minefield")) {
		// underwater minefield - a trigger entity with an enclosed brush
		fprintf(m->map_file, "{\n"
							 "\"targetname\" \"minefield\"\n"
							 "\"classname\" \"trigger_multiple\"\n");
	
		// just below halfway between bottom of bridge and bottom of valley
		z_bottom = -1.0 * m->height;
		z1 = ((-1.0 * (float)m->height) + b->low_z) / 2;
		a[0].z = a[1].z = a[2].z = a[3].z = z_bottom;
	
		a[0].x = r->x_min;
		a[0].y = r->y_max;
	
		a[1].x = r->x_max;
		a[1].y = r->y_max;
	
		a[2].x = r->x_max;
		a[2].y = r->y_min;
	
		a[3].x = r->x_min;
		a[3].y = r->y_min;
	
		box(m->map_file, a, z1 - z_bottom, "trigger");
	
		fprintf(m->map_file, "}\n");
	}	
}

static void rmod_bridge_cleanup(REGION *r)
{
	BRIDGE *b = NULL;
	b = (BRIDGE *)r->mod_data;
	free(b);
}

RMOD rmod_bridge = { "bridges", rmod_bridge_brushes, NULL, rmod_bridge_entities, rmod_bridge_cleanup };

