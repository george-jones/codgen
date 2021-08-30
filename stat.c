#include <math.h>
#include <stdio.h>

#include "codgen_random.h"

//  See http://www.megspace.com/science/sfe/i_ot_std.html for info on calculating standard deviation

// like a c++ template, only dirtier and not on-demand
#define stdev_func(T) \
float stdev_ ## T (T *vals, int num) \
{\
	double d = 0.0; \
	double n = 0.0; \
	double m = 0.0; \
	double ssd = 0.0; \
	int i = 0; \
	if (num>=2) { \
		for (i=0; i<num-1; i++) { \
			d = ((double)vals[i+1] - m) / (n + 1); \
			ssd += n*d*d; \
			m += d; \
			n += 1.0; \
			ssd += pow((double)vals[i] - m, 2.0); \
		} \
		n += 1.0;\
		ssd /= (n-1); \
		return (float)sqrt(ssd); \
	} else { \
		return 0.0; \
	} \
}

stdev_func(int) // generates stdev_int

stdev_func(float) // generates stdev_float

