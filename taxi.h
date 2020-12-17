#ifndef __TAXI_Header__
#define __TAXI_Header__
#include "cell.h"
#include "grid.h"
#define Boolean int



typedef struct taxi
{
  Cell position;
  Cell destination;
  Boolean busy;
  int TTD; /*Total Travelling Distance*/
  int TLT; /*Time of Longest Travel*/
  int totalTrips;
}Taxi;

extern void printTaxi(Taxi t);
extern void sendMsgOnPipe(char* s,int fdRead, int fdWrite);
extern int move (Taxi* taxi,Grid* mappa, int fdWrite);
extern void setDestination(Taxi* taxi, Cell c);

#endif
