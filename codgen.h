#ifndef _CODGEN_H
#define _CODGEN_H

#include <stdio.h>

#include "terrain.h"
#include "map.h"

#define DBG 0

float map_height_at_point(MAP *m, float x, float y, int *on_a_rock);

#endif
