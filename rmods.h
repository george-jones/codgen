// Region Mods

#ifndef _RMODS_H
#define _RMODS_H

#include "map.h"
#include "region.h"

void regions_eval(MAP *m);
void regions_brushes(MAP *m);
void regions_terrain(MAP *m);
void regions_entities(MAP *m);

// mods can call this to figure out if a particular point is actually in the region they are messing with.
// only the x and y components of the point are considered
int point_in_region(MAP *m, REGION *r, FloatPoint3 *p);

typedef void (*RMF_BRUSHES)(MAP *m, REGION *r);
typedef void (*RMF_ENTITIES)(MAP *m, REGION *r);
typedef void (*RMF_TERRAIN)(MAP *m, REGION *r);
typedef void (*RMF_CLEANUP)(REGION *r);

typedef struct {
	char name[50]; // name as found in configuration xml file
	RMF_BRUSHES brushes; // form the underlying terrain as needed
	RMF_TERRAIN terrain; // puts out terrain
	RMF_ENTITIES entities; // does the work of populating a region with stuff	
	RMF_CLEANUP cleanup; // frees any data allocated by other functions
} RMOD;

extern RMOD rmod_forest;
extern RMOD rmod_bridge;
extern RMOD rmod_rocks;
extern RMOD rmod_buildings;
extern RMOD rmod_graveyard;
extern RMOD rmod_mound;
extern RMOD rmod_trench;

#endif
