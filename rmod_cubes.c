
#include "rmods.h"
#include "codgen_random.h"
#include "taken_area.h"

// * flattish areas are good for cube buildings
static RMOD_QUALITY rmod_cubes_eval(MAP *m, REGION *r)
{
	RMOD_QUALITY ret;
	
	if (r->z_stdev >= (float)m->height / 6) {
		ret = RMOD_NO_WAY;
	} else if (r->z_stdev >= (float)m->height / 22) {
		ret = RMOD_BAD;
	} else if (r->z_stdev >= (float)m->height / 35) {
		ret = RMOD_OK;
	} else {
		ret = RMOD_GOOD;
	}

	return ret;
}

static void rmod_cubes_fill(MAP *m, REGION *r)
{
	char *model = NULL;
	int i = 0;
	FloatPoint3 p;
	float sqinches_per_acre = 6272640.0;
	float cubes_per_acre = 20.0;
	int num_cubes = 0;
	MixedPoint3 *mp;
	int x1=0;
	int y1=0;
	int x2=0;
	int y2=0;
	int width=0;
	int length=0;
	int angle=0;
	int oor=0;	// out of region flag

	fprintf(stderr, "Cubes\n");

	width = r->x_max - r->x_min;
	length = r->y_max - r->y_min;

	// we'll use the assumption that one unit is approximately an inch
	num_cubes = (int) (cubes_per_acre * (float)(width * length) / sqinches_per_acre);

	for (i=0; i<num_cubes; i++) {
		do {
			oor = 1;
			p.x = (float)(r->x_min + genrand(width));
			p.y = (float)(r->y_min + genrand(length));
	
			if (point_in_region(m, r, &p)) {
				p.z = terrain_height_at_point(m->terrain, p.x, p.y, m->x_size, m->y_size, m->height);
				oor = 0;
			}
		} while (oor || TakenArea_IsIn(m->ta,p.x,p.y,p.z,95,166,NULL));
		TakenArea_AddBox(m->ta,p.x,p.y,p.z,95,95,166,"Building Base",1);
		angle = genrand(360);

		fprintf(m->map_file,
			"{\n"
			"\"model\" \"prefabs/codgen/building_base.map\"\n"
			"\"angles\" \"0 %d 0\"\n"
			"\"origin\" \"%.2f %.2f %.2f\"\n"
			"\"classname\" \"misc_prefab\"\n"
			"}\n", angle, p.x, p.y, p.z);
	}	
}

RMOD rmod_cubes = { "cubes", rmod_cubes_eval, NULL, rmod_cubes_fill };

