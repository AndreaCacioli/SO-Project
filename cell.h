#include "taxi.h"
#ifndef __CELL_Header__
#define __CELL_Header__
#define Boolean int

typedef struct cell
{
  int capacity;
  int delay;
  int x;
  int y;
  Boolean available;
  Boolean source;
  struct taxi* taxisOnCell;

}Cell;

extern void printCell(Cell c);

#endif
