#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
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

void initTaxi(Taxi* taxi,Grid* MAPPA)
{
  int x,y;
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

void moveUp(Taxi* taxi, Grid* mappa, int fdWrite)
{
  char message[500] = "";
  waitOnCell(taxi);
  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  sprintf(message, "[%d]:(%d,%d)->(%d, %d)\n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x-1,taxi->position.y);
  taxi->position = mappa->grid[taxi->position.x-1][taxi->position.y];
  write(fdWrite, message  ,strlen(message) * sizeof(char));
  taxi->TTD++;
}
void moveDown(Taxi* taxi, Grid* mappa, int fdWrite)
{
  char message[500] = "";
  waitOnCell(taxi);
  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  sprintf(message, "[%d]:(%d,%d)->(%d, %d)\n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x+1,taxi->position.y);
  taxi->position = mappa->grid[taxi->position.x+1][taxi->position.y];
  write(fdWrite, message  ,strlen(message) * sizeof(char));
  taxi->TTD++;
}
void moveRight(Taxi* taxi, Grid* mappa, int fdWrite)
{
  char message[500] = "";
  waitOnCell(taxi);
  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  sprintf(message, "[%d]:(%d,%d)->(%d, %d)\n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x,taxi->position.y+1);
  taxi->position = mappa->grid[taxi->position.x][taxi->position.y+1];
  write(fdWrite, message  ,strlen(message) * sizeof(char));
  taxi->TTD++;
}
void moveLeft(Taxi* taxi, Grid* mappa, int fdWrite)
{
  char message[500] = "";
  waitOnCell(taxi);
  mappa->grid[taxi->position.x][taxi->position.y].crossings++;
  sprintf(message, "[%d]:(%d,%d)->(%d, %d)\n",getpid(),taxi->position.x,taxi->position.y,taxi->position.x,taxi->position.y-1);
  taxi->position = mappa->grid[taxi->position.x][taxi->position.y-1];
  write(fdWrite, message  ,strlen(message) * sizeof(char));
  taxi->TTD++;
}



int move (Taxi* taxi, Grid* mappa, int fdWrite) /*Returns 1 if taxi has arrived and 0 otherwise*/
/*Implements Aldo's L rule*/
{
  char message[500] = "";

  if(taxi->position.x == taxi->destination.x && taxi->position.y == taxi->destination.y)
  {
    sprintf(message, "Il taxi con pid: %d é arrivato a destinazione\n",getpid());
    write(fdWrite, message  ,strlen(message) * sizeof(char));
    return 1;
  }

  if(taxi->position.x - taxi->destination.x < 0)/*Downward direction*/
  {
    if(!mappa->grid[taxi->position.x+1][taxi->position.y].available)
    {
      /*Sotto c'é un buco!!!*/
      if(taxi->position.y - 1 < 0)/*É vietato andare a sinistra?*/
      {
        moveRight(taxi, mappa, fdWrite);
        moveDown(taxi, mappa, fdWrite);
        if(taxi->position.x + 1 < mappa->height) moveDown(taxi, mappa, fdWrite);
      }
      else
      {
        moveLeft(taxi, mappa, fdWrite);
        moveDown(taxi, mappa, fdWrite);
        if(taxi->position.x + 1 < mappa->height) moveDown(taxi, mappa, fdWrite);
      }
    }
    else moveDown(taxi, mappa, fdWrite);
  }

  else if(taxi->position.x - taxi->destination.x > 0)/*Upwnward direction*/
  {
    if(!mappa->grid[taxi->position.x-1][taxi->position.y].available)
    {
      /*Sopra c'é un buco!!!*/
      if(taxi->position.y - 1 < 0)/*É vietato andare a sinistra?*/
      {
        moveRight(taxi, mappa, fdWrite);
        moveUp(taxi, mappa, fdWrite);
        if(taxi->position.x - 1 >= 0) moveUp(taxi, mappa, fdWrite);
      }
      else
      {
        moveLeft(taxi, mappa, fdWrite);
        moveUp(taxi, mappa, fdWrite);
        if(taxi->position.x - 1 >= 0) moveUp(taxi, mappa, fdWrite);
      }
    }
    else moveUp(taxi, mappa, fdWrite);
  }

  else if(taxi->position.y - taxi->destination.y < 0)/*Right direction*/
  {
    if(!mappa->grid[taxi->position.x][taxi->position.y+1].available)
    {
      if(taxi->position.x - 1 < 0)/*É vietato andare a su?*/
      {
        moveDown(taxi, mappa, fdWrite);
        moveRight(taxi, mappa, fdWrite);
        if(taxi->position.y + 1 < mappa->width) moveRight(taxi, mappa, fdWrite);
      }
      else
      {
        moveUp(taxi, mappa, fdWrite);
        moveRight(taxi, mappa, fdWrite);
        if(taxi->position.y + 1 < mappa->width) moveRight(taxi, mappa, fdWrite);
      }
    }
    else moveRight(taxi, mappa, fdWrite);
  }
  else if(taxi->position.y - taxi->destination.y > 0)/*Left direction*/
  {
    if(!mappa->grid[taxi->position.x][taxi->position.y-1].available)
    {
      if(taxi->position.x - 1 < 0)/*É vietato andare a su?*/
      {
        moveDown(taxi, mappa, fdWrite);
        moveLeft(taxi, mappa, fdWrite);
        if(taxi->position.y - 1 >= 0) moveLeft(taxi, mappa, fdWrite);
      }
      else
      {
        moveUp(taxi, mappa, fdWrite);
        moveLeft(taxi, mappa, fdWrite);
        if(taxi->position.y - 1 >= 0) moveLeft(taxi, mappa, fdWrite);
      }
    }
    else moveLeft(taxi, mappa, fdWrite);
  }
  return 0;
}
