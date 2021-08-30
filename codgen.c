#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>

#include "gd.h"

#include "codgen.h"
#include "primitives.h"
#include "mazemaker.h"
#include "terrain.h"
#include "codgen_random.h"
#include "region.h"
#include "region_maker.h"
#include "rmods.h"
#include "map.h"
#include "taken_area.h"
#include "mapfile_parser.h"
#include "xml.h"
#include "output_terrain.h"

#ifdef WIN32
#define snprintf _snprintf
#define strcasecmp strcmpi
#endif

#define WALL_OUTER_HEIGHT 300
#define TERRAIN_HEIGHT 250
#define DIST_FROM_OUTER_WALL 40
#define SKY_OFFSET 300
#define SPAWN_RADIUS 15
#define HQ_RADIUS 15

#define DEFAULT_CONF_XML "default.xml"

typedef enum {
	SPAWN_CAMERA,
	SPAWN_DM,
	SPAWN_TDM,
	SPAWN_CTF_ALLIED,
	SPAWN_CTF_AXIS
} SPAWN_TYPE;

static char *spawn_string(SPAWN_TYPE s)
{
	switch (s) {
		case SPAWN_CAMERA:
			return "mp_global_intermission";
		case SPAWN_DM:
			return "mp_dm_spawn";
		case SPAWN_CTF_ALLIED:
			return "mp_ctf_spawn_allied";
		case SPAWN_CTF_AXIS:
			return "mp_ctf_spawn_axis";
		case SPAWN_TDM:
			return "mp_tdm_spawn";
	}
	return "ILLEGAL SPAWN TYPE";
}

// distance squared
#define DIST2(x1, y1, x2, y2) (pow(x1 - x2, 2.0) + pow(y1 - y2, 2.0))
// distance true
#define DIST(x1,y1,x2,y2) sqrt(DIST2(x1,y1,x2,y2))

// one of the dirtier macros I've written.  The value of the last assignment is what gets returned from this expression.
// this lets us call the pseudo function pt(x,y,z) when we want to take three ints and make an IntPoint3 out of them.
static IntPoint3 tmppt;
static IntPoint3 *tmpptp = &tmppt;
#define pt(xval,yval,zval) (tmppt.x = (xval), tmppt.y = (yval), tmppt.z = (zval), tmpptp = &tmppt)

float map_height_at_point(MAP *m, float x, float y, int *on_a_rock)
{
	REGION *r = NULL;
	FloatPoint3 p;	
	float height = 0.0;
	float r_x = 0.0;
	float r_y = 0.0;
	int i = 0;

	// default is NOT on a rock
	if (on_a_rock) *on_a_rock = 0;	

	p.x = x;
	p.y = y;
	p.z = 0;

	height = terrain_height_at_point(m->terrain, x, y, m->x_size, m->y_size, m->height);

	if (on_a_rock) {
		// see it we're in a rocky region	
		for (i=0; i<m->num_regions; i++) {
			// is it rocky?
			r = m->regions[i];
			if (r->mod == &rmod_rocks || r->mod == &rmod_mound) {		
				if (point_in_region(m,r,&p)) {
					MAP *region_map = NULL;
					float region_height = 0.0;
	
					region_map = (MAP *)r->mod_data;
					region_height = terrain_height_at_point(region_map->terrain, x - region_map->x_offset,
															y - region_map->y_offset, region_map->x_size,
															region_map->y_size, region_map->height);
					if (region_height > height) {
						height = region_height;						
						*on_a_rock = 1;
					}
					break;
				}
			}
		}
	}	

	return height;
}

static int entity_placeable(MAP *m, int x, int y, int z, int width, int height, float *allowed_z)
{
	REGION *r = NULL;
	FloatPoint3 p;
	int i=0;

	if (TakenArea_IsIn(m->ta,x,y,z,width,height,allowed_z) && (allowed_z == NULL || *allowed_z == 0.0)) return 0;

	p.x = (float)x;
	p.y = (float)y;
	p.z = (float)z;

	// don't put anything in bridge regions.  they are troublesome.
	for (i=0; i<m->num_regions; i++) {
		r = m->regions[i];
		if (r && (RMOD *)r->mod == &rmod_bridge && point_in_region(m, r, &p)) return 0;
	}

	return 1;
}

#define rand_txt(a) random_texture(a, sizeof(a)/sizeof(char *))
static char *random_texture(char **a, size_t siz)
{
	int idx = 0;
	idx = randrange(0, siz);
	return a[idx];
}

static char *rand_txt_node(DOMNODE *p)
{
	DOMNODE *n;
	char *ret = "caulk";

	n = pick_child_random(p);
	if (n) {
		ret = XMLDomGetAttribute(n, "name");
	}

	if (!ret) ret = "caulk";	

	return ret;
}

