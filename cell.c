#include <stdio.h>
#include "cell.h"

void printCell(Cell c)
{
  printf("CELL (%d,%d) %c %c\t",c.x,c.y,c.available?'O':'X',c.source?'S':'-');
}
