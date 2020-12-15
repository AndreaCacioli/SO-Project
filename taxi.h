#ifndef __TAXI_Header__
#define __TAXI_Header__
#include "cell.h"
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

#endif
