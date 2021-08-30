
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "terrain.h"
#include "codgen_random.h"
#include "primitives.h"

#define tp(t,x,y) t->points[x + y*t->x_res]

TERRAIN *terrain_create(int x_res, int y_res)
{
	TERRAIN *t = NULL;

	t = (TERRAIN *)malloc(sizeof(TERRAIN));
	if (t) {
		t->x_res = x_res;
		t->y_res = y_res;		
		t->points = (float *)malloc(sizeof(float) * (x_res * y_res));
		if (t->points) {
			terrain_flatten(t);
		} else {
			free(t->points);
		}
		
	}

	return t;
}

TERRAIN *terrain_generate(int x_res, int y_res, float s)
{
	TERRAIN *t = NULL;
	float fudge = 0.2;
	int i=0;
	int p=0;

	t = terrain_create(x_res, y_res);
	if (t) {	

		p = (int)(s * (float)t->x_res * (float)t->y_res * s * fudge);

		terrain_randomize(t);

		for (i=0; i<p; i++) {
			terrain_blur(t);
		}
		terrain_amplify(t);
	}

	return t;
}

TERRAIN *terrain_copy(TERRAIN *t)
{
	TERRAIN *c = NULL;
	int i=0;
	int j=0;

	c = terrain_create(t->x_res, t->y_res);
	if (c) {
		for (j=0; j<t->y_res; j++) {
			for (i=0; i<t->x_res; i++) {
				tp(c,i,j) = tp(t,i,j);
			}
		}
	}

	return c;
}

void terrain_destroy(TERRAIN *t)
{
	if (t) {
		free(t->points);
		free(t);
	}
}

void terrain_flatten(TERRAIN *t)
{
	int x=0;
	int y=0;

	for (y=0; y<t->y_res; y++) {
		for (x=0; x<t->x_res; x++) {
			tp(t,x,y) = 0.0;
		}
	}
}

#define plusOne(n,lim) ((n+1)%lim)
#define minusOne(n,lim) ((n-1<0)?lim-1:n-1)

void terrain_randomize(TERRAIN *t)
{	
	int x=0;
	int y=0;

	for (y=0; y < t->y_res; y++) {
		for (x=0; x < t->x_res; x++) {
			tp(t,x,y) = (float) genrand(MAX_GEN_RANDOM)/RAND_MAX;
		}
	}
}

void terrain_blur_area(TERRAIN *t, int x1, int y1, int x2, int y2)
{
	TERRAIN *c = NULL;	
	int x=0;
	int y=0;

	// copy	
	c = terrain_create(t->x_res, t->y_res);
	if (!c) return;
	for (y=0; y < c->y_res; y++) {
		for (x=0; x < c->x_res; x++) {
			tp(c,x,y) = tp(t,x,y);
		}
	}		
			
	for (y=y1; y <= y2; y++) {
		for (x=x1; x <= x2; x++) {
			tp(t,x,y) = 0.5*tp(c, x, y)
				+ 0.1 * tp(c, plusOne(x,c->x_res), y)
				+ 0.1 * tp(c, minusOne(x,c->x_res), y)
				+ 0.1 * tp(c, x ,plusOne(y,c->y_res))
				+ 0.1 * tp(c, x ,minusOne(y,c->y_res))
				+ 0.025*tp(c, plusOne(x,c->x_res), plusOne(y,c->y_res))
				+ 0.025*tp(c, plusOne(x,c->x_res), minusOne(y,c->y_res))
				+ 0.025*tp(c, minusOne(x,c->x_res), plusOne(y,c->y_res))				
				+ 0.025*tp(c, minusOne(x,c->x_res), minusOne(y,c->y_res));
		}
	}
	
	terrain_destroy(c);
}

void terrain_blur(TERRAIN *t)
{
	terrain_blur_area(t, 0, 0, t->x_res-1, t->y_res-1);
}

void terrain_amplify(TERRAIN *t)
{
	float low = 0.0;
	float high = 0.0;
	float mid = 0.0;
	float amp = 0.0;
	float low_mult = 0.0;
	float high_mult = 0.0;
	float v = 0.0;
	int x=0;
	int y=0;	

	// find low and high
	for (y=0; y < t->y_res; y++) {
		for (x=0; x < t->x_res; x++) {
			if (y==0 && x==0) {
				low = tp(t, x, y);
			} else if (tp(t, x, y) < low) {
				low = tp(t, x, y);
			} else if (tp(t, x, y) > high) {
				high = tp(t, x, y);
			}
		}
	}
	
	amp = high - low;
	// If the averaging out has caused a perfectly smooth map, we simply cannot force any detail
	// out of it.  Plus we'd end up dividing by zero.  This is exceptionally unlikely though unless
	// the terrain has been smoothed a million times or something, or was flat to begin with.
	if (amp == 0.0) return;
	mid = (high + low) / 2.0;
	low_mult = 0.49 / (mid - low);
	high_mult = 0.49 / (high - mid);
	
	// adjust
	for (y=0; y < t->y_res; y++) {
		for (x=0; x < t->x_res; x++) {
			v = tp(t, x, y);
			if (v > mid) {
				tp(t, x, y) = (v - mid) * high_mult + 0.5;
			} else {
				tp(t, x, y) = (v - low) * low_mult;
			}
		}
	}	
	
}

float terrain_get_point(TERRAIN *t, int x, int y)
{
	return tp(t,x,y);
}

void terrain_set_point(TERRAIN *t, int x, int y, float z)
{
	tp(t,x,y) = z;
}

