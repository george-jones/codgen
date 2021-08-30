
#ifndef _RMOD_BUILDINGS_H
#define _RMOD_BUILDINGS_H

#include "primitives.h"

typedef struct {
	IntPoint3 points[4]; // points are in order of top-left, top-right, bottom-right, bottom-left (like terrain)
} BLDG;

typedef struct {
	BLDG *b;
	int num;
} BLDG_LIST;

BLDG_LIST *building_list_create();

void building_list_destroy(BLDG_LIST *l);

void building_add(BLDG_LIST *l, BLDG *b);

BLDG *building_contains(BLDG_LIST *l, IntPoint3 *p);

#endif
