#ifndef CATENARY_H
#define CATENARY_H

//
// Catenary curves are handy for modeling rope bridges, arches, and other natural and architectural objects
//
//   z = k * cosh((x + c)/k) + d
// 

typedef struct {
	double k;
	double c;
	double d;
} CATENARY;

// finds an equation of a catenary that connects points (x1,z1) and (x2,z2) with z2 >= z1, and a slope
// at point 2 of m.  Returns 0 on success, otherwise -1.
int catenary_find_equation(CATENARY *eq, double x1, double z1, double x2, double z2, double m);

// find z from x for a given catenary
double catenary_eval(CATENARY *eq, double x);

#endif // CATENARY_H

