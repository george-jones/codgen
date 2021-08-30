#ifndef TAKEN_AREA_H
#define TAKEN_AREA_H

#include "primitives.h"

typedef struct {
	float x;
	float y;
	float z;
	float x_radius;
	float y_radius;
	float height;
	char desc[255+1];
	int place_above;
} TAKEN_POINT;

typedef struct {
	TAKEN_POINT *points;
	int num_points;
	int alloced;
} TAKEN_AREA;

TAKEN_AREA *TakenArea_Init(int n);
void TakenArea_Destroy(TAKEN_AREA *t);
void TakenArea_AddCircle(TAKEN_AREA *t, float x, float y, float z, float radius, float height, char *desc, int place_above);
void TakenArea_AddBox(TAKEN_AREA *t, float x, float y, float z, float x_radius, float y_radius, float height, char *desc, int place_above);
void TakenArea_AddBoxPoint(TAKEN_AREA *t, IntPoint3 *p, float xlen, float ylen, float zlen, char *desc, int place_above);

// Returns >0.  Fills allowed_z with z-value if this is a place_above-able location.
// Otherwise allowed_z will be exactly 0.0;
// Pass NULL for spawn_z if you don't care.
int TakenArea_IsIn(TAKEN_AREA *t, float x, float y, float z, float radius, float height, float *allowed_z);
int TakenArea_BoxIn(TAKEN_AREA *t, FloatPoint3 *four_points, float height);


#endif // TAKEN_AREA_H
