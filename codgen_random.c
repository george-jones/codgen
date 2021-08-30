#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codgen_random.h"

#ifdef WIN32
#define strcasecmp strcmpi
#endif


// beging magic number defines  // * stolen from ansi standard *//

#define IA 1103515245
#define IC 12345
#define IM 65536
#define GENRAND_MAX 32768

// end magic number defines  

// global seed
unsigned long randseed=1;

void genrand_seed(char *map_string) 
{
	unsigned long seed = 1;
	unsigned long mult = 1;
	unsigned long m = 7; // with repeated multiplication, this will not turn into zero after crossing 32 bit boundary

	if (!map_string) {
		map_string = "the_coolest_map_in_the_world";
	}	

	while (*map_string) {
		seed += (unsigned long)(*map_string * mult);
		mult *= m;
		map_string++;
	} 

	randseed = seed % GENRAND_MAX;
	fprintf(stderr, "Random seed (based on map name): %u\r\n", randseed);	

	return;
}

void genrand_srand(unsigned int seed) 
{
	 randseed=seed;
}

static int Myrand()  // this is a fairly standard way to generate numbers
{
	randseed = randseed*IA + IC;
	return (unsigned int)(randseed/IM) % GENRAND_MAX;
}

int genrand(int max_random)
{
	int ret = 0;
	double base = 1.0;

	if (max_random < 0) {
		max_random = BASE_RANDOM_MAX;
	}

	base = (max_random * base);

	//ret = (Myrand() % max_random) + 0;
	ret = (int) (base*Myrand()/(GENRAND_MAX+1.0));	

	return ret;
}


int randrange(int min, int max)
{
	return min + (genrand(MAX_GEN_RANDOM) % (max-min)); 
}

DOMNODE *pick_child_random(DOMNODE *p)
{
	DOMNODE *n = NULL;
	float *vals = NULL;
	float c = 0.0;
	float v = 0.0;
	float sum = 0.0;
	float c_sum = 0.0;
	int items = 0;
	int i = 0;
	
	// count nodes, and sum "chances"
	for (i=0; (n = p->children[i]) != NULL; i++) {
		v = XMLDomGetAttributeFloat(n, "chance", 1.0);
		sum += v;
		items++;
	}

	if (items <= 1) return p->children[0];

	// get the values
	vals = (float *)malloc(sizeof(float) * items);
	if (vals) {
		for (i=0; i<items; i++) {
			n = p->children[i];
			vals[i] = XMLDomGetAttributeFloat(n, "chance", 1.0);
		}
	} else {
		return NULL;
	}

	c = sum * (float)genrand(10000) / 10000.0;
	c_sum = 0.0;
	for (i=0; i<items; i++) {
		n = p->children[i];
		c_sum += XMLDomGetAttributeFloat(n, "chance", 1.0);
		if (c_sum > c) break;
	}

	free(vals);

	return n;
}

float random_normal(float mean, float stdev)
{
	// uses the polar form of the Box-Muller transformation
	float x1, x2, w, y1, y2;
 
	do {
		x1 = 2.0 * ((float)Myrand() / (float)GENRAND_MAX) - 1.0;
		x2 = 2.0 * ((float)Myrand() / (float)GENRAND_MAX) - 1.0;
		w = x1 * x1 + x2 * x2;
	} while ( w >= 1.0 );

	w = sqrt( (-2.0 * log( w ) ) / w );
	y1 = x1 * w;
	y2 = x2 * w;

	// this method always generates two numbers.  We'll just pick the first one (y1)

	y1 = mean + stdev * y1;

	return y1;
}
