#include <stdio.h>
#include "cell.h"

void printCell(Cell c)
{
  printf("CELL (%d,%d) %c %c Cap:%d Del:%d Cross:%d\t\t",c.x,c.y,c.available?'O':'X',c.source?'S':'-', c.capacity, c.delay, c.crossings);
}
