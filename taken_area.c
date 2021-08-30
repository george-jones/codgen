#include <stdlib.h>
#include <stdio.h>

#include "codgen.h"
#include "taken_area.h"

TAKEN_AREA *TakenArea_Init(int n)
{
	TAKEN_AREA *t = calloc(1,sizeof(TAKEN_AREA));
	if (t) {
		if (n <= 0) {
			n = 10;
		}
		t->points = calloc(n,sizeof(TAKEN_POINT));
		t->alloced = n;
	}
	return t;
}

void TakenArea_Destroy(TAKEN_AREA *t)
{
	if (t->points) free(t->points);
	t->alloced = 0;
	t->num_points = 0;
}

void TakenArea_AddCircle(TAKEN_AREA *t, float x, float y, float z, float radius, float height, char *desc, int place_above)
{
	TAKEN_POINT *c;
	
	if (t->num_points >= t->alloced) {
		t->alloced+=10;
		t->points = realloc(t->points,(t->alloced)*sizeof(TAKEN_POINT));
		if (!t->points) {
			fprintf(stderr,"AAAAAAAAAAAAAAH\n");
		}
	}
	c = &(t->points[t->num_points++]);
	c->x = x;
	c->y = y;
	c->z = z;
	c->x_radius = radius;
	c->y_radius = radius;
	c->height = height;
	c->place_above = place_above;
	if (desc) {
		strncpy(c->desc,desc,sizeof(c->desc));
	} else {
		strncpy(c->desc,"Unknown",sizeof(c->desc));
	}
	if (DBG) fprintf(stderr,"Added Taken Point (%d,%d,%d) of size %d for %s\n",(int)x,(int)y,(int)z,(int)radius,c->desc);
}

void TakenArea_AddBox(TAKEN_AREA *t, float x, float y, float z, float x_radius, float y_radius, float height, char *desc, int place_above)
{
	TAKEN_POINT *c;
	
	if (t->num_points >= t->alloced) {
		t->alloced+=10;
		t->points = realloc(t->points,(t->alloced)*sizeof(TAKEN_POINT));
		if (!t->points) {
			if (DBG) fprintf(stderr,"AAAAAAAAAAAAAAH\n");
		}
	}
	c = &(t->points[t->num_points++]);
	c->x = x;
	c->y = y;
	c->z = z;
	c->x_radius = x_radius;
	c->y_radius = y_radius;
	c->height = height;
	c->place_above = place_above;
	if (desc) {
		strncpy(c->desc,desc,sizeof(c->desc));
	} else {
		strncpy(c->desc,"Unknown",sizeof(c->desc));
	}
	if (DBG) fprintf(stderr,"Added Taken Point (%d,%d,%d) of size %dx%d for %s\n",(int)x,(int)y,(int)z,(int)x_radius,(int)y_radius,c->desc);
}

void TakenArea_AddBoxPoint(TAKEN_AREA *t, IntPoint3 *p, float xlen, float ylen, float zlen, char *desc, int place_above)
{
	TakenArea_AddBox(t, p->x + xlen/2, p->y + ylen/2, p->z, xlen/2, ylen/2, zlen, desc, place_above);
}

int TakenArea_IsIn(TAKEN_AREA *t, float x, float y, float z, float radius, float height, float *allowed_z)
{
	int i, x_conflict, y_conflict, z_conflict;
	float p_side_1,p_side_2,e_side_1,e_side_2;

	if (z < 0.0) return 1;

	for (i=0;i<t->num_points;i++) {
		x_conflict = y_conflict = z_conflict = 0;

		e_side_1 = t->points[i].x - t->points[i].x_radius;
		e_side_2 = t->points[i].x + t->points[i].x_radius;
		p_side_1 = x - radius;
		p_side_2 = x + radius;
		if ((p_side_1 < e_side_2 && p_side_2 >= e_side_1) || p_side_2 < e_side_1 && p_side_1 >= e_side_1) {
			x_conflict = 1;
			if (DBG) fprintf(stderr,"X confict found\n");
		}
		e_side_1 = t->points[i].y - t->points[i].y_radius;
		e_side_2 = t->points[i].y + t->points[i].y_radius;
		p_side_1 = y - radius;
		p_side_2 = y + radius;
		if ((p_side_1 < e_side_2 && p_side_2 >= e_side_1) || p_side_2 < e_side_1 && p_side_1 >= e_side_1) {
			y_conflict = 1;
			if (DBG) fprintf(stderr,"Y confict found\n");
		}
		e_side_1 = t->points[i].z;
		e_side_2 = t->points[i].z + t->points[i].height;
		p_side_1 = z;
		p_side_2 = z + height;
		if ((p_side_1 < e_side_2 && p_side_2 >= e_side_1) || p_side_2 < e_side_1 && p_side_1 >= e_side_1) {
			z_conflict = 1;
			if (DBG) fprintf(stderr,"Z confict found\n");
		}
		if (x_conflict && y_conflict && (z_conflict || allowed_z != NULL)) {
			if (allowed_z != NULL && t->points[i].place_above) {
				if (*allowed_z < t->points[i].z + t->points[i].height) {
					*allowed_z = (float)(t->points[i].z + t->points[i].height);
				}
				continue;
			}
			if (DBG) fprintf(stderr,"Disallowing point %d,%d,%d Because of %s @ %d,%d,%d\n",(int)x,(int)y,(int)z,t->points[i].desc,(int)t->points[i].x,(int)t->points[i].y,(int)t->points[i].x_radius);
			return 1;
		}
	}
	return 0;
}

int TakenArea_BoxIn(TAKEN_AREA *t, FloatPoint3 *four_points, float height)
{
	FloatPoint3 *p = NULL;
	int i = 0;

	for (i=0; i<4; i++) {
		p = &four_points[i];
		if (TakenArea_IsIn(t, p->x, p->y, p->z, 1.0, height, NULL)) return 1;		
	}

	return 0;
}

