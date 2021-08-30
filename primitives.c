
#include <stdio.h>
#include <stdlib.h>

#include "primitives.h"

static void point(FILE *f, IntPoint3 *p)
{
	fprintf(f, " ( %d %d %d ) ", p->x, p->y, p->z);
}

void plane(FILE *f, IntPoint3 *p0, IntPoint3 *p1, IntPoint3 *p2, char *texture)
{
	point(f, p0);
	point(f, p1);
	point(f, p2);
	fprintf(f, "%s 256 256 0 0 0 0 lightmap_gray 16384 16384 0 0 0 0\n", texture);
}

static void box_txt(FILE *f, IntPoint3 *a, int height, char **textures)
{
	IntPoint3 p[4];
	int i=0;

	for (i=0; i<4; i++) {
		p[i].x = a[i].x;
		p[i].y = a[i].y;
		p[i].z = a[i].z + height;		
	}	

	fprintf(f, "{\n");
	plane(f, &p[3], &p[0], &p[1], textures[CUBE_FACE_TOP]);
	plane(f, &a[3], &p[3], &p[2], textures[CUBE_FACE_FRONT]);
	plane(f, &p[2], &p[1], &a[1], textures[CUBE_FACE_RIGHT]);
	plane(f, &p[1], &p[0], &a[0], textures[CUBE_FACE_BACK]);
	plane(f, &p[0], &p[3], &a[3], textures[CUBE_FACE_LEFT]);
	plane(f, &a[0], &a[3], &a[1], textures[CUBE_FACE_BOTTOM]);

	fprintf(f, "}\n");
}

/*
static void box_complex_txt(FILE *f, IntPoint3 *a, int height, char **textures)
{
	IntPoint3 p[4];
	int i=0;

	for (i=0; i<4; i++) {
		p[i].x = a[i].x;
		p[i].y = a[i].y;
		p[i].z = a[i].z + height;		
	}	

	fprintf(f, "{\n");	
	plane(f, &p[3], &p[0], &p[1], textures[CUBE_FACE_TOP]);
	plane(f, &p[1], &p[2], &p[3], textures[CUBE_FACE_TOP]);
	plane(f, &a[3], &p[3], &p[2], textures[CUBE_FACE_FRONT]);
	plane(f, &p[2], &a[2], &a[3], textures[CUBE_FACE_FRONT]);
	plane(f, &p[2], &p[1], &a[1], textures[CUBE_FACE_RIGHT]);
	plane(f, &a[1], &a[2], &p[2], textures[CUBE_FACE_RIGHT]);
	plane(f, &p[1], &p[0], &a[0], textures[CUBE_FACE_BACK]);
	plane(f, &a[0], &a[1], &p[1], textures[CUBE_FACE_BACK]);
	plane(f, &p[0], &p[3], &a[3], textures[CUBE_FACE_LEFT]);
	plane(f, &a[3], &a[0], &p[0], textures[CUBE_FACE_LEFT]);
	plane(f, &a[0], &a[3], &a[2], textures[CUBE_FACE_BOTTOM]);
	plane(f, &a[2], &a[1], &a[0], textures[CUBE_FACE_BOTTOM]);

	fprintf(f, "}\n");
}

void box_complex(FILE *f, IntPoint3 *a, int height, char *texture)
{
	char *textures[6];
	textures[0] = textures[1] = textures[2] = textures[3] = textures[4] = textures[5] = texture;
	box_complex_txt(f,a,height,textures);
}
*/

void box(FILE *f, IntPoint3 *a, int height, char *texture)
{
	char *textures[6];

	textures[0] = textures[1] = textures[2] = textures[3] = textures[4] = textures[5] = texture;
	box_txt(f,a,height,textures);
}

void portal(FILE *f, IntPoint3 *a, int height, CUBE_FACE face)
{
	char *textures[6];

	textures[0] = textures[1] = textures[2] = textures[3] = textures[4] = textures[5] = "portal_nodraw";
	textures[face] = "portal";

	box_txt(f,a,height,textures);
}

