
#include "primitives.h"

typedef struct {
	IntPoint3 p1;
	IntPoint3 p2;
	int valid;
} MAPINFO;

void mapfile_parse(char *filename, MAPINFO *mi);