static char *HEADERS[] =  {
	"\"diffusefraction\" \"0.2\"\n"
	"\"sundiffusecolor\" \".75 .84 1\"\n"
	"\"sundirection\" \"-45 330 0\"\n"
	"\"suncolor\" \"1 .73 .42\"\n"
	"\"sunlight\" \"2\"\n"
	"\"ambient\" \"0.50\"\n"
	"\"_color\" \".75 .84 1\"\n"
	"\"northyaw\" \"90\"\n"
	"\"classname\" \"worldspawn\"\n"
	,
    "\"minlightcolor\" \".75 .84 1\"\n"
	"\"diffusefraction\" \"0.45\"\n"
	"\"sundiffusecolor\" \".75 .84 1\"\n"
	"\"sundirection\" \"-45 210 0\"\n"
	"\"suncolor\" \"1 .73 .42\"\n"
	"\"sunlight\" \"1.5\"\n"
	"\"_color\" \".75 .84 1\"\n"
	"\"ambient\" \".2\"\n"
	,
	"\"diffusefraction\" \"0.6\"\n"
	"\"sundiffusecolor\" \".75 .84 1\"\n"
	"\"sundirection\" \"-45 330 0\"\n"
	"\"suncolor\" \"1 .73 .42\"\n"
	"\"sunlight\" \"1.3\"\n"
	"\"ambient\" \"0.18\"\n"
	"\"_color\" \".75 .84 1\"\n"
	"\"northyaw\" \"90\"\n"
	,
	"\"northyaw\" \"270\"\n"
	"\"diffusefraction\" \".4\"\n"
	"\"sunlight\" \".7\"\n"
	"\"sundiffusecolor\" \"14 21 32\"\n"
	"\"suncolor\" \"33 43 53\"\n"
	"\"sundirection\" \"-40 5 0\"\n"
	"\"_color\" \"1.000000 0.980392 0.925490\"\n"
	"\"ambient\" \".12\"\n"
	,
	"\"_color\" \"200 200 205\"\n"
	"\"ambient\" \"0.16\"\n"
	"\"sunlight\" \"1\"\n"
	"\"suncolor\" \"100 100 210\"\n"
	"\"sundirection\" \"-45 95 0\"\n"
	"\"sundiffusecolor\" \"200 200 210\"\n"
	"\"diffusefraction\" \"0.6\"\n"
	,
	"\"_color\" \"0.95 0.95 1.000000\"\n"
	"\"sundirection\" \"-35 195 0\"\n"
	"\"suncolor\" \"0.99 0.98 0.86\"\n"
	"\"sunlight\" \"1.6\"\n"
	"\"ambient\" \".20\"\n"
	"\"sundiffusecolor\" \"0.94 0.94 1.000000\"\n"
	"\"diffusefraction\" \".55\"\n"
	"\"northyaw\" \"90\"\n"
	,
	"\"sundirection\" \"-40 225 0\"\n"
	"\"suncolor\" \"100 100 105\"\n"
	"\"sunlight\" \"1.3\"\n"
	"\"sundiffusecolor\" \"100 100 100\"\n"
	"\"diffusefraction\" \".65\"\n"
	"\"ambient\" \".10\"\n"
	"\"_color\" \"100 100 105\"\n"
	,
	"\"_color\" \"1 1 1\"\n"
	"\"ambient\" \".18\"\n"
	"\"diffusefraction\" \".65\"\n"
	"\"suncolor\" \"0.953642 0.980132 1.000000\"\n"
	"\"sunfiffusecolor\" \"0.962963 0.984127 1.000000\"\n"
	"\"sundirection\" \"-65 75 0\"\n"
	"\"sunlight\" \"1\"\n"
};

static void map_header(FILE *f)
{
	int hdr_idx = genrand(sizeof(HEADERS)/sizeof(char *));
	char *header = HEADERS[hdr_idx];

	fprintf(f, "iwmap 4\n"
			"// entity 0\n"
			"{\n"
			"%s"
			"\"classname\" \"worldspawn\"\n"
			, header);
}

static void map_spawn_point(FILE *f, IntPoint3 *p, SPAWN_TYPE s, float xz_angle, float xy_angle, float yz_angle)
{
	fprintf(f, "{\n"
			"\"classname\" \"%s\"\n"
			"\"angles\" \"%.2f %.2f %.2f\"\n"
			"\"origin\" \"%d %d %d\"\n"
			"}\n", spawn_string(s), xz_angle, xy_angle, yz_angle, p->x, p->y, p->z);
}

