
#include "rmods.h"
#include "codgen_random.h"
#include "taken_area.h"
#include "xml.h"

static char *tree_rand(DOMNODE *node_forest)
{
	DOMNODE *t = NULL;
	t = pick_child_random(node_forest);
	if (t) {
		return XMLDomGetAttribute(t, "name");
	} else {
		return "";
	}
}

static void rmod_forest_entities(MAP *m, REGION *r)
{
	DOMNODE *node_forest = NULL;	
	char *model = NULL;
	int i = 0;
	FloatPoint3 p;
	float sqinches_per_acre = 6272640.0;
	float trees_per_acre = 300.0;
	float height_percent_mean = 0.7;
	float height_percent_stdev = 0.04;
	int num_trees = 0;
	MixedPoint3 *mp;
	int x1=0;
	int y1=0;
	int x2=0;
	int y2=0;
	int width=0;
	int length=0;
	int angle=0;
	int allow_resize=1;

	fprintf(stderr, "Forest\n");

	node_forest = XMLDomGetChildNamed(XMLDomGetChildNamed(m->conf, "regions"), "forest");
	trees_per_acre = XMLDomGetAttributeFloat(node_forest, "trees_per_acre", trees_per_acre);
	allow_resize = XMLDomGetAttributeTF(node_forest, "allow_resize", allow_resize);
	height_percent_mean = XMLDomGetAttributeFloat(node_forest, "height_percent_mean", height_percent_mean);
	height_percent_stdev = XMLDomGetAttributeFloat(node_forest, "height_percent_stdev", height_percent_stdev);

	width = r->x_max - r->x_min;
	length = r->y_max - r->y_min;

	// we'll use the assumption that one unit is approximately an inch
	num_trees = (int) (trees_per_acre * (float)(width * length) / sqinches_per_acre);

	for (i=0; i<num_trees; i++) {
		float scale = 1.0;

		if (allow_resize) {			
            scale = random_normal(height_percent_mean, height_percent_stdev);
			if (scale < 0.01) scale = 0.01; // make sure sizes are non-zero
		}

		p.x = (float)(r->x_min + genrand(width));
		p.y = (float)(r->y_min + genrand(length));

		if (point_in_region(m, r, &p)) {
			p.z = terrain_height_at_point(m->terrain, p.x, p.y, m->x_size, m->y_size, m->height);
			model = tree_rand(node_forest);
			if (!TakenArea_IsIn(m->ta,p.x,p.y,p.z,20,300,NULL)) {
				TakenArea_AddCircle(m->ta,p.x,p.y,p.z,20,300,"Tree",0);
				angle = genrand(360);
				
				fprintf(m->map_file,
						"{\n"
						"\"model\" \"%s\"\n"
						"\"modelscale\" \"%.2f\"\n"
						"\"angles\" \"0 %d 0\"\n"
						"\"origin\" \"%.2f %.2f %.2f\"\n"
						"\"classname\" \"misc_model\"\n"
						"}\n", model, scale, angle, p.x, p.y, p.z);
			}
		}		
	}	
}

RMOD rmod_forest = { "forest", NULL, NULL, rmod_forest_entities, NULL };