void terrain_multiply(TERRAIN *t, float v)
{
	int x=0;
	int y=0;

	for (y=0; y < t->y_res; y++) {
		for (x=0; x < t->x_res; x++) {
			tp(t, x, y) *= v;
		}
	}
}

// just an absolute value definition
#define TSABS(a) (((a) < 0)?(-1 * (a)):a)
float terrain_find_max_x_slope(TERRAIN *t)
{
	float max_slope = 0.0;
	float z1 = 0.0;
	float z2 = 0.0;
	float s = 0.0;
	int x=0;
	int y=0;

	for (y=0; y < t->y_res-1; y++) {
		for (x=0; x < t->x_res-1; x++) {
			// slope to point to the right
			z1 = tp(t,x,y);
			z2 = tp(t,x+1,y);
			s = (z2-z1); // no need to divide by "run" because it is just 1
			s = TSABS(s);
			if (s > max_slope) {
				max_slope = s;
			}
		}
	}

	return max_slope;
}

float terrain_find_max_y_slope(TERRAIN *t)
{
	float max_slope = 0.0;
	float z1 = 0.0;
	float z2 = 0.0;
	float s = 0.0;
	int x=0;
	int y=0;

	for (y=0; y < t->y_res-1; y++) {
		for (x=0; x < t->x_res-1; x++) {
			// slope to point below
			z1 = tp(t,x,y);			
			z2 = tp(t,x,y+1);
			s = (z2-z1);
			s = TSABS(s);
			if (s > max_slope) {
				max_slope = s;
			}
		}
	}

	return max_slope;
}

// distance squared
#define DIST2(x1, y1, x2, y2) (powf(x1 - x2, 2.0) + powf(y1 - y2, 2.0))

float terrain_height_at_point(TERRAIN *t, float x, float y, int x_size, int y_size, int terrain_height)
{
	float x_mult, y_mult;
	float x1, x2, y1, y2;
	float tx1, tx2, tx3, ty1, ty2, ty3, tz1, tz2, tz3;
	float f1,f2,f3,f4;
	int i1, i2, j1, j2;
	float A,B,C,negD;
	float z;

	// f2
	// +---------+ x2,y2,f3
	// |\        |
	// | \ upper |
	// |  \      |
	// |   \     |
	// |    \    |
	// |     \   |
	// |      \  |
	// | lower \ |
	// |        \|
	// +---------+ f4
	// x1,y1
	// f1

	x_mult = (float)x_size / (float)(t->x_res-1);
	y_mult = (float)y_size / (float)(t->y_res-1);	

	if (x > x_size) x = (float)x_size;	
	if (y > y_size) y = (float)y_size;

	// find the four surrounding points.  Or if we're exactly on a line or point, some of these will be equal
	i1 = (int)(floor(x / x_mult));
	i2 = (int)(ceil(x / x_mult));
	j1 = (int)(floor(y / y_mult));
	j2 = (int)(ceil(y / y_mult));

	if (i2 >= t->x_res) i2 = t->x_res-1;
	if (j2 >= t->y_res) j2 = t->y_res-1;	

	// corner coordinates
	x1 = (float)i1 * x_mult;
	x2 = (float)i2 * x_mult;
	y1 = (float)j1 * y_mult;
	y2 = (float)j2 * y_mult;

	// height at each corner
	f1 = terrain_get_point(t, i1, j1) * terrain_height;
	f2 = terrain_get_point(t, i1, j2) * terrain_height;
	f3 = terrain_get_point(t, i2, j2) * terrain_height;
	f4 = terrain_get_point(t, i2, j1) * terrain_height;

	// special (easy) cases

	if (i1 == i2 && j1 == j2) { // exactly on a grid point
		return f1;
	} else if (i1 == i2) { // on a constant-x grid line
		float m = (f2 - f1) / (y2 - y1);
		float b = f1 - m * y1;
		return m * y + b;
	} else if (j1 == j2) { // on a constant-y grid line
		float m = (f4 - f1) / (x2 - x1);
		float b = f1 - m * x1;
		return m * x + b;
	}

	// figure out which triangle we're in
	if (x / x_mult - i1 + y / y_mult - j1 <= 1.0) {
		tx1 = x1;
		ty1 = y1;
		tz1 = f1;

		tx2 = x1;
		ty2 = y2;
		tz2 = f2;

		tx3 = x2;
		ty3 = y1;
		tz3 = f4;

	} else { // "upper" triangle
		tx1 = x1;
		ty1 = y2;
		tz1 = f2;

		tx2 = x2;
		ty2 = y2;
		tz2 = f3;

		tx3 = x2;
		ty3 = y1;
		tz3 = f4;
	}

	// find plane connecting 3 points
	A = ty1*(tz2 - tz3) + ty2*(tz3 - tz1) + ty3*(tz1 - tz2);
	B = tz1*(tx2 - tx3) + tz2*(tx3 - tx1) + tz3*(tx1 - tx2);
	C = tx1*(ty2 - ty3) + tx2*(ty3 - ty1) + tx3*(ty1 - ty2);
	negD = tx1*(ty2*tz3 - ty3*tz2) + tx2*(ty3*tz1 - ty1*tz3) + tx3*(ty1*tz2 - ty2*tz1);

	// Ax + By + Cz + D = 0
	// z = (-D - Ax - By) / C

	if (C != 0) {
		z = (negD - A*x - B*y) / C;
	} else  {
		z = 0.0;
	}	

	return z;
}