static void map_ctf_flag(FILE *f, IntPoint3 *p, int allied_0_axis_1)
{
	fprintf(f, "{\n"
			"\"angles\" \"0 0 0\"\n"
			"\"origin\" \"%d %d %d\"\n"
			"\"model\" \"prefabs/mp/%s\"\n"
			"\"classname\" \"misc_prefab\"\n"
			"}\n", p->x, p->y, p->z,
			(allied_0_axis_1 == 0)?"ctf_flag_allies.map":"ctf_flag_axis.map");
}

static void	make_traversible(MAP *m)
{
	TERRAIN *t = m->terrain;
	float x_slope = 0.0;
	float y_slope = 0.0;
	float x_unit = 0.0;
	float y_unit = 0.0;
	float s = 0.0;
	float max_s = 1.0; // I think.

	// actual size of grid's unit rectangle
	x_unit = (float) m->x_size / (float) t->x_res;
	y_unit = (float) m->y_size / (float) t->y_res;

	// highest actual slopes found
	x_slope = (float) m->height * terrain_find_max_x_slope(t) / x_unit;
	y_slope = (float) m->height * terrain_find_max_y_slope(t) / y_unit;

	// scale down the height
	s = (x_slope > y_slope) ? x_slope : y_slope;
	m->height = (int)((float)m->height * max_s / s);
}

typedef struct {
	char *desc;
	int width;
	int height;
	int max_repeat;
} MAP_FLAVOR;

static MAP_FLAVOR *RandomFlavor(DOMNODE *node_models, MAP_FLAVOR *mf)
{
	DOMNODE *model = NULL;

	model = pick_child_random(node_models);
	if (model) {
		mf->desc = XMLDomGetAttribute(model, "name");
		mf->height = XMLDomGetAttributeInt(model, "height", 10);
		mf->max_repeat = XMLDomGetAttributeInt(model, "repeat", 0);
		mf->width = XMLDomGetAttributeInt(model, "radius", 30);
		return mf;
	}

	return NULL;
}

static void map_add_grass_everywhere(MAP *m)
{
	FILE *f = m->map_file;
	char *desc;
	int width;
	int x=0,y=0,z=0;
	int add=0;
	int on_a_rock=0;

	fprintf(stderr,"Adding grass everywhere\n");
	desc =  "brush_shortnewgrass_squareclumpshort";
	width = 44;
	add = width/2;

	for (x = 0; x < m->x_size/8; x+=add) {
		for (y = 0; y < m->y_size/8; y+=add) {
			z = map_height_at_point(m,x,y,&on_a_rock);
			fprintf(f,"{\n");
			fprintf(f,"\"origin\" \"%d %d %d\"\n",x,y,z);
			fprintf(f,"\"model\" \"xmodel/%s\"\n",desc);
			fprintf(f,"\"modelscale\" \"1\"\n");
			fprintf(f,"\"classname\" \"script_model\"\n");
			fprintf(f,"\"angles\" \"0 %d 0\"\n",genrand(360));
			fprintf(f,"}\n");
		}
	}
}

static void map_add_flavor(MAP *m)
{
	FILE *f = m->map_file;
	DOMNODE *node_models = NULL;
	int i = 0;
	int n = 0;
	int x,y,z;
	MAP_FLAVOR mf;
	MAP_FLAVOR *mfp = NULL;
	int on_a_rock=0;

	node_models = XMLDomGetChildNamed(m->conf, "models");
	if (!node_models) {
		return;
	}

	n = XMLDomGetAttributeInt(node_models, "limit", 500);

	fprintf(stderr,"Adding %d 'Flavor' models.\n",n);
	for (i=0;i<n;i++) {
		x = randrange(0,m->x_size);
		y = randrange(0,m->y_size);
		z = map_height_at_point(m,x,y,&on_a_rock);
		mfp = RandomFlavor(node_models, &mf);
		// don't allow this to overlay something important
		
		if (mfp) {
			int j,n_x,n_y,n_z,rep=1;
			if (mfp->max_repeat > 0)  rep = randrange(0,mfp->max_repeat)+1;

			for (j=0;j<rep;j++) {
				if (j == 0) {
					n_x = x;
					n_y = y;
					n_z = z;
				} else {
					int a = 1;
					if (rep > 3) {
						a = randrange(1,rep+1);
					}
					n_x = randrange(x-a*(mfp->width/2),x+a*(mfp->width/2));
					n_y = randrange(y-a*(mfp->width/2),y+a*(mfp->width/2));
					n_z = map_height_at_point(m,n_x,n_y,&on_a_rock);
				}
				if (!on_a_rock && n_x < m->x_size && n_y < m->y_size && n_x > 0 && n_y > 0
					&& entity_placeable(m,n_x,n_y,n_z,mfp->width,mfp->height,NULL)) {
					fprintf(f,"{\n");
					fprintf(f,"\"origin\" \"%d %d %d\"\n",n_x,n_y,n_z);
					fprintf(f,"\"model\" \"xmodel/%s\"\n",mfp->desc);
					fprintf(f,"\"classname\" \"misc_model\"\n");
					fprintf(f,"\"angles\" \"0 %d 0\"\n",randrange(0,360));
					fprintf(f,"}\n");
				}
			}
		}
	}
}

