#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/sem.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include "taxi.h"
#include "grid.h"
#define FALSE 0
#define TRUE 1

void printTaxi(Taxi t)
{
  printf("---------Taxi--%d------\n",t.pid);
  printf("|Position:     (%d,%d)    |\n",t.position.x,t.position.y);
  printf("|Destination:  (%d,%d)    |\n",t.destination.x,t.destination.y);
  printf("|Busy:         %s     |\n", t.busy ? "TRUE" : "FALSE");
  printf("|TTD:          %d       |\n",t.TTD);
  printf("|TLT:          %f       |\n",t.TLT);
  printf("|Total Trips:  %d       |\n",t.totalTrips);
  printf("-------------------------\n");
}

int initTaxi(Taxi* taxi,Grid* MAPPA, void (*signal_handler)(int), void (*die)(int), int semSetKey)
{
  int x,y;
  struct sigaction SigHandler;
  srand(getpid()); /* Initializing the seed to pid*/
  do
  {
    x = (rand() % MAPPA->height);
    y = (rand() % MAPPA->width);
  } while(!MAPPA->grid[x][y].available || semctl(semSetKey,cellToSemNum(MAPPA->grid[x][y],MAPPA->width), GETVAL) <= 0);

  /*printf("My semaphore before: %d\n",semctl(semSetKey, cellToSemNum(MAPPA->grid[x][y],MAPPA->width), GETVAL));*/
  if((dec_sem(semSetKey, cellToSemNum(MAPPA->grid[x][y],MAPPA->width)))==-1){
    fprintf(stderr,"Error %s:%d: in initTaxi %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  /*printf("My semaphore after: %d\n",semctl(semSetKey, cellToSemNum(MAPPA->grid[x][y],MAPPA->width), GETVAL));*/
  taxi->position = MAPPA->grid[x][y];
  taxi->busy = FALSE;
  taxi->destination = MAPPA->grid[0][0];
  taxi->TTD = 0;
  taxi->TLT = 0;
  taxi->totalTrips = 0;
  taxi->pid = getpid();

  bzero(&SigHandler, sizeof(SigHandler));
  SigHandler.sa_handler = signal_handler;
  if((sigaction(SIGUSR1, &SigHandler, NULL))==-1){
    fprintf(stderr,"Error %s:%d: in initTaxi %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  SigHandler.sa_handler = die;
  if((sigaction(SIGINT, &SigHandler, NULL))==-1){
    fprintf(stderr,"Error %s:%d: in initTaxi %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  if((sigaction(SIGALRM, &SigHandler, NULL))==-1){
    fprintf(stderr,"Error %s:%d: in initTaxi %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }

  return 0;
 }


void sendMsgOnPipe(char* s, int fdWrite)
{
  write(fdWrite, s ,strlen(s) * sizeof(char));
}

void setDestination(Taxi* taxi, Cell c)
{
  taxi->destination = c;
}

int waitOnCell(Taxi* taxi)
{
  struct timespec waitTime;
  waitTime.tv_sec = 0;
  waitTime.tv_nsec = taxi->position.delay;
  if((nanosleep(&waitTime,NULL))==-1){
    fprintf(stderr,"Error %s:%d: in waitOnCell %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return-1;
  }
  return 0;
}

int moveUp(Taxi* taxi, Grid* mappa,int semSetKey, int semStartKey,int SO_TIMEOUT,int semMutexKey)
{
  Cell* currentPosition = &(mappa->grid[taxi->position.x][taxi->position.y]);
  Cell* newPosition = &(mappa->grid[taxi->position.x-1][taxi->position.y]);
  
  alarm(SO_TIMEOUT);
  if((waitOnCell(taxi))==-1){
    fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return-1;
  }

  if((dec_sem(semStartKey, 0))==-1){
    fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  if((inc_sem(semStartKey, 0))==-1){
    fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  /*If I have come here it means master isn't printing*/

  if((dec_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width)))==-1){ /*Update starting Cell data*/
    fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }

  currentPosition->crossings++;

  if((inc_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width)))==-1){
    fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }

  if((dec_sem(semSetKey, cellToSemNum(*newPosition, mappa->width)))==-1){ /*Wait to be admitted in next cell*/
    fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }

  taxi->position = *newPosition;
  /*
   *  VERY IMPORTANT NOTE: a taxi might be printed in two different nearby cells if the cell it is on is printed between above and below instructions
   * 
   *  There is no way to atomically decrease a semaphore and increase another one so a small time interval between the two modifications exists and could lead to a misleading print.
  */
  if((inc_sem(semSetKey, cellToSemNum(*currentPosition, mappa->width)))==-1){ /*Leave Current Cell*/
    fprintf(stderr,"Error %s:%d: in moveUp %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }

  taxi->TTD++;
  alarm(0); 
  return 0;
}
int moveDown(Taxi* taxi, Grid* mappa,int semSetKey,int semStartKey, int SO_TIMEOUT,int semMutexKey)
{
  Cell* currentPosition = &(mappa->grid[taxi->position.x][taxi->position.y]);
  Cell* newPosition = &(mappa->grid[taxi->position.x+1][taxi->position.y]);
  
  alarm(SO_TIMEOUT);
  if((waitOnCell(taxi))==-1){
    fprintf(stderr,"Error %s:%d: in moveDown %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return-1;
  }

  if((dec_sem(semStartKey, 0))==-1){
    fprintf(stderr,"Error %s:%d: in moveDown %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  if((inc_sem(semStartKey, 0))==-1){
    fprintf(stderr,"Error %s:%d: in moveDown %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  /*If I have come here it means master isn't printing*/

  if((dec_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width)))==-1){ /*Update starting Cell data*/
    fprintf(stderr,"Error %s:%d: in moveDown %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  currentPosition->crossings++;
  if((inc_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width)))==-1){
    fprintf(stderr,"Error %s:%d: in moveDown %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }

  if((dec_sem(semSetKey, cellToSemNum(*newPosition, mappa->width)))==-1){ /*Wait to be admitted in next cell*/
    fprintf(stderr,"Error %s:%d: in moveDown %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  taxi->position = *newPosition;
  /*
   *  VERY IMPORTANT NOTE: a taxi might be printed in two different nearby cells if the cell it is on is printed between above and below instructions
   * 
   *  There is no way to atomically decrease a semaphore and increase another one so a small time interval between the two modifications exists and could lead to a misleading print.
  */
  if((inc_sem(semSetKey, cellToSemNum(*currentPosition, mappa->width)))==-1){ /*Leave Current Cell*/
    fprintf(stderr,"Error %s:%d: in moveDown %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  taxi->TTD++;
  alarm(0);
  return 0;
}
int moveRight(Taxi* taxi, Grid* mappa,int semSetKey, int semStartKey,int SO_TIMEOUT,int semMutexKey)
{
  Cell* currentPosition = &(mappa->grid[taxi->position.x][taxi->position.y]);
  Cell* newPosition = &(mappa->grid[taxi->position.x][taxi->position.y+1]);
  
  alarm(SO_TIMEOUT);
  if((waitOnCell(taxi))==-1){
    fprintf(stderr,"Error %s:%d: in moveRight %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return-1;
  }

  if((dec_sem(semStartKey, 0))==-1){
    fprintf(stderr,"Error %s:%d: in moveRight %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }

  if((inc_sem(semStartKey, 0))==-1){
    fprintf(stderr,"Error %s:%d: in moveRight %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  /*If I have come here it means master isn't printing*/

  if((dec_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width)))==-1){ /*Update starting Cell data*/
    fprintf(stderr,"Error %s:%d: in moveRight %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  
  currentPosition->crossings++;
  
  if((inc_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width)))==-1){
    fprintf(stderr,"Error %s:%d: in moveRight %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }

  if((dec_sem(semSetKey, cellToSemNum(*newPosition, mappa->width)))==-1){ /*Wait to be admitted in next cell*/
    fprintf(stderr,"Error %s:%d: in moveRight %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }

  taxi->position = *newPosition;
  /*
   *  VERY IMPORTANT NOTE: a taxi might be printed in two different nearby cells if the cell it is on is printed between above and below instructions
   * 
   *  There is no way to atomically decrease a semaphore and increase another one so a small time interval between the two modifications exists and could lead to a misleading print.
  */
  if((inc_sem(semSetKey, cellToSemNum(*currentPosition, mappa->width)))==-1){ /*Leave Current Cell*/
    fprintf(stderr,"Error %s:%d: in moveRight %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  taxi->TTD++;
  alarm(0);
  return 0;
}
int moveLeft(Taxi* taxi, Grid* mappa,int semSetKey, int semStartKey, int SO_TIMEOUT,int semMutexKey)
{
  Cell* currentPosition = &(mappa->grid[taxi->position.x][taxi->position.y]);
  Cell* newPosition = &(mappa->grid[taxi->position.x][taxi->position.y-1]);

  alarm(SO_TIMEOUT);
  if((waitOnCell(taxi))==-1){
    fprintf(stderr,"Error %s:%d: in moveLeft %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return-1;
  }

  if((dec_sem(semStartKey, 0))==-1){
    fprintf(stderr,"Error %s:%d: in moveLeft %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  if((inc_sem(semStartKey, 0))==-1){
    fprintf(stderr,"Error %s:%d: in moveLeft %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  /*If I have come here it means master isn't printing*/

  if((dec_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width)))==-1){ /*Update starting Cell data*/
    fprintf(stderr,"Error %s:%d: in moveLeft %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  currentPosition->crossings++;
  
  if((inc_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width)))==-1){
    fprintf(stderr,"Error %s:%d: in moveLeft %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }

  if((dec_sem(semSetKey, cellToSemNum(*newPosition, mappa->width)))==-1){ /*Wait to be admitted in next cell*/
    fprintf(stderr,"Error %s:%d: in moveLeft %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }

  taxi->position = *newPosition;
  /*
   *  VERY IMPORTANT NOTE: a taxi might be printed in two different nearby cells if the cell it is on is printed between above and below instructions
   * 
   *  There is no way to atomically decrease a semaphore and increase another one so a small time interval between the two modifications exists and could lead to a misleading print.
  */
  if((inc_sem(semSetKey, cellToSemNum(*currentPosition, mappa->width)))==-1){ /*Leave Current Cell*/
    fprintf(stderr,"Error %s:%d: in moveLeft %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  taxi->TTD++;
  alarm(0);
  return 0;
}



int move (Taxi* taxi, Grid* mappa, int semSetKey,int semStartKey,int SO_TIMEOUT,int semMutexKey) /*Returns 1 if taxi has arrived and 0 otherwise*/
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
          if((moveRight(taxi, mappa, semSetKey,semStartKey, SO_TIMEOUT, semMutexKey))==-1){
            fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            return -1;
          }
          if((moveDown(taxi, mappa, semSetKey,semStartKey, SO_TIMEOUT, semMutexKey))==-1){
            fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            return -1;
          }
          if(taxi->position.x + 1 < mappa->height){ 
            if((moveDown(taxi, mappa, semSetKey,semStartKey, SO_TIMEOUT, semMutexKey))==-1){
              fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
              return -1;
            }
          }
      }
      else
      {
          if((moveLeft(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
            fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            return -1;
          }
          if((moveDown(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
              fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
              return -1;
          }
          if(taxi->position.x + 1 < mappa->height){
            if((moveDown(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
              fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
              return -1;
            }
          } 
      }
    }
    else{
        if((moveDown(taxi, mappa, semSetKey,semStartKey, SO_TIMEOUT, semMutexKey))==-1){
          fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          return -1;
        }
    } 
  }

  else if(taxi->position.x - taxi->destination.x > 0)/*Upwnward direction*/
  {
    if(!mappa->grid[taxi->position.x-1][taxi->position.y].available)
    {
      /*Sopra c'é un buco!!!*/
      if(taxi->position.y - 1 < 0 && taxi->position.y <= taxi->destination.y)/*É vietato andare a sinistra e la destinazione è "a destra"*/
      {
        if((moveRight(taxi, mappa,semSetKey,semStartKey, SO_TIMEOUT, semMutexKey))==-1){
          fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          return -1;
        }
        if((moveUp(taxi, mappa,semSetKey,semStartKey, SO_TIMEOUT, semMutexKey))==-1){
          fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          return -1;
        }
        if(taxi->position.x - 1 >= 0){
          if((moveUp(taxi, mappa, semSetKey,semStartKey, SO_TIMEOUT, semMutexKey))==-1){
            fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            return -1;
          }
        }
      }
      else
      {
        if((moveLeft(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
          fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          return -1;
        }
        if((moveUp(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
          fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          return -1;
        }
        if(taxi->position.x - 1 >= 0){
          if((moveUp(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
            fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            return -1;
          }
        }
      }
    }
    else{
      if((moveUp(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
        fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
        return -1;
      }
    } 
  }

  else if(taxi->position.y - taxi->destination.y < 0)/*Right direction*/
  {
    if(!mappa->grid[taxi->position.x][taxi->position.y+1].available)
    {
      if(taxi->position.x - 1 < 0 && taxi->position.x <= taxi->destination.x)/*É vietato andare a su e la destinazione è "sotto"*/
      {
        if((moveDown(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
          fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          return -1;
        }
        if((moveRight(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
          fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          return -1;
        }
        if(taxi->position.y + 1 < mappa->width){
          if((moveRight(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
            fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            return -1;
          }
        }
      }
      else
      {
        if((moveUp(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
          fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          return -1;
        }
        if((moveRight(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
          fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          return -1;
        }
        if(taxi->position.y + 1 < mappa->width){
          if((moveRight(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
            fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            return -1;
          }
        }
      }
    }
    else{
      if((moveRight(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
        fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
        return -1;
      }
    }
  }
  else if(taxi->position.y - taxi->destination.y > 0)/*Left direction*/
  {
    if(!mappa->grid[taxi->position.x][taxi->position.y-1].available)
    {
      if(taxi->position.x - 1 < 0 && taxi->position.x <= taxi->destination.x)/*É vietato andare a su e la destinazione è "sotto"*/
      {
        if((moveLeft(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
          fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          return -1;
        }
        if((moveDown(taxi, mappa, semSetKey,semStartKey, SO_TIMEOUT, semMutexKey))==-1){
          fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          return -1;
        }
        if(taxi->position.y - 1 >= 0){
          if((moveLeft(taxi, mappa, semSetKey,semStartKey, SO_TIMEOUT, semMutexKey))==-1){
            fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            return -1;
          }
        }
      }
      else
      {
        if((moveUp(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
          fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
           return -1;
        }
        if((moveLeft(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
          fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          return -1;
        }
        if(taxi->position.y - 1 >= 0){
          if((moveLeft(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
            fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
            return -1;
          }
        }
      }
    }
    else{
      if((moveLeft(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey))==-1){
        fprintf(stderr,"Error %s:%d: in move %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
        return -1;
      }
    }
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


void moveTo(Taxi* t, Grid* MAPPA,int semSetKey, int semMutexKey, int semStartKey, int Busy, int SO_TIMEOUT)
{
  struct timespec startingTime, arrivalTime;
  float seconds = 0;
  long nanos = 0;

  if(Busy==TRUE){ /* counting TLT */
    clock_gettime(CLOCK_REALTIME, &startingTime);
    while(move(t,MAPPA,semSetKey,semStartKey, SO_TIMEOUT,semMutexKey) == 0);
    clock_gettime(CLOCK_REALTIME, &arrivalTime);

    nanos = arrivalTime.tv_nsec - startingTime.tv_nsec; 
    seconds = nanos / pow(10,9);

    if(seconds > t->TLT){
        t->TLT = seconds;
    }
    t -> totalTrips++;
  }
  else
      while(move(t,MAPPA,semSetKey,semStartKey, SO_TIMEOUT,semMutexKey) == 0);

}

void compareTaxi(Taxi* compTaxi, int taxiNumber)
{
	int i = 0;

	Taxi bestTotTrips=compTaxi[0], bestTTD=compTaxi[0], bestTLT=compTaxi[0];

		for(i=0;i<taxiNumber;i++){
			if(compTaxi[i].totalTrips > bestTotTrips.totalTrips){
				bestTotTrips = compTaxi[i];
			}
			if(compTaxi[i].TTD > bestTTD.TTD){
				bestTTD = compTaxi[i];
			}
			if(compTaxi[i].TLT > bestTLT.TLT){
				bestTLT = compTaxi[i];
			}
		}
		printf("Printing The Best TOTAL TRIPS TAXI\n");
		printTaxi(bestTotTrips);
		printf("Printing The Best TTD TAXI\n");
		printTaxi(bestTTD);
		printf("Printing The Best TLT TAXI\n");
		printTaxi(bestTLT);
	
}


int dec_sem (int sem_id, int index)
{
    struct sembuf sem_op;
    sem_op.sem_num  = index;
    sem_op.sem_op   = -1;
    sem_op.sem_flg = 0;
    if((semop(sem_id, &sem_op, 1))==-1){
      fprintf(stderr,"Error %s:%d: in dec_sem %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
      return -1;
    }
    else 
      return 0;

}

int inc_sem(int sem_id, int index)
{
    struct sembuf sem_op;
    sem_op.sem_num  = index;
    sem_op.sem_op   = 1;
    sem_op.sem_flg = 0;
    if((semop(sem_id, &sem_op, 1))==-1){
      fprintf(stderr,"Error %s:%d: in inc_sem %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
      return -1;
    }
    else 
      return 0;
}
int taxiDie(Taxi taxi, int fdWrite, Grid grid, int sem_id, int semMutexKey)
{
  char* message = malloc(500);
  alarm(0);
  if((inc_sem(sem_id, cellToSemNum(taxi.position, grid.width)))==-1){
    fprintf(stderr,"Error %s:%d: in taxiDie %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    return -1;
  }
  sprintf(message, "%d %d %d %d %d %d %d %f %d\n", getpid(), taxi.position.x, taxi.position.y, taxi.destination.x , taxi.destination.y, taxi.busy, taxi.TTD, taxi.TLT, taxi.totalTrips); /*The \n is the message terminator*/
  sendMsgOnPipe(message,fdWrite);
  free(message);
  close(fdWrite);
  exit(EXIT_SUCCESS);
}
