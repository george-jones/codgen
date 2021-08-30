#ifndef _MAP_H
#define _MAP_H

#include <stdio.h>

#include "terrain.h"
#include "taken_area.h"
#include "region.h"
#include "xml.h"
#include "rmod_buildings.h"

typedef struct {
	FILE *map_file;
	TERRAIN *terrain;
	char *terrain_texture;
	REGION **regions;
	int num_regions;
	TAKEN_AREA *ta;
	DOMNODE *conf;
	BLDG_LIST *buildings;
	char rock_texture[256];
	char mound_texture[256];
	char trench_texture[256];
	int x_size;
	int x_offset;
	int y_size;
	int y_offset;
	int z_size;
	int height;
	int outer_wall_height;
	int sky_height;
	float grid_x_unit;
	float grid_y_unit;
} MAP;

#endif

