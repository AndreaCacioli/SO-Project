#include "taxi.h"
#ifndef __GRID_Header__
#define __GRID_Header__
#define Boolean int

typedef struct grid
{
  int height;
  int width;
  Cell** grid;
}Grid;

#endif
