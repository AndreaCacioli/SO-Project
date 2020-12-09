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
