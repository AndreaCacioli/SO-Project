#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/sem.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include "taxi.h"
#include "grid.h"
#define FALSE 0
#define TRUE 1

void printTaxi(Taxi t)
{
  printf("---------Taxi--%d------\n",getpid());
  printf("|Position:     (%d,%d)    |\n",t.position.x,t.position.y);
  printf("|Destination:  (%d,%d)    |\n",t.destination.x,t.destination.y);
  printf("|Busy:         %s     |\n", t.busy ? "TRUE" : "FALSE");
  printf("|TTD:          %d       |\n",t.TTD);
  printf("|TLT:          %f       |\n",t.TLT);
  printf("|Total Trips:  %d       |\n",t.totalTrips);
  printf("-------------------------\n");
}
void printTaxiWithGivenPid(Taxi t, pid_t pid)
{
  printf("---------Taxi--%d------\n",pid);
  printf("|Position:     (%d,%d)    |\n",t.position.x,t.position.y);
  printf("|Destination:  (%d,%d)    |\n",t.destination.x,t.destination.y);
  printf("|Busy:         %s     |\n", t.busy ? "TRUE" : "FALSE");
  printf("|TTD:          %d       |\n",t.TTD);
  printf("|TLT:          %f       |\n",t.TLT);
  printf("|Total Trips:  %d       |\n",t.totalTrips);
  printf("-------------------------\n");
}

void initTaxi(Taxi* taxi,Grid* MAPPA, void (*signal_handler)(int), void (*die)(int), int semSetKey)
{
  int x,y;
  struct sigaction SigHandler;
  srand(getpid()); /* Initializing the seed to pid*/
  do
  {
    x = (rand() % MAPPA->height);
    y = (rand() % MAPPA->width);
  } while(!MAPPA->grid[x][y].available || semctl(semSetKey,cellToSemNum(MAPPA->grid[x][y],MAPPA->width), GETVAL) <= 0);

  dec_sem(semSetKey, cellToSemNum(MAPPA->grid[x][y],MAPPA->width));
  taxi->position = MAPPA->grid[x][y];
  taxi->busy = FALSE;
  taxi->destination = MAPPA->grid[0][0];
  taxi->TTD = 0;
  taxi->TLT = 0;
  taxi->totalTrips = 0;

  bzero(&SigHandler, sizeof(SigHandler));
  SigHandler.sa_handler = signal_handler;
  sigaction(SIGUSR1, &SigHandler, NULL);
  SigHandler.sa_handler = die;
  sigaction(SIGINT, &SigHandler, NULL);

 }


void sendMsgOnPipe(char* s, int fdWrite)
{
  write(fdWrite, s ,strlen(s) * sizeof(char));
}

void setDestination(Taxi* taxi, Cell c)
{
  taxi->destination = c;
}

void waitOnCell(Taxi* taxi)
{
  struct timespec waitTime;
  waitTime.tv_sec = 0;
  waitTime.tv_nsec = taxi->position.delay;
  nanosleep(&waitTime,NULL);
}

