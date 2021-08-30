#ifndef _REGION_MAKER_H
#define _REGION_MAKER_H

#include "region.h"
#include "map.h"

// returns null-terminated array of REGION pointers
REGION **get_regions(MAP *m, int num, int prune);

void destroy_regions(REGION **r);

#endif

