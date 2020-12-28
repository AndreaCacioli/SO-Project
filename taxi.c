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
  printf("|TLT:          %d       |\n",t.TLT);
  printf("|Total Trips:  %d       |\n",t.totalTrips);
  printf("-------------------------\n");
}

void initTaxi(Taxi* taxi,Grid* MAPPA, void (*signal_handler)(int) )
{
  int x,y;
  struct sigaction SigHandler;
  srand(getpid()); /* Initializing the seed to pid*/
  do
  {
    x = (rand() % MAPPA->height);
    y = (rand() % MAPPA->width);
  } while(!MAPPA->grid[x][y].available);

  taxi->position = MAPPA->grid[x][y];
  /*taxi->position = MAPPA->grid[0][0]; *****************/
  taxi->busy = FALSE;
  taxi->destination = MAPPA->grid[0][0];
  taxi->TTD = 0;
  taxi->TLT = 0;
  taxi->totalTrips = 0;

  bzero(&SigHandler, sizeof(SigHandler));
  SigHandler.sa_handler = signal_handler;
  sigaction(SIGUSR1, &SigHandler, NULL);

 }

void sendMsgOnPipe(char* s, int fdRead, int fdWrite)
{
  close(fdRead);
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

void moveUp(Taxi* taxi, Grid* mappa,int semSetKey)
{
  waitOnCell(taxi);
  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  inc_sem(semSetKey,cellToSemNum(mappa->grid[taxi->position.x][taxi->position.y],mappa->width));
  printf("[%d]:(%d,%d)->(%d, %d)\n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x-1,taxi->position.y);
  dec_sem(semSetKey,cellToSemNum(mappa->grid[taxi->position.x-1][taxi->position.y-1],mappa->width), taxi, mappa);
  taxi->position = mappa->grid[taxi->position.x-1][taxi->position.y];
  taxi->TTD++;
}
void moveDown(Taxi* taxi, Grid* mappa,int semSetKey)
{
  waitOnCell(taxi);
  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  inc_sem(semSetKey,cellToSemNum(mappa->grid[taxi->position.x][taxi->position.y],mappa->width));
  printf("[%d]:(%d,%d)->(%d, %d)\n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x+1,taxi->position.y);
  dec_sem(semSetKey,cellToSemNum(mappa->grid[taxi->position.x+1][taxi->position.y],mappa->width), taxi, mappa);
  taxi->position = mappa->grid[taxi->position.x+1][taxi->position.y];
  taxi->TTD++;
}
void moveRight(Taxi* taxi, Grid* mappa,int semSetKey)
{
  waitOnCell(taxi);
  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  inc_sem(semSetKey,cellToSemNum(mappa->grid[taxi->position.x][taxi->position.y],mappa->width));
  printf("[%d]:(%d,%d)->(%d, %d)\n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x,taxi->position.y+1);
  dec_sem(semSetKey,cellToSemNum(mappa->grid[taxi->position.x][taxi->position.y+1],mappa->width), taxi, mappa);
  taxi->position = mappa->grid[taxi->position.x][taxi->position.y+1];
  taxi->TTD++;
}
void moveLeft(Taxi* taxi, Grid* mappa,int semSetKey)
{
  waitOnCell(taxi);
  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  inc_sem(semSetKey,cellToSemNum(mappa->grid[taxi->position.x][taxi->position.y],mappa->width));
  printf("[%d]:(%d,%d)->(%d, %d)\n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x,taxi->position.y-1);
  dec_sem(semSetKey, cellToSemNum(mappa->grid[taxi->position.x][taxi->position.y-1],mappa->width), taxi, mappa);
  taxi->position = mappa->grid[taxi->position.x][taxi->position.y-1];
  taxi->TTD++;
}



int move (Taxi* taxi, Grid* mappa, int semSetKey) /*Returns 1 if taxi has arrived and 0 otherwise*/
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
      if(taxi->position.y - 1 < 0 && taxi->position.y < taxi->destination.y)/*É vietato andare a sinistra e la destinazione è "a destra"*/
      {
        moveRight(taxi, mappa, semSetKey);
        moveDown(taxi, mappa, semSetKey);
        if(taxi->position.x + 1 < mappa->height) moveDown(taxi, mappa, semSetKey);
      }
      else
      {
        moveLeft(taxi, mappa, semSetKey);
        moveDown(taxi, mappa, semSetKey);
        if(taxi->position.x + 1 < mappa->height) moveDown(taxi, mappa, semSetKey);
      }
    }
    else moveDown(taxi, mappa, semSetKey);
  }

  else if(taxi->position.x - taxi->destination.x > 0)/*Upwnward direction*/
  {
    if(!mappa->grid[taxi->position.x-1][taxi->position.y].available)
    {
      /*Sopra c'é un buco!!!*/
      if(taxi->position.y - 1 < 0 && taxi->position.y < taxi->destination.y)/*É vietato andare a sinistra e la destinazione è "a destra"*/
      {
        moveRight(taxi, mappa,semSetKey);
        moveUp(taxi, mappa,semSetKey);
        if(taxi->position.x - 1 >= 0) moveUp(taxi, mappa, semSetKey);
      }
      else
      {
        moveLeft(taxi, mappa, semSetKey);
        moveUp(taxi, mappa, semSetKey);
        if(taxi->position.x - 1 >= 0) moveUp(taxi, mappa, semSetKey);
      }
    }
    else moveUp(taxi, mappa, semSetKey);
  }

  else if(taxi->position.y - taxi->destination.y < 0)/*Right direction*/
  {
    if(!mappa->grid[taxi->position.x][taxi->position.y+1].available)
    {
      if(taxi->position.x - 1 < 0 && taxi->position.x < taxi->destination.x)/*É vietato andare a su e la destinazione è "sotto"*/
      {
        moveDown(taxi, mappa, semSetKey);
        moveRight(taxi, mappa, semSetKey);
        if(taxi->position.y + 1 < mappa->width) moveRight(taxi, mappa, semSetKey);
      }
      else
      {
        moveUp(taxi, mappa, semSetKey);
        moveRight(taxi, mappa, semSetKey);
        if(taxi->position.y + 1 < mappa->width) moveRight(taxi, mappa, semSetKey);
      }
    }
    else moveRight(taxi, mappa, semSetKey);
  }
  else if(taxi->position.y - taxi->destination.y > 0)/*Left direction*/
  {
    if(!mappa->grid[taxi->position.x][taxi->position.y-1].available)
    {
      if(taxi->position.x - 1 < 0 && taxi->position.x < taxi->destination.x)/*É vietato andare a su e la destinazione è "sotto"*/
      {
        moveDown(taxi, mappa, semSetKey);
        moveLeft(taxi, mappa, semSetKey);
        if(taxi->position.y - 1 >= 0) moveLeft(taxi, mappa, semSetKey);
      }
      else
      {
        moveUp(taxi, mappa, semSetKey);
        moveLeft(taxi, mappa, semSetKey);
        if(taxi->position.y - 1 >= 0) moveLeft(taxi, mappa, semSetKey);
      }
    }
    else moveLeft(taxi, mappa, semSetKey);
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

void findNearestSource(Taxi* taxi, Cell* sources, int entries)
{
  int i = 0;
  Cell closest;
  closest.x = INT_MIN;
  closest.y = INT_MIN;
  /*Init to a non-existing Cell*/
  do
  {
    if(taxi->position.x == sources[i].x && taxi->position.y == sources[i].y)
    {
      continue; /*We don't consider the cell we are on top of!*/
      i++;
    }
    else if(dist(taxi->position.x, taxi->position.y, sources[i].x, sources[i].y) < dist(taxi->position.x, taxi->position.y,closest.x, closest.y))
    {
      /*printf("Taxi(%d,%d)\n",taxi->position.x, taxi->position.y);
      printf("Sorgente(%d,%d) i=%d\n",sources[i].x, sources[i].y,i);*/
      closest.x = sources[i].x;
      closest.y = sources[i].y;
    }
    i++;
  }while(i < entries);
  setDestination(taxi, closest);
}


void moveTo(Taxi* t, Grid* MAPPA,int semSetKey)
{
  while(move(t,MAPPA,semSetKey) == 0);
}


void dec_sem (int sem_id, int index, Taxi* taxi, Grid* mappa)
{
    struct sembuf sem_op;
    /*mappa->grid[taxi->position.x][taxi->position.y].capacity = mappa->grid[taxi->position.x][taxi->position.y].capacity-1; Capacity of the cell doesn't change*/
    if(semctl(sem_id, /*semnum=*/index, GETVAL) - 1 < 0) /*The -1 is because we are about to decrease the semaphore*/
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