void moveUp(Taxi* taxi, Grid* mappa,int semSetKey, int SO_TIMEOUT)
{
  clock_t Timestart,Timemove; 
  Timestart = clock();
  waitOnCell(taxi);
  Timemove = clock();
  if((Timemove-Timestart)>=SO_TIMEOUT){
    printf("\n*** [%d] DIE***\n\n",getpid());
    exit(EXIT_FAILURE);
  }

  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  inc_sem(semSetKey,cellToSemNum(mappa->grid[taxi->position.x][taxi->position.y],mappa->width));
  printf("[%d]:(%d,%d)->(%d, %d) UP \n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x-1,taxi->position.y);
  dec_sem(semSetKey,cellToSemNum(mappa->grid[taxi->position.x-1][taxi->position.y],mappa->width));
  taxi->position = mappa->grid[taxi->position.x-1][taxi->position.y];
  taxi->TTD++;
}
void moveDown(Taxi* taxi, Grid* mappa,int semSetKey, int SO_TIMEOUT)
{

  clock_t Timestart,Timemove; 
  Timestart = clock();
  waitOnCell(taxi);
  Timemove = clock();
  if((Timemove-Timestart)>=SO_TIMEOUT){
    printf("\n*** [%d] DIE***\n\n",getpid());
    exit(EXIT_FAILURE);
  }

  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  inc_sem(semSetKey,cellToSemNum(mappa->grid[taxi->position.x][taxi->position.y],mappa->width));
  printf("[%d]:(%d,%d)->(%d, %d) DOWN \n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x+1,taxi->position.y);
  dec_sem(semSetKey,cellToSemNum(mappa->grid[taxi->position.x+1][taxi->position.y],mappa->width));
  taxi->position = mappa->grid[taxi->position.x+1][taxi->position.y];
  taxi->TTD++;
}
void moveRight(Taxi* taxi, Grid* mappa,int semSetKey, int SO_TIMEOUT)
{
  clock_t Timestart,Timemove; 
  Timestart = clock();
  waitOnCell(taxi);
  Timemove = clock();
  if((Timemove-Timestart)>=SO_TIMEOUT){
    printf("\n*** [%d] DIE***\n\n",getpid());
    exit(EXIT_FAILURE);
  }

  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  inc_sem(semSetKey,cellToSemNum(mappa->grid[taxi->position.x][taxi->position.y],mappa->width));
  printf("[%d]:(%d,%d)->(%d, %d) RIGHT \n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x,taxi->position.y+1);
  dec_sem(semSetKey,cellToSemNum(mappa->grid[taxi->position.x][taxi->position.y+1],mappa->width));
  taxi->position = mappa->grid[taxi->position.x][taxi->position.y+1];
  taxi->TTD++;
}
void moveLeft(Taxi* taxi, Grid* mappa,int semSetKey, int SO_TIMEOUT)
{
  clock_t Timestart,Timemove; 
  Timestart = clock();
  waitOnCell(taxi);
  Timemove = clock();
  if((Timemove-Timestart)>=SO_TIMEOUT){
    printf("\n*** [%d] DIE***\n\n",getpid());
    exit(EXIT_FAILURE);
  }

  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  inc_sem(semSetKey,cellToSemNum(mappa->grid[taxi->position.x][taxi->position.y],mappa->width));
  printf("[%d]:(%d,%d)->(%d, %d) LEFT \n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x,taxi->position.y-1);
  dec_sem(semSetKey, cellToSemNum(mappa->grid[taxi->position.x][taxi->position.y-1],mappa->width));
  taxi->position = mappa->grid[taxi->position.x][taxi->position.y-1];
  taxi->TTD++;
}



int move (Taxi* taxi, Grid* mappa, int semSetKey,int SO_TIMEOUT) /*Returns 1 if taxi has arrived and 0 otherwise*/
/*Implements Aldo's L rule*/
{
  if(taxi->position.x == taxi->destination.x && taxi->position.y == taxi->destination.y)
  {
    return 1;
  }

  if(taxi->position.x - taxi->destination.x < 0)/*Downward direction*/
  {
    if(!mappa->grid[taxi->position.x+1][taxi->position.y].available)
    {
      /*Sotto c'é un buco!!!*/
      if(taxi->position.y - 1 < 0 && taxi->position.y <= taxi->destination.y)/*É vietato andare a sinistra e la destinazione è "a destra"*/
      {
        moveRight(taxi, mappa, semSetKey, SO_TIMEOUT);
        moveDown(taxi, mappa, semSetKey, SO_TIMEOUT);
        if(taxi->position.x + 1 < mappa->height) moveDown(taxi, mappa, semSetKey, SO_TIMEOUT);
      }
      else
      {
        moveLeft(taxi, mappa, semSetKey, SO_TIMEOUT);
        moveDown(taxi, mappa, semSetKey, SO_TIMEOUT);
        if(taxi->position.x + 1 < mappa->height) moveDown(taxi, mappa, semSetKey, SO_TIMEOUT);
      }
    }
    else moveDown(taxi, mappa, semSetKey, SO_TIMEOUT);
  }

  else if(taxi->position.x - taxi->destination.x > 0)/*Upwnward direction*/
  {
    if(!mappa->grid[taxi->position.x-1][taxi->position.y].available)
    {
      /*Sopra c'é un buco!!!*/
      if(taxi->position.y - 1 < 0 && taxi->position.y <= taxi->destination.y)/*É vietato andare a sinistra e la destinazione è "a destra"*/
      {
        moveRight(taxi, mappa,semSetKey, SO_TIMEOUT);
        moveUp(taxi, mappa,semSetKey, SO_TIMEOUT);
        if(taxi->position.x - 1 >= 0) moveUp(taxi, mappa, semSetKey, SO_TIMEOUT);
      }
      else
      {
        moveLeft(taxi, mappa, semSetKey, SO_TIMEOUT);
        moveUp(taxi, mappa, semSetKey, SO_TIMEOUT);
        if(taxi->position.x - 1 >= 0) moveUp(taxi, mappa, semSetKey, SO_TIMEOUT);
      }
    }
    else moveUp(taxi, mappa, semSetKey, SO_TIMEOUT);
  }

  else if(taxi->position.y - taxi->destination.y < 0)/*Right direction*/
  {
    if(!mappa->grid[taxi->position.x][taxi->position.y+1].available)
    {
      if(taxi->position.x - 1 < 0 && taxi->position.x <= taxi->destination.x)/*É vietato andare a su e la destinazione è "sotto"*/
      {
        moveDown(taxi, mappa, semSetKey, SO_TIMEOUT);
        moveRight(taxi, mappa, semSetKey, SO_TIMEOUT);
        if(taxi->position.y + 1 < mappa->width) moveRight(taxi, mappa, semSetKey, SO_TIMEOUT);
      }
      else
      {
        moveUp(taxi, mappa, semSetKey, SO_TIMEOUT);
        moveRight(taxi, mappa, semSetKey, SO_TIMEOUT);
        if(taxi->position.y + 1 < mappa->width) moveRight(taxi, mappa, semSetKey, SO_TIMEOUT);
      }
    }
    else moveRight(taxi, mappa, semSetKey, SO_TIMEOUT);
  }
  else if(taxi->position.y - taxi->destination.y > 0)/*Left direction*/
  {
    if(!mappa->grid[taxi->position.x][taxi->position.y-1].available)
    {
      if(taxi->position.x - 1 < 0 && taxi->position.x <= taxi->destination.x)/*É vietato andare a su e la destinazione è "sotto"*/
      {
        moveDown(taxi, mappa, semSetKey, SO_TIMEOUT);
        moveLeft(taxi, mappa, semSetKey, SO_TIMEOUT);
        if(taxi->position.y - 1 >= 0) moveLeft(taxi, mappa, semSetKey, SO_TIMEOUT);
      }
      else
      {
        moveUp(taxi, mappa, semSetKey, SO_TIMEOUT);
        moveLeft(taxi, mappa, semSetKey, SO_TIMEOUT);
        if(taxi->position.y - 1 >= 0) moveLeft(taxi, mappa, semSetKey, SO_TIMEOUT);
      }
    }
    else moveLeft(taxi, mappa, semSetKey, SO_TIMEOUT);
  }
  return 0;
}

double dist(int x, int y, int x1, int y1)
{
  if(x1<0 || y1<0)
      return INT_MAX;
  else
      return sqrt((x1 - x)*(x1 - x) + (y1 - y)*(y1 - y));
}

void findNearestSource(Taxi* taxi, Cell** sources, int entries)
{
  int i = 0;
  Cell closest;
  closest.x = INT_MIN;
  closest.y = INT_MIN;
  /*Init to a non-existing Cell*/
  do
  {
    if(taxi->position.x == sources[i]->x && taxi->position.y == sources[i]->y)
    {
      i++;
      continue; /*We don't consider the cell we are on top of!*/
    }
    else if(dist(taxi->position.x, taxi->position.y, sources[i]->x, sources[i]->y) < dist(taxi->position.x, taxi->position.y,closest.x, closest.y))
    {
      closest.x = sources[i]->x;
      closest.y = sources[i]->y;
    }
    i++;
  }while(i < entries);
  setDestination(taxi, closest);
}


void moveTo(Taxi* t, Grid* MAPPA,int semSetKey, int Busy, int SO_TIMEOUT)
{
  clock_t x_startTime,x_endTime;
  float seconds=0;

  if(Busy==TRUE){ /* counting TLT */
    x_startTime = clock();
    while(move(t,MAPPA,semSetKey,SO_TIMEOUT) == 0);
    x_endTime = clock();

    seconds = (float)(x_endTime - x_startTime) / CLOCKS_PER_SEC;

    if(seconds>t->TLT){
        t->TLT=seconds;
    }
    t->totalTrips++;
  }
  else
      while(move(t,MAPPA,semSetKey,SO_TIMEOUT) == 0);

}


void dec_sem (int sem_id, int index)
{
    struct sembuf sem_op;
    if(semctl(sem_id, /*semnum=*/index, GETVAL) == 0)
    {
      printf("Taxi [%d] sleeping\n",getpid());
    }
    sem_op.sem_num  = index;
    sem_op.sem_op   = -1;
    sem_op.sem_flg = 0;
    semop(sem_id, &sem_op, 1);

}

void inc_sem(int sem_id, int index)
{
    struct sembuf sem_op;
    sem_op.sem_num  = index;
    sem_op.sem_op   = 1;
    sem_op.sem_flg = 0;
    semop(sem_id, &sem_op, 1);
}
void taxiDie(Taxi taxi, int fdWrite, Grid grid, int sem_id)
{
  char* message = malloc(500);
  inc_sem(sem_id, cellToSemNum(taxi.position, grid.width));
  sprintf(message, "%d %d %d %d %d %d %d %f %d\n", getpid(), taxi.position.x, taxi.position.y, taxi.destination.x , taxi.destination.y, taxi.busy, taxi.TTD, taxi.TLT, taxi.totalTrips); /*The \n is the message terminator*/
  sendMsgOnPipe(message,fdWrite);
  free(message);
  close(fdWrite);
  exit(EXIT_SUCCESS);
}
