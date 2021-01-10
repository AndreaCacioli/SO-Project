#ifndef __GRID_Header__
#define __GRID_Header__
#include "taxi.h"
#define Boolean int

typedef struct grid
{
  int height;
  int width;
  Cell** grid;
}Grid;

#endif
