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
  Boolean taken;
  int crossings;

}Cell;

extern void printCell(Cell c, int semKey, Boolean compact);
extern int cellToSemNum(Cell c, int width);

#endif