int generate_arena(char *map_name, DOMNODE *conf)
{
	int buf_size = strlen(map_name)+7;
	char *buf = malloc(buf_size);
	FILE *f = NULL;
	
	if (!buf) return 0;
	snprintf(buf, buf_size, "%s.arena", map_name);
	f = fopen(buf,"w");
	if (f) {
		DOMNODE *game = XMLDomGetChildNamed(conf, "game");
		DOMNODE *n = NULL;
		int i = 0;
		int first = 1;

		fprintf(f,"{\n"
				  "	map \"%s\"\n"
				  "	longname \"%s\"\n", map_name, map_name);
		fprintf(f,"	gametype \"");

		for (i=0; (n = game->children[i]) != NULL; i++) {
			if (strcasecmp(n->tagName, "gametype") == 0) {
				if (!first) fprintf(f, " ");
				first = 0;
				fprintf(f, "%s", XMLDomGetAttribute(n, "name"));
			}
		}

		fprintf(f, "\"\n"
				   "}\n");

		fclose(f);
	}
	free(buf);
	return 1;
}

static void gsc_getTeam(DOMNODE *node_team, char **team, char **model)
{
	if (node_team) {
		DOMNODE *node_nationality = pick_child_random(node_team);
		if (node_nationality) {
			// only allow xml to set model if also setting team. try to keep invalid team/model combos from occuring
			DOMNODE *node_model = pick_child_random(node_nationality);
			if (node_model) {
				char *tmpteam=NULL,*tmpmodel=NULL;
				tmpteam = XMLDomGetAttribute(node_nationality, "name");
				tmpmodel = XMLDomGetAttribute(node_model, "name");

				if (tmpteam && tmpmodel) {
					*team = tmpteam;
					*model = tmpmodel;
				}
			}
		}
	}
}

int generate_gsc(char *map_name, MAP *m)
{
	char buf[1024];
	FILE *fp = NULL;
	char *allied_team = "british";
	char *allied_model = "normandy";
	char *axis_team = "german";
	char *axis_model = "normandy";
	char *ambient_sound=NULL;
	DOMNODE *game = XMLDomGetChildNamed(m->conf, "game");
	DOMNODE *node_teams = XMLDomGetChildNamed(game, "teams");
	DOMNODE *node_ambient = XMLDomGetChildNamed(m->conf, "ambient");
	if (node_teams) {
		//
		// Get allied team/model
		//
		gsc_getTeam(XMLDomGetChildNamed(node_teams,"allies"),&allied_team,&allied_model);
		gsc_getTeam(XMLDomGetChildNamed(node_teams,"axies"),&axis_team,&axis_model);
	}
	if (node_ambient) {
		ambient_sound = XMLDomGetAttribute(pick_child_random(node_ambient),"name");
	}
	if (!ambient_sound) {
		ambient_sound = "france";
	}
	snprintf(buf,sizeof(buf),"%s.gsc",map_name);
	fp = fopen(buf,"w");	

	if (fp) {
		int x,y,z,i,num_hq,j;
		int try_max = 1000;
		int on_a_rock = 0;

		fprintf(fp,"main()\n"
						"{\n"
						"	\n"
						"	maps\\mp\\mp_breakout_fx::main();\n"
						"	maps\\mp\\_load::main();\n"
						"\n"
						"	setExpFog(0.00015, 0.15, 0.14, 0.13, 0);\n"
						"	ambientPlay(\"ambient_%s\");\n",ambient_sound);
		fprintf(fp, "\n"
						"	game[\"allies\"] = \"%s\";\n",allied_team);
		fprintf(fp, "	game[\"axis\"] = \"%s\";\n",axis_team);
		fprintf(fp,	"	game[\"attackers\"] = \"allies\";\n"
						"	game[\"defenders\"] = \"axis\";\n"
						"	game[\"%s_soldiertype\"] = \"%s\";\n",allied_team,allied_model);
		fprintf(fp,	"	game[\"%s_soldiertype\"] = \"%s\";\n",axis_team,axis_model);
		fprintf(fp, "\n"
						"	setcvar(\"r_glowbloomintensity0\",\".25\");\n"
						"	setcvar(\"r_glowbloomintensity1\",\".25\");\n"
						"	setcvar(\"r_glowskybleedintensity0\",\".3\");\n"
						"\n"
						"\n"
						"	if((getcvar(\"g_gametype\") == \"hq\"))\n"
						"	{\n"
						"		level.radio = [];\n");
		num_hq = randrange(1,m->x_size*m->y_size/400000);
		for (i=0;i<num_hq;i++) {
			float allowed_z = 0.0;
			j=0;
			do {
				j++;
				x = randrange(0,m->x_size);
				y = randrange(0,m->y_size);
				//z = (int)map_height_at_point(m, x, y, &on_a_rock);
				z = 40;
				if (DBG) fprintf(stderr,"try %d for hq %d: %d,%d,%d\n",j,i,x,y,z);
			//} while (!entity_placeable(m,x,y,z,HQ_RADIUS,100,&allowed_z) && j<try_max);
			} while (j<try_max);
			if (j < try_max) {
				//if (allowed_z != 0.) z = (int)ceil(allowed_z);
				//TakenArea_AddCircle(m->ta,x,y,z,HQ_RADIUS,100,"HQ",0);
				if (DBG) fprintf(stderr,"Adding HQ\n");
				fprintf(fp, "		level.radio[%d] = spawn(\"script_model\", (%d, %d, %d));\n"
								"		level.radio[%d].angles = (0, 0, 0);\n"
								"		\n",i,x,y,z,i);
			}
		}
		fprintf(fp, "	}\n"
						"}\n"
						"\n");

		fclose(fp);
	}
	return 1;
}

