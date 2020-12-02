#include <stdio.h>
#include "taxi.h"

void printTaxi(Taxi t)
{
  printf("-----------Taxi----------\n");
  printf("|Position:     (%d,%d)    |\n",t.position.x,t.position.y);
  printf("|Destination:  (%d,%d)    |\n",t.destination.x,t.destination.y);
  printf("|Busy:         %s     |\n", t.busy ? "TRUE" : "FALSE");
  printf("|TTD:          %d       |\n",t.TTD);
  printf("|TLT:          %d       |\n",t.TLT);
  printf("|Total Trips:  %d       |\n",t.totalTrips);
  printf("-------------------------\n");
}