void box_easy(FILE *f, IntPoint3 *pt, int x_size, int y_size, int z_size, char *texture)
{
	IntPoint3 p[4];

	p[0].x = pt->x;
	p[0].y = pt->y + y_size;

	p[1].x = pt->x + x_size;
	p[1].y = pt->y + y_size;

	p[2].x = pt->x + x_size;
	p[2].y = pt->y;

	p[3].x = pt->x;
	p[3].y = pt->y;

	p[0].z = p[1].z = p[2].z = p[3].z = pt->z;
	box(f, p, z_size, texture);
}

void hollowbox(FILE *f, IntPoint3 *pt, int x_size, int y_size, int z_size, int thickness, char *texture, int options)
{
	IntPoint3 p[4];

	if ((options & HOLLOW_BOTTOMLESS) == 0) {
		// floor
		p[0].x = pt->x;
		p[0].y = pt->y + y_size;
	
		p[1].x = pt->x + x_size;
		p[1].y = pt->y + y_size;
	
		p[2].x = pt->x + x_size;
		p[2].y = pt->y;
	
		p[3].x = pt->x;
		p[3].y = pt->y;
	
		p[0].z = p[1].z = p[2].z = p[3].z = pt->z;
		box(f, p, thickness, texture);
	}

	if ((options & HOLLOW_TOPLESS) == 0) {	
		// ceiling
		p[0].x = pt->x;
		p[0].y = pt->y + y_size;
	
		p[1].x = pt->x + x_size;
		p[1].y = pt->y + y_size;
	
		p[2].x = pt->x + x_size;
		p[2].y = pt->y;
	
		p[3].x = pt->x;
		p[3].y = pt->y;

		p[0].z = p[1].z = p[2].z = p[3].z = pt->z + z_size - thickness;
		box(f, p, thickness, texture);
	}

	// left wall
	p[0].x = pt->x;
	p[0].y = pt->y + y_size;

	p[1].x = pt->x + thickness;
	p[1].y = pt->y + y_size;

	p[2].x = pt->x + thickness;
	p[2].y = pt->y;

	p[3].x = pt->x;
	p[3].y = pt->y;

	p[0].z = p[1].z = p[2].z = p[3].z = pt->z + thickness;
	box(f, p, z_size - 2*thickness, texture);

	// right wall
	p[0].x = pt->x + x_size - thickness;
	p[0].y = pt->y + y_size;

	p[1].x = pt->x + x_size;
	p[1].y = pt->y + y_size;

	p[2].x = pt->x + x_size;
	p[2].y = pt->y;

	p[3].x = pt->x + x_size - thickness;
	p[3].y = pt->y;

	p[0].z = p[1].z = p[2].z = p[3].z = pt->z + thickness;
	box(f, p, z_size - 2*thickness, texture);

	// top wall
	p[0].x = pt->x + thickness;
	p[0].y = pt->y + y_size;

	p[1].x = pt->x + x_size - thickness;
	p[1].y = pt->y + y_size;

	p[2].x = pt->x + x_size - thickness;
	p[2].y = pt->y + y_size - thickness;

	p[3].x = pt->x + thickness;
	p[3].y = pt->y + y_size - thickness;

	p[0].z = p[1].z = p[2].z = p[3].z = pt->z + thickness;
	box(f, p, z_size - 2*thickness, texture);

	// bottom wall
	p[0].x = pt->x + thickness;
	p[0].y = pt->y + thickness;

	p[1].x = pt->x + x_size - thickness;
	p[1].y = pt->y + thickness;

	p[2].x = pt->x + x_size - thickness;
	p[2].y = pt->y;

	p[3].x = pt->x + thickness;
	p[3].y = pt->y;

	p[0].z = p[1].z = p[2].z = p[3].z = pt->z + thickness;
	box(f, p, z_size - 2*thickness, texture);	
}

static FloatPoint2 *bfop_add_pt(FloatPoint2 *pts, float x, float y, int *n)
{
	int num = *n + 1;

	pts = (FloatPoint2 *)realloc(pts, sizeof(FloatPoint2) * num);
	if (pts) {	
		pts[num-1].x = x;
		pts[num-1].y = y;
		*n = num;
	}

	return pts;
}

