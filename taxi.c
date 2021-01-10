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
  printf("---------Taxi--%d------\n",t.pid);
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

  /*printf("My semaphore before: %d\n",semctl(semSetKey, cellToSemNum(MAPPA->grid[x][y],MAPPA->width), GETVAL));*/
  dec_sem(semSetKey, cellToSemNum(MAPPA->grid[x][y],MAPPA->width));
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
  sigaction(SIGUSR1, &SigHandler, NULL);
  SigHandler.sa_handler = die;
  sigaction(SIGINT, &SigHandler, NULL);
  sigaction(SIGALRM, &SigHandler, NULL);

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

void moveUp(Taxi* taxi, Grid* mappa,int semSetKey, int semStartKey,int SO_TIMEOUT,int semMutexKey)
{
  Cell* currentPosition = &(mappa->grid[taxi->position.x][taxi->position.y]);
  Cell* newPosition = &(mappa->grid[taxi->position.x-1][taxi->position.y]);
  
  alarm(SO_TIMEOUT);
  waitOnCell(taxi);

  dec_sem(semStartKey, 0);
  inc_sem(semStartKey, 0);
  /*If I have come here it means master isn't printing*/

  dec_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width)); /*Update starting Cell data*/
  currentPosition->crossings++;
  inc_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width));

  dec_sem(semSetKey, cellToSemNum(*newPosition, mappa->width)); /*Wait to be admitted in next cell*/
  /*
   *  VERY IMPORTANT NOTE: a taxi might be printed in two different nearby cells if the cell it is on is printed between above and below instructions
   * 
   *  There is no way to atomically decrease a semaphore and increase another one so a small time interval between the two modifications exists and could lead to a misleading print.
  */
  inc_sem(semSetKey, cellToSemNum(*currentPosition, mappa->width)); /*Leave Current Cell*/
  taxi->position = *newPosition;
  taxi->TTD++;
  alarm(0); 

}
void moveDown(Taxi* taxi, Grid* mappa,int semSetKey,int semStartKey, int SO_TIMEOUT,int semMutexKey)
{
  Cell* currentPosition = &(mappa->grid[taxi->position.x][taxi->position.y]);
  Cell* newPosition = &(mappa->grid[taxi->position.x+1][taxi->position.y]);
  
   alarm(SO_TIMEOUT);
  waitOnCell(taxi);

  dec_sem(semStartKey, 0);
  inc_sem(semStartKey, 0);
  /*If I have come here it means master isn't printing*/

  dec_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width)); /*Update starting Cell data*/
  currentPosition->crossings++;
  inc_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width));

  dec_sem(semSetKey, cellToSemNum(*newPosition, mappa->width)); /*Wait to be admitted in next cell*/
  inc_sem(semSetKey, cellToSemNum(*currentPosition, mappa->width)); /*Leave Current Cell*/
  taxi->position = *newPosition;
  taxi->TTD++;
  alarm(0); 
}
void moveRight(Taxi* taxi, Grid* mappa,int semSetKey, int semStartKey,int SO_TIMEOUT,int semMutexKey)
{
  Cell* currentPosition = &(mappa->grid[taxi->position.x][taxi->position.y]);
  Cell* newPosition = &(mappa->grid[taxi->position.x][taxi->position.y+1]);
  
   alarm(SO_TIMEOUT);
  waitOnCell(taxi);

  dec_sem(semStartKey, 0);
  inc_sem(semStartKey, 0);
  /*If I have come here it means master isn't printing*/

  dec_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width)); /*Update starting Cell data*/
  currentPosition->crossings++;
  inc_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width));

  dec_sem(semSetKey, cellToSemNum(*newPosition, mappa->width)); /*Wait to be admitted in next cell*/
  inc_sem(semSetKey, cellToSemNum(*currentPosition, mappa->width)); /*Leave Current Cell*/
  taxi->position = *newPosition;
  taxi->TTD++;
  alarm(0); 
}
void moveLeft(Taxi* taxi, Grid* mappa,int semSetKey, int semStartKey, int SO_TIMEOUT,int semMutexKey)
{
  Cell* currentPosition = &(mappa->grid[taxi->position.x][taxi->position.y]);
  Cell* newPosition = &(mappa->grid[taxi->position.x][taxi->position.y-1]);

   alarm(SO_TIMEOUT);
  waitOnCell(taxi);

  dec_sem(semStartKey, 0);
  inc_sem(semStartKey, 0);
  /*If I have come here it means master isn't printing*/

  dec_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width)); /*Update starting Cell data*/
  currentPosition->crossings++;
  inc_sem(semMutexKey, cellToSemNum(*currentPosition, mappa->width));

  dec_sem(semSetKey, cellToSemNum(*newPosition, mappa->width)); /*Wait to be admitted in next cell*/
  inc_sem(semSetKey, cellToSemNum(*currentPosition, mappa->width)); /*Leave Current Cell*/
  taxi->position = *newPosition;
  taxi->TTD++;
  alarm(0); 
}



