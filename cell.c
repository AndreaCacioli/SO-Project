#include <stdio.h>
#include "cell.h"

void printCell(Cell c, int numtaxi, Boolean compact)
{
  if(compact){
    if(numtaxi>0)
    {
        printf("[%s%d] ", c.source ? " S T " : " T ",numtaxi);
        /*printf("[%s%d]\t", c.source ? " S " : " T ",numtaxi); no 'T' on source*/

    }
    else
        printf("[%s]\t", c.source ? " S " : (c.available ? " O " : " X ") );
  } 
  else printf("CELL (%d,%d) %c %c Cap:%d Del:%d Cross:%d %s\t\t",c.x,c.y,c.available?'O':'X',c.source?'S':'-', c.capacity, c.delay, c.crossings, c.taken?"TAKEN":"FREE");
}

int cellToSemNum(Cell c, int width)
{
  return c.x * width + c.y;
}
