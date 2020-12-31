#ifndef __TAXI_Header__
#define __TAXI_Header__
#define _GNU_SOURCE
#include <time.h>
#include <sys/sem.h>
#include "cell.h"
#include "grid.h"
#define Boolean int

typedef struct taxi
{
  Cell position;
  Cell destination;
  Boolean busy;
  int TTD; /*Total Travelling Distance*/
  float TLT; /*Time of Longest Travel*/
  int totalTrips;
}Taxi;

extern void printTaxi(Taxi t);
extern void sendMsgOnPipe(char* s,int fdRead, int fdWrite);
extern int move (Taxi* taxi,Grid* mappa,int semSetKey);
extern void setDestination(Taxi* taxi, Cell c);
extern void initTaxi(Taxi* taxi,Grid* MAPPA, void (*signal_handler)(int));
extern void findNearestSource(Taxi* taxi, Cell** sources, int entries);
extern void moveTo(Taxi* taxi, Grid* MAPPA,int semSetKey, int Busy);
extern void dec_sem (int sem_id, int index);
extern void inc_sem(int sem_id, int index);
extern void taxiDie(Taxi t, int fdRead, int fdWrite);


#endif
