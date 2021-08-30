#ifndef _PRIMITIVES_H
#define _PRIMITIVES_H

#include <stdio.h>

typedef struct {
	int x;
	int y;	
} IntPoint2;

typedef struct {
	int x;
	int y;
	int z;
} IntPoint3;

typedef struct {
	int x;
	int y;
	float z;
} MixedPoint3;

typedef struct {
	float x;
	float y;
} FloatPoint2;

typedef struct {
	float x;
	float y;
	float z;
} FloatPoint3;

void plane(FILE *f, IntPoint3 *p0, IntPoint3 *p1, IntPoint3 *p2, char *texture);

// "a" should be an array of 4 points - topLeft, topRight, bottomRight, bottomLeft
void box(FILE *f, IntPoint3 *a, int height, char *texture);

// like box(), only easier, but not as flexible.
void box_easy(FILE *f, IntPoint3 *p, int x_size, int y_size, int z_size, char *texture);

// box that has varying z-values.  Puts out a more complex brush than regular "box" function,
// but doesn't get screwed up when you put in varying z values.
//void box_complex(FILE *f, IntPoint3 *a, int height, char *texture);

typedef enum {
	CUBE_FACE_BOTTOM,
	CUBE_FACE_TOP,
	CUBE_FACE_LEFT,
	CUBE_FACE_RIGHT,
	CUBE_FACE_FRONT,
	CUBE_FACE_BACK
} CUBE_FACE;

void portal(FILE *f, IntPoint3 *a, int height, CUBE_FACE face);

// a pre-hollowed box is good for skyboxes and other containers
#define HOLLOW_DEFAULT 0
#define HOLLOW_TOPLESS 1
#define HOLLOW_BOTTOMLESS 2
void hollowbox(FILE *f, IntPoint3 *pt, int x_size, int y_size, int z_size, int thickness, char *texture, int options);

float interpolate(float a1, float b1, float a2, float b2, float a);

// returns array of points of interest
FloatPoint2 *boxes_find_overlay_points(FloatPoint2 *a_origin, float a_length, float a_height,
									   FloatPoint2 *b_origin, float b_length, float b_height, int *num_pts);

#endif
