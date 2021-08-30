#include "rmods.h"
#include "codgen_random.h"
#include "taken_area.h"
#include "xml.h"
#include "codgen.h"

// mod that makes tombstones

static char *grave_rand(DOMNODE *node_parent)
{
	DOMNODE *t = NULL;
	t = pick_child_random(node_parent);
	if (t) {
		return XMLDomGetAttribute(t, "name");
	} else {
		return "";
	}
}

static void graveyard_entities(MAP *m, REGION *r)
{
	DOMNODE *node_graveyard = NULL;
	char *model = NULL;
	int i = 0;
	FloatPoint3 p;
	float sqinches_per_acre = 6272640.0;
	float graves_per_acre = 0.0;
	int num_graves = 0;
	MixedPoint3 *mp;
	int x1=0;
	int y1=0;
	int x2=0;
	int y2=0;
	int width=0;
	int length=0;
	int angle=0;
	int a = 0;

	// typical facing for tomstones in this graveyard
	a = genrand(360);

	fprintf(stderr, "Graveyard\n");

	node_graveyard = XMLDomGetChildNamed(XMLDomGetChildNamed(m->conf, "regions"), "graveyard");
	graves_per_acre = XMLDomGetAttributeFloat(node_graveyard, "graves_per_acre", 200.0);

	width = r->x_max - r->x_min;
	length = r->y_max - r->y_min;

	// we'll use the assumption that one unit is approximately an inch
	num_graves = (int) (graves_per_acre * (float)(width * length) / sqinches_per_acre);

	for (i=0; i<num_graves; i++) {
		float scale = 1.0;		

		p.x = (float)(r->x_min + genrand(width));
		p.y = (float)(r->y_min + genrand(length));

		if (point_in_region(m, r, &p)) {
			p.z = terrain_height_at_point(m->terrain, p.x, p.y, m->x_size, m->y_size, m->height);
			model = grave_rand(node_graveyard);
			if (!TakenArea_IsIn(m->ta,p.x,p.y,p.z,40,100,NULL)) {
				TakenArea_AddCircle(m->ta,p.x,p.y,p.z,40,100,"Grave",0);
				angle = a + randrange(-8, 8);

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

RMOD rmod_graveyard = { "graveyard", NULL, NULL, graveyard_entities, NULL };

