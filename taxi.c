#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <math.h>
#include <time.h>
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

  taxi->position = MAPPA->grid[x][y]; /* TODO verifica che non serva un puntatore */
  taxi->busy = FALSE;
  printf("\n");
  taxi->destination = MAPPA->grid[0][0]; /* TODO inizializzare destination per farlo andare alla source */
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

void moveUp(Taxi* taxi, Grid* mappa)
{
  waitOnCell(taxi);
  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  printf("[%d]:(%d,%d)->(%d, %d)\n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x-1,taxi->position.y);
  taxi->position = mappa->grid[taxi->position.x-1][taxi->position.y];
  taxi->TTD++;
}
void moveDown(Taxi* taxi, Grid* mappa)
{

  waitOnCell(taxi);
  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  printf("[%d]:(%d,%d)->(%d, %d)\n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x+1,taxi->position.y);
  taxi->position = mappa->grid[taxi->position.x+1][taxi->position.y];
  taxi->TTD++;
}
void moveRight(Taxi* taxi, Grid* mappa)
{
  waitOnCell(taxi);
  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  printf("[%d]:(%d,%d)->(%d, %d)\n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x,taxi->position.y+1);
  taxi->position = mappa->grid[taxi->position.x][taxi->position.y+1];
  taxi->TTD++;
}
void moveLeft(Taxi* taxi, Grid* mappa)
{
  waitOnCell(taxi);
  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  printf("[%d]:(%d,%d)->(%d, %d)\n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x,taxi->position.y-1);
  taxi->position = mappa->grid[taxi->position.x][taxi->position.y-1];
  taxi->TTD++;
}



int move (Taxi* taxi, Grid* mappa) /*Returns 1 if taxi has arrived and 0 otherwise*/
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
        moveRight(taxi, mappa);
        moveDown(taxi, mappa);
        if(taxi->position.x + 1 < mappa->height) moveDown(taxi, mappa);
      }
      else
      {
        moveLeft(taxi, mappa);
        moveDown(taxi, mappa);
        if(taxi->position.x + 1 < mappa->height) moveDown(taxi, mappa);
      }
    }
    else moveDown(taxi, mappa);
  }

  else if(taxi->position.x - taxi->destination.x > 0)/*Upwnward direction*/
  {
    if(!mappa->grid[taxi->position.x-1][taxi->position.y].available)
    {
      /*Sopra c'é un buco!!!*/
      if(taxi->position.y - 1 < 0 && taxi->position.y < taxi->destination.y)/*É vietato andare a sinistra e la destinazione è "a destra"*/
      {
        moveRight(taxi, mappa);
        moveUp(taxi, mappa);
        if(taxi->position.x - 1 >= 0) moveUp(taxi, mappa);
      }
      else
      {
        moveLeft(taxi, mappa);
        moveUp(taxi, mappa);
        if(taxi->position.x - 1 >= 0) moveUp(taxi, mappa);
      }
    }
    else moveUp(taxi, mappa);
  }

  else if(taxi->position.y - taxi->destination.y < 0)/*Right direction*/
  {
    if(!mappa->grid[taxi->position.x][taxi->position.y+1].available)
    {
      if(taxi->position.x - 1 < 0 && taxi->position.x < taxi->destination.x)/*É vietato andare a su e la destinazione è "sotto"*/
      {
        moveDown(taxi, mappa);
        moveRight(taxi, mappa);
        if(taxi->position.y + 1 < mappa->width) moveRight(taxi, mappa);
      }
      else
      {
        moveUp(taxi, mappa);
        moveRight(taxi, mappa);
        if(taxi->position.y + 1 < mappa->width) moveRight(taxi, mappa);
      }
    }
    else moveRight(taxi, mappa);
  }
  else if(taxi->position.y - taxi->destination.y > 0)/*Left direction*/
  {
    if(!mappa->grid[taxi->position.x][taxi->position.y-1].available)
    {
      if(taxi->position.x - 1 < 0 && taxi->position.x < taxi->destination.x)/*É vietato andare a su e la destinazione è "sotto"*/
      {
        moveDown(taxi, mappa);
        moveLeft(taxi, mappa);
        if(taxi->position.y - 1 >= 0) moveLeft(taxi, mappa);
      }
      else
      {
        moveUp(taxi, mappa);
        moveLeft(taxi, mappa);
        if(taxi->position.y - 1 >= 0) moveLeft(taxi, mappa);
      }
    }
    else moveLeft(taxi, mappa);
  }
  return 0;
}

double dist(int x, int y, int x1, int y1)
{
  return sqrt((x1 - x)*(x1 - x) + (y1 - y)*(y1 - y));
}

void findNearestSource(Taxi* taxi, Cell* sources, int entries)
{
  int i = 0;
  Cell closest = sources[0];
  for(i = 1; i< entries; i++)
  {
    if(dist(taxi->position.x, taxi->position.y, sources[i].x, sources[i].y) < dist(taxi->position.x, taxi->position.y,closest.x, closest.y))
    {
      closest = sources[i];
    }
  }
  setDestination(taxi, closest);
}


void moveTo(Taxi* t, Grid* MAPPA)
{
  while(move(t,MAPPA) == 0)
  {
    printf("[%d] Position %d %d\n",getpid(),t->position.x,t->position.y);
  }
}