int generate_iwdmakescript(char *map_name)
{
	char buf[1024];
	FILE *fp = NULL;
	snprintf(buf,sizeof(buf),"%s2iwd.bat",map_name);
	fp = fopen(buf,"w");
	if (fp) {
		fprintf(fp,"cd ..\r\n"
					"del main\\%s.iwd\r\n",map_name);
		fprintf(fp,"copy map_source\\%s.gsc maps\\mp\\\r\n",map_name);
		fprintf(fp,"copy map_source\\%s.csv maps\\mp\\\r\n",map_name);
		fprintf(fp,"copy map_source\\%s.arena mp\\\r\n",map_name);
		fprintf(fp,"copy main\\maps\\mp\\%s.d3dbsp maps\\mp\\\r\n",map_name);
		fprintf(fp,"c:\\progra~1\\winzip\\wzzip -yp -P main\\%s.iwd maps\\mp\\%s.gsc maps\\mp\\%s.csv mp\\%s.arena maps\\mp\\%s.d3dbsp\r\n",
				  map_name,map_name,map_name,map_name,map_name);
		fprintf(fp,"@echo off\r\n"
					"If Not ErrorLevel 1 Goto Exit\r\n"
					"@echo on\r\n"
					"Echo ***SERIOUS ERROR DETECTED***\r\n"
					":Exit\r\n");
		fclose(fp);
	}
	return 1;
}

static void	regions_begin(MAP *m, int prune)
{
	DOMNODE *node_regions = NULL;
	int num = 10;

	node_regions = XMLDomGetChildNamed(m->conf, "regions");
	if (node_regions) {
		num = XMLDomGetAttributeInt(node_regions, "num", num);
	}

    m->regions = get_regions(m, num, prune);
	m->num_regions = num;
	if (m->regions) regions_eval(m);
	regions_brushes(m);
}

static void regions_fill(MAP *m)
{	
	regions_entities(m);	
}

static void kill_regions(MAP *m) {
	regions_cleanup(m);
	destroy_regions(m->regions);
}

struct {
	int r;
	int b;
	int g;
} predefined_colors[] = {
	{ 255, 0, 0 },
	{ 0, 255, 0 },
	{ 0, 0, 255 },
	{ 255, 255, 0 },
	{ 0, 255, 255 },
	{ 0, 0, 0 }
};

static int mult=10;
static void terrain_to_png(char *filename, TERRAIN *t, int colorized)
{
	FILE *imagefile = NULL;
	gdImagePtr im;
	int width = t->x_res * mult;
	int height = t->y_res * mult;
	int x=0;
	int y=0;
	int c = 0;
	int *colors = NULL;

	imagefile = fopen(filename, "wb");
	if (!imagefile) return;	

	colors = (int *)calloc(256, sizeof(int));
	if (!colors) {
		fprintf(stderr, "Unable to allocate colors.");
		return;
	}

	im = gdImageCreate(width, height);
	if (im) {
		for (y=0; y<t->y_res; y++) {
			for (x=0; x<t->x_res; x++) {
				float col = terrain_get_point(t, x, y);

				if (!colorized) {   		
					c = (int)(256.0 * terrain_get_point(t, x, y));
					if (c < 0) c = 0;
					if (!colors[c]) {
						colors[c] = gdImageColorAllocate(im, c, c, c);
					}
				} else {
					if (col == 0.0) {
						c = 0;
					} else if (col == 1.0) {
						c = 1;
					} else if (col == 2.0) {
						c = 2;
					} else if (col == 3.0) {
						c = 3;
					} else if (col == 4.0) {
						c = 4;
					} else {
						c = 5;
					}				

					if (!colors[c]) {
						colors[c] = gdImageColorAllocate(im, predefined_colors[c].r, predefined_colors[c].g, predefined_colors[c].b);
					}
				}

				c = colors[c];
				
				gdImageFilledRectangle(im, x*mult, (t->y_res-y-1)*mult, (x+1)*mult, ((t->y_res - y))*mult, c);
			}
		}
		
		gdImagePng(im, imagefile);
		gdImageDestroy(im);
	}
	
	free(colors);

	fclose(imagefile);
	
	return;
}

