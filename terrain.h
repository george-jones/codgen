
#ifndef TERRAINTULA
#define TERRAINTULA

typedef struct {
	float *points;
	int x_res;
	int y_res;
} TERRAIN;

// generate an interesting terrain.
// smoothness ranges from 0 (no smoothing) to infinity (smoothed into oblivion)
// values between 0.2 and 0.8 seem reasonable.
TERRAIN *terrain_generate(int x_res, int y_res, float smoothness);

// create a flat terrain.  You probably wouldn't want that.  Just use a big box for that.
TERRAIN *terrain_create(int x_res, int y_res);

// duplicate a terrain
TERRAIN *terrain_copy(TERRAIN *t);

// free memory
void terrain_destroy(TERRAIN *t);

// set all points to 0.0
void terrain_flatten(TERRAIN *t);

// my first attempt at random generation
void terrain_randomize(TERRAIN *t);

// soften the terrain
void terrain_blur(TERRAIN *t);

// soften just a section of the terrain
void terrain_blur_area(TERRAIN *t, int x1, int y1, int x2, int y2);

// read a point from the terrain
float terrain_get_point(TERRAIN *t, int x, int y);

// set point on terrain
void terrain_set_point(TERRAIN *t, int x, int y, float z);

// maximize amplitude to 0 up to 1.0.
// This makes the terrain less boring (which happens after terrain_blur'ing several times)
void terrain_amplify(TERRAIN *t);

// multiply height of all points on the terrain by a certain value
void terrain_multiply(TERRAIN *t, float v);

// get the (absolute) value of the largest single point-to-point horizontal slope on the terrain.  Since
// a terrain consists of an orderly grid, there is no possibility for infinite slope (vertical lines)
float terrain_find_max_x_slope(TERRAIN *t);

// get the (absolute) value of the largest single point-to-point vertical slope on the terrain.  Since
// a terrain consists of an orderly grid, there is no possibility for infinite slope (vertical lines)
float terrain_find_max_y_slope(TERRAIN *t);

// my best guess as what the height at a point in the terrain is, given the passed scaling factors
float terrain_height_at_point(TERRAIN *t, float x, float y, int x_size, int y_size, int terrain_height);

#endif

