
#include <math.h>
#include <stdio.h>

#include "catenary.h"

#ifdef WIN32
#define asinh(a) asin(a)
#endif

static double catenary_test(double x1, double z1, double x2, double z2, double m, double k)
{
	double asinhm = 0.0;
	double ret = 0.0;	

	asinhm = asinh(m);
	ret = k * (cosh(asinhm) - cosh((x1 + k*asinhm - x2)/k)) + z1 - z2;

	return ret;
}

int catenary_find_equation(CATENARY *eq, double x1, double z1, double x2, double z2, double m)
{
	double min_t = 0.0;
	double max_t = 0.0;
	double mid_t = 0.0;
	double min_k = 1.0;
	double max_k = 1.0;
	double inc_k = 100.0;
	double mid_k = 0.0;
	double err = 1.0;
	double tol = 0.00000001;
	int min_sign = 0;
	int sign = 0;

	min_sign = (catenary_test(x1, z1, x2, z2, m, min_k) >= 0.0)? 1 : -1;

	// look for a right side boundary
	for (sign = min_sign; sign == min_sign; max_k += inc_k) {
		sign = (catenary_test(x1, z1, x2, z2, m, max_k) >= 0.0)? 1 : -1;
	}
	max_k -= inc_k;
	mid_k = (min_k + max_k) / 2;

	// binary search
	while (1) {

		min_t = catenary_test(x1, z1, x2, z2, m, min_k);
		mid_t = catenary_test(x1, z1, x2, z2, m, mid_k);
		max_t = catenary_test(x1, z1, x2, z2, m, max_k);

		// test
		err = (mid_t < 0.0) ? -1 * mid_t : mid_t;
		if (err <= tol) break;		

		// same sign checks
		if ((min_t > 0) == (mid_t > 0)) {
			min_k = mid_k;
		} else if ((max_t > 0) == (mid_t > 0)) {
			max_k = mid_k;
		}

		mid_k = (min_k + max_k) / 2;		
	}

	eq->k = mid_k;
	eq->c = eq->k * asinh(m) - x2;
	eq->d = z1 - eq->k * cosh((x1 + eq->c)/eq->k);

	return 0;
}

double catenary_eval(CATENARY *eq, double x)
{
	return eq->k * cosh((x + eq->c)/eq->k) + eq->d;
}