static void map_spawn_points_random(MAP *m)
{
	TERRAIN *t = m->terrain;
	FILE *f = m->map_file;
	int x,y,z,x_size,y_size;
	int a,j=0,max_points,try_max;
	float angle;
	float spawn_z = 0.0;
	int on_a_rock = 0;

	try_max = 10000000;

	max_points = randrange(20,40);  // generate a random number of spawn points max of 100 spawn points
	x_size = (m->x_size - 100);  // subtract 100 from sizes to 
	y_size = (m->y_size - 100);  // ensure the point is not in a wall

	for (a = 0; a < max_points && j<try_max; a++) {
		//do {
			spawn_z = 0.0;
			j++;
			x = genrand(x_size);
			if (x <= 100) x+= 100; // make sure we're not spawning in a wall
			y = genrand(y_size);
			if (y <= 100) y+= 100; // make sure we're not spawning in a wall
			//z = (int)map_height_at_point(m, x, y, &on_a_rock);

			z = 35;

		//} while (!entity_placeable(m,x,y,z,SPAWN_RADIUS,100,&spawn_z) && j<try_max);
        
		if (j < try_max) {
			//if (spawn_z != 0.0) z = (int)ceil(spawn_z);
			//TakenArea_AddCircle(m->ta,x,y,z,SPAWN_RADIUS,100,"Spawn",0);
			angle = (float)genrand(360);   // generate an angle between 0 and 360
			map_spawn_point(f, pt(x, y, z), SPAWN_DM, 0.0, angle, 0.0);
			map_spawn_point(f, tmpptp, SPAWN_TDM, 0.0, angle, 0.0);
		}
	}

}

static void map_spawn_points_random_ctf(MAP *m)
{
	TERRAIN *t = m->terrain;
	FILE *f = m->map_file;
	int x,y,z,Max_x,Max_y;
	int i,j=0,max_spawn,try_max;
	float angle = 0.0;
	float spawn_z = 0.0;
	int on_a_rock = 0;

	try_max = 10000000;

	max_spawn = randrange(20,40);  // generate a random number of spawn points max of 100 spawn points

	Max_x = (m->x_size/2) - 100;  // subtract 100 from sizes to 
	Max_y = m->y_size - 100;      // ensure the point is no in a wall

	for (i = 0; i < max_spawn && j<try_max; i++) {

		//do {
			spawn_z = 0.0;
			j++;
			x = genrand(Max_x);
			if (x <= 100) x+= 100; // make sure we're not spawning in a wall
			if (!(i%2)) x+=Max_x;
			y = genrand(Max_y);
			if (y <= 100) y+= 100; // make sure we're not spawning in a wall
			//z = (int)map_height_at_point(m, x, y, &on_a_rock);
			z = 35;
		//} while (!entity_placeable(m,x,y,z,SPAWN_RADIUS,100,&spawn_z) && j<try_max);
		

		if (j < try_max ) {
			//if (spawn_z != 0.0) z = (int)ceil(spawn_z);
			//TakenArea_AddCircle(m->ta,x,y,z,SPAWN_RADIUS,100,"Spawn",0);
			angle = (float)genrand(360);   // generate an angle between 0 and 360
			if (i%2) {
				map_spawn_point(f, pt(x,y,z), SPAWN_CTF_ALLIED, 0.0, angle, 0.0);
			} else {
				map_spawn_point(f, pt(x,y,z), SPAWN_CTF_AXIS, 0.0, angle, 0.0);
			}
		}
	}								
}
	