#define sgndiff(a,b) ((((a)>0)?1:0) != (((b)>0)?1:0))

float interpolate(float a1, float b1, float a2, float b2, float a)
{
	if (a2-a1 != 0.0) {	
		return b1 + (b2-b1)*(a-a1)/(a2-a1);
	} else {
		return b1;
	}
}

FloatPoint2 *boxes_find_overlay_points(FloatPoint2 *a_origin, float a_length, float a_height,
									   FloatPoint2 *b_origin, float b_length, float b_height, int *num_pts)
{
	FloatPoint2 *r = NULL;
	FloatPoint2 a[4], b[4];
	int n = 0;
	int i=0;
	int j=0;	

	a[0].x = a_origin->x;
	a[0].y = a_origin->y;
	a[1].x = a_origin->x + a_length;
	a[1].y = a_origin->y;
	a[2].x = a_origin->x + a_length;
	a[2].y = a_origin->y + a_height;
	a[3].x = a_origin->x;
	a[3].y = a_origin->y + a_height;

	b[0].x = b_origin->x;
	b[0].y = b_origin->y;
	b[1].x = b_origin->x + b_length;
	b[1].y = b_origin->y;
	b[2].x = b_origin->x + b_length;
	b[2].y = b_origin->y + b_height;
	b[3].x = b_origin->x;
	b[3].y = b_origin->y + b_height;

	// check for all a-inside-b points
	for (i=0; i<4; i++) {
		if (a[i].x >= b[0].x && a[i].x <= b[1].x && a[i].y >= b[0].y && a[i].y <= b[2].y) {
			r = bfop_add_pt(r, a[i].x, a[i].y, &n);
		}
	}

	// check for all b-inside-a points
	for (i=0; i<4; i++) {
		if (b[i].x >= a[0].x && b[i].x <= a[1].x && b[i].y >= a[0].y && b[i].y <= a[2].y) {
			r = bfop_add_pt(r, b[i].x, b[i].y, &n);
		}
	}

	// trickier: line intersections

	//    +-----+
	//    |     |
	// +--X-----X--+
	// |  |     |  |
	// +--X-----X--+
	//    |     |
	//    +-----+

	if (a[0].y >= b[0].y && a[0].y <= b[3].y && sgndiff(a[0].x - b[0].x, a[1].x - b[0].x)) {
		r = bfop_add_pt(r, b[0].x, a[0].y, &n);
	}

	if (a[1].y >= b[1].y && a[1].y <= b[2].y && sgndiff(a[0].x - b[1].x, a[1].x - b[1].x)) {
		r = bfop_add_pt(r, b[1].x, a[1].y, &n);
	}

	if (a[2].y >= b[1].y && a[2].y <= b[2].y && sgndiff(a[3].x - b[2].x, a[2].x - b[2].x)) {
		r = bfop_add_pt(r, b[2].x, a[2].y, &n);
	}

	if (a[3].y >= b[0].y && a[3].y <= b[3].y && sgndiff(a[3].x - b[3].x, a[2].x - b[3].x)) {
		r = bfop_add_pt(r, b[3].x, a[3].y, &n);
	}

	if (b[0].y >= a[0].y && b[0].y <= a[3].y && sgndiff(b[0].x - a[0].x, b[1].x - a[0].x)) {
		r = bfop_add_pt(r, a[0].x, b[0].y, &n);
	}

	if (b[1].y >= a[1].y && b[1].y <= a[2].y && sgndiff(b[0].x - a[1].x, b[1].x - a[1].x)) {
		r = bfop_add_pt(r, a[1].x, b[1].y, &n);
	}

	if (b[2].y >= a[1].y && b[2].y <= a[2].y && sgndiff(b[3].x - a[2].x, b[2].x - a[2].x)) {
		r = bfop_add_pt(r, a[2].x, b[2].y, &n);
	}

	if (b[3].y >= a[0].y && b[3].y <= a[3].y && sgndiff(b[3].x - a[3].x, b[2].x - a[3].x)) {
		r = bfop_add_pt(r, a[3].x, b[3].y, &n);
	}

	*num_pts = n;
	return r;
}
