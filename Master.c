#include <stdio.h>
#include <stdlib.h>
#include "taxi.h"
#include "cell.h"
#include "mapgenerator.h"
#define FALSE 0
#define TRUE !FALSE

int main(void)
{
  Taxi t;
  Cell** c;
  printf("Allocating c\n");
  c = malloc(8);
  printf("Allocating c[0]\n");
  c[0] = calloc(5, sizeof(Cell));
  printf("Allocating c[1]\n");
  c[1] = calloc(2, sizeof(Cell));
  printf("Allocation done, now printing\n");
  printCell(c[0][1]);
  printf("\n");

  t.position.x = 1;
  t.position.y = 4;
  t.destination.x = 5;
  t.destination.y = 3;
  t.busy = TRUE;
  t.TTD = 45;
  t.TLT = 78;
  t.totalTrips = 10;

  printTaxi(t);
  generateMap(3,3);



  system("ipcs");

  return 0;
}
