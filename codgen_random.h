#ifndef __COD2GEN_RANDOM
#define __COD2GEN_RANDOM

#include "xml.h"

#ifndef BASE_RANDOM_MAX
#define BASE_RANDOM_MAX 9999997;
#endif

#define MAX_GEN_RANDOM 10000

void genrand_seed(char *map_string);
void genrand_srand(unsigned int seed);

int genrand(int max_random);

int randrange(int min, int max);

// pick a child node from a DOM element randomly
DOMNODE *pick_child_random(DOMNODE *p);

// get a random number, with a gaussian distribution, and given mean and standard deviation
float random_normal(float mean, float stdev);

#endif //__COD2GEN_RANDOM					
