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
  pid_t pid;
}Taxi;

extern void printTaxi(Taxi t);
extern void sendMsgOnPipe(char* s, int fdWrite);
extern int move (Taxi* taxi,Grid* mappa,int semSetKey,int semStartKey,int SO_TIMEOUT, int semMutexKey);
extern void setDestination(Taxi* taxi, Cell c);
extern int initTaxi(Taxi* taxi,Grid* MAPPA, void (*signal_handler)(int), void (*die)(int), int semSetKey);
extern void findNearestSource(Taxi* taxi, Cell** sources, int entries);
extern int moveTo(Taxi* taxi, Grid* MAPPA,int semSetKey,int semMutexKey,int semStartKey, int Busy,int SO_TIMEOUT);
extern int dec_sem (int sem_id, int index);
extern int inc_sem(int sem_id, int index);
extern int taxiDie(Taxi t, int fdWrite, Grid grid, int sem_id, int semStartKey);
extern void compareTaxi(Taxi* compTaxi,int taxiNumber);


#endif