static void map_flag_points(MAP *m)
{
	TERRAIN *t = m->terrain;
	FILE *f = m->map_file;
	int x,y,z;
	int i,j;
	int on_a_rock = 0;

	// for "intermission" camera
	map_spawn_point(f, pt(m->x_size/2, m->y_size/2, m->z_size - 360), SPAWN_CAMERA, 30.0, 45.0, 0.0);

	// flags for capture-the-flag	
	i = 1;
	j = t->y_res / 2;
	x = ((float)i + 0.5)* (float)m->x_size / (float)t->x_res;
	y = m->y_size / 2;
	z = (int)map_height_at_point(m, x, y, &on_a_rock);
	TakenArea_AddCircle(m->ta,x,y,z,50,100,"Spawn",0);
	map_ctf_flag(f, pt(x,y,z), 0); // allied

	i = t->x_res - 2;
	j = t->y_res / 2;
	x = ((float)i - 0.5)* (float)m->x_size / (float)t->x_res;
	y = m->y_size / 2;
	z = (int)map_height_at_point(m, x, y, &on_a_rock);
	TakenArea_AddCircle(m->ta,x,y,z,50,100,"Spawn",0);
	map_ctf_flag(f, pt(x,y,z), 1); // axis

}

// returns 1 if dom is ok, otherwise 0.
static int validate_conf_dom(DOMNODE *conf)
{
	char *required[] = { "map", "sky", "terrain" };
	int num_req = sizeof(required) / sizeof(char *);
	int i=0;

	for (i=0; i<num_req; i++) {
		if (!XMLDomGetChildNamed(conf, required[i])) {
			fprintf(stderr, "ERROR: XML config file missing required element: %s", required[i]);
			return 0;
		}
	}

	return 1;
}

static void usage(char *progname, char *fmt, ...)
{
	printf("Usage: %s [map_name] [opts]\n"
		   "       map_name   Name of the Map to generate. '-' for stdout.\n"
		   "       -h         Display this list.\n"
		   "       -s [seed]  Set the seed for Random Numbers\n"
		   "       -c config  Use configuration xml file other than default.xml\n", progname);
	if (fmt && fmt[0]) {
		va_list args;
		printf("\n");
		va_start(args,fmt);
		vprintf(fmt,args);
		printf("\n");
		va_end(args);
	}

	exit(1);
}