int move (Taxi* taxi, Grid* mappa, int semSetKey,int semStartKey,int SO_TIMEOUT,int semMutexKey) /*Returns 1 if taxi has arrived and 0 otherwise*/
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
        moveRight(taxi, mappa, semSetKey,semStartKey, SO_TIMEOUT, semMutexKey);
        moveDown(taxi, mappa, semSetKey,semStartKey, SO_TIMEOUT, semMutexKey);
        if(taxi->position.x + 1 < mappa->height) moveDown(taxi, mappa, semSetKey,semStartKey, SO_TIMEOUT, semMutexKey);
      }
      else
      {
        moveLeft(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
        moveDown(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
        if(taxi->position.x + 1 < mappa->height) moveDown(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
      }
    }
    else moveDown(taxi, mappa, semSetKey,semStartKey, SO_TIMEOUT, semMutexKey);
  }

  else if(taxi->position.x - taxi->destination.x > 0)/*Upwnward direction*/
  {
    if(!mappa->grid[taxi->position.x-1][taxi->position.y].available)
    {
      /*Sopra c'é un buco!!!*/
      if(taxi->position.y - 1 < 0 && taxi->position.y <= taxi->destination.y)/*É vietato andare a sinistra e la destinazione è "a destra"*/
      {
        moveRight(taxi, mappa,semSetKey,semStartKey, SO_TIMEOUT, semMutexKey);
        moveUp(taxi, mappa,semSetKey,semStartKey, SO_TIMEOUT, semMutexKey);
        if(taxi->position.x - 1 >= 0) moveUp(taxi, mappa, semSetKey,semStartKey, SO_TIMEOUT, semMutexKey);
      }
      else
      {
        moveLeft(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
        moveUp(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
        if(taxi->position.x - 1 >= 0) moveUp(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
      }
    }
    else moveUp(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
  }

  else if(taxi->position.y - taxi->destination.y < 0)/*Right direction*/
  {
    if(!mappa->grid[taxi->position.x][taxi->position.y+1].available)
    {
      if(taxi->position.x - 1 < 0 && taxi->position.x <= taxi->destination.x)/*É vietato andare a su e la destinazione è "sotto"*/
      {
        moveDown(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
        moveRight(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
        if(taxi->position.y + 1 < mappa->width) moveRight(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
      }
      else
      {
        moveUp(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
        moveRight(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
        if(taxi->position.y + 1 < mappa->width) moveRight(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
      }
    }
    else moveRight(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
  }
  else if(taxi->position.y - taxi->destination.y > 0)/*Left direction*/
  {
    if(!mappa->grid[taxi->position.x][taxi->position.y-1].available)
    {
      if(taxi->position.x - 1 < 0 && taxi->position.x <= taxi->destination.x)/*É vietato andare a su e la destinazione è "sotto"*/
      {
        moveDown(taxi, mappa, semSetKey,semStartKey, SO_TIMEOUT, semMutexKey);
        moveLeft(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
        if(taxi->position.y - 1 >= 0) moveLeft(taxi, mappa, semSetKey,semStartKey, SO_TIMEOUT, semMutexKey);
      }
      else
      {
        moveUp(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
        moveLeft(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
        if(taxi->position.y - 1 >= 0) moveLeft(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
      }
    }
    else moveLeft(taxi, mappa, semSetKey, semStartKey,SO_TIMEOUT, semMutexKey);
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


void dec_sem (int sem_id, int index)
{
    struct sembuf sem_op;
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
void taxiDie(Taxi taxi, int fdWrite, Grid grid, int sem_id, int semMutexKey)
{
  char* message = malloc(500);
  alarm(0);
  inc_sem(sem_id, cellToSemNum(taxi.position, grid.width));
  sprintf(message, "%d %d %d %d %d %d %d %f %d\n", getpid(), taxi.position.x, taxi.position.y, taxi.destination.x , taxi.destination.y, taxi.busy, taxi.TTD, taxi.TLT, taxi.totalTrips); /*The \n is the message terminator*/
  sendMsgOnPipe(message,fdWrite);
  free(message);
  close(fdWrite);
  exit(EXIT_SUCCESS);
}