int main(int argc, char *argv[])
{
	FILE *f=NULL;
	MAP m;
	TERRAIN *t = NULL;
	DOMNODE *node_map = NULL;
	DOMNODE *node_sky = NULL;
	DOMNODE *node_terrain = NULL;
	char *sky_texture = NULL;
	int height = 0;
	int x=0;
	int y=0;
	int z=0;
	int i=0;
	int j=0;
	char *progname = argv[0];
	char *map_name="";
	int c,seed=0;
	int sky_height=0;	

	if (argc < 2) {
		usage(progname,"Map name is required");
	}

	memset(&m, 0, sizeof(MAP));
	m.ta = TakenArea_Init(0);
	m.buildings = building_list_create();

	for (c=1;c<argc;c++) {
		switch (argv[c][0]) {
			case '-':
				switch (argv[c][1]) {
					case 'h':
						usage(progname,"");
						break;
					case 'c':
						if (c < argc-1) {							
							m.conf = XMLDomParseFileName(argv[++c]);
						} else {
							usage(progname,"-c option requires a value");
							return;
						}
						break;
					case 's':
						if (c < argc-1) {
							seed = atoi(argv[++c]);
						} else {
							usage(progname,"-s option requires a value");
						}
						break;
					case '\0':
						if (c == 1) {
							// they specified stdout for the target
							map_name = argv[c];
						} else {
							usage(progname,"Unknown option - %s",argv[c]);
						}
						break;
					default:
						usage(progname,"Unknown option: %s",argv[c]);
						break;
				}
				break;
			default:
				if (c == 1) {
					map_name = argv[1];
				} else {
					usage(progname,"Map Name argument must be the first argument.");
				}
				break;
		}
	}
	if (map_name && map_name[0] != '-') {
		char buf[1024];
		if (seed > 0) {
			genrand_srand(seed);
		} else {
			genrand_seed(map_name);
		}
		snprintf(buf,sizeof(buf),"%s.map",map_name);
		f = fopen(buf,"w");
	} else {
		genrand_srand((unsigned int) time ((time_t *) NULL));
		f = stdout;
	}

	if (m.conf == NULL) {
		// get default config
		m.conf = XMLDomParseFileName(DEFAULT_CONF_XML);
		if (m.conf == NULL) {
			fprintf(stderr, "ERROR: Unable to parse %s config file.", DEFAULT_CONF_XML);
			return -1;
		}
	}

	// make sure the config file contains minimum required elements
	if (!validate_conf_dom(m.conf)) {
		return -1;
	}

	// .map file
	m.map_file = f;

	node_map = XMLDomGetChildNamed(m.conf, "map");
	node_sky = XMLDomGetChildNamed(m.conf, "sky");
	sky_texture = XMLDomGetAttribute(pick_child_random(node_sky), "name");
	node_terrain = XMLDomGetChildNamed(m.conf, "terrain");

	m.outer_wall_height = XMLDomGetAttributeInt(node_map, "outer_wall_height", 300);
	m.x_size = XMLDomGetAttributeInt(node_map, "x_size", 4500);
	m.y_size = XMLDomGetAttributeInt(node_map, "y_size", 2 * m.x_size / 3);	
	m.z_size = XMLDomGetAttributeInt(node_map, "z_size", 1350);
	m.height = XMLDomGetAttributeInt(node_terrain, "height", 250);
	m.x_offset = 0;
	m.y_offset = 0;
	m.rock_texture[0] = 0;
	m.mound_texture[0] = 0;
	m.trench_texture[0] = 0;
	sky_height = m.z_size;

	map_header(f);

	// sky box
	hollowbox(f, pt(-2, -2, -2 - SKY_OFFSET), m.x_size + 4, m.y_size + 4, sky_height + m.height,
			  1, sky_texture, HOLLOW_DEFAULT);

	// clip box (prevent player from walking off the map somehow, or even jumping onto the outside wall)	
	hollowbox(f, pt(-1, -1, -1 - SKY_OFFSET), m.x_size + 2, m.y_size + 2, sky_height + m.height,
			  1, "clip", HOLLOW_TOPLESS); // topless so that grenades don't bounce off of ceiling

	//
	// playable area starts at 0,0,0 and extends to x_size,y_size,WALL_OUTER_HEIGHT	 
	//
	
	// terrain
	t = terrain_generate(XMLDomGetAttributeInt(node_terrain, "x_res", 50),
						 XMLDomGetAttributeInt(node_terrain, "y_res", 50),
						 XMLDomGetAttributeFloat(node_terrain, "smoothness", 0.65));
	if (!t) {
		fprintf(stderr, "ERROR - unable to create terrain.\n\n");
	}

	m.terrain = t;
	m.terrain_texture = rand_txt_node(node_terrain);
	m.grid_x_unit = (float)m.x_size / (float)(t->x_res-1);
	m.grid_y_unit = (float)m.y_size / (float)(t->y_res-1);


	//terrain_to_png("images/map.png", t, 0);

	// decrease height if necessary
	//// make_traversible(&m); TODO - fix this function.  for large meshes, the terrain is getting flattened too much

	// outer walls
	hollowbox(f, pt(-1,-1,-1), m.x_size+2, m.y_size+2, m.outer_wall_height, 1,
			  rand_txt_node(node_map), HOLLOW_TOPLESS|HOLLOW_BOTTOMLESS);


	regions_begin(&m, 2);

	// show mesh points
	output_terrain(&m);
	regions_terrain(&m);

	fprintf(f, "}\n");

	// -- needs a bigger model for grass... we are having too many entities.
	//map_add_grass_everywhere(&m);
	//
	map_flag_points(&m);

	if (map_name && map_name[0] != '-' && map_name[0] != 0) {
		generate_arena(map_name, m.conf);
		generate_gsc(map_name, &m);
	}

	regions_fill(&m);

	if (map_name && map_name[0] != '-') {
		char pngname[256];
		snprintf(pngname, sizeof(pngname), "images/%s.png", map_name);
		terrain_to_png(pngname, t, 0);
	} else {
		terrain_to_png("images/map.png", t, 0);
	}

	// random DM & TDM spawn points
	map_spawn_points_random(&m);

	// random CTF spawn points
	map_spawn_points_random_ctf(&m);

	TakenArea_AddBoxPoint(m.ta, pt(0,0,0), m.x_size, 50, m.outer_wall_height, "outerwall", 0);
	TakenArea_AddBoxPoint(m.ta, pt(0,0,0), 50, m.y_size, m.outer_wall_height, "outerwall", 0);
	TakenArea_AddBoxPoint(m.ta, pt(0,m.y_size+1,0), m.x_size, 50, m.outer_wall_height, "outerwall", 0);	
	TakenArea_AddBoxPoint(m.ta, pt(m.x_size+2,0,0), 50, m.y_size, m.outer_wall_height, "outerwall", 0);

    map_add_flavor(&m); // tasty

	// cleanup, cleanup, everybody everywhere.  cleanup, cleanup, everybody do your share.
	terrain_destroy(t);
	TakenArea_Destroy(m.ta);
	kill_regions(&m);
	building_list_destroy(m.buildings);
	XMLDomDestroy(m.conf);

	if (map_name && map_name[0] != '-') {
		fclose(f);
	}
	//MAPINFO mi;
	//mapfile_parse("prefabs/codgen/building_base.map", &mi);
	//fprintf(stderr, "%d %d %d, %d %d %d\n", mi.p1.x, mi.p1.y, mi.p1.z, mi.p2.x, mi.p2.y, mi.p2.z);

	return 0;
}

