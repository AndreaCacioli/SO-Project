#define _GNU_SOURCE
#include "cell.h"
#include "grid.h"
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <math.h>
#include <time.h>
#define Boolean int
#define FALSE 0
#define TRUE !FALSE


void printMap(Grid grid,int semKey,Boolean compact)
{
  size_t i,j;

  printf("Printing the GRID:\n");
  for (i = 0; i < grid.height; i++)
  {
    for (j = 0; j < grid.width; j++)
    {
      printCell(grid.grid[i][j], grid.grid[i][j].capacity - semctl(semKey, cellToSemNum(grid.grid[i][j], grid.width), GETVAL) , compact);
    }
    printf("\n");
  }
}

Grid AllocateMap(int height, int width, int minCap, int maxCap, int minDelay, int maxDelay, Boolean verbose)
{
  int shmID;
  Grid grid;
  int i;
  int j;
  time_t t;

  srand((unsigned) time(&t)); /* Initializing the seed */

  grid.height = height;
  grid.width = width;

  if ((shmID = shmget(IPC_PRIVATE, sizeof(Cell) * height, IPC_CREAT | 0644))==-1){
    fprintf(stderr,"Error %s:%d: in AllocateMap %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (verbose) printf("Finding %lu bytes the array of Cell*, found at place %d\n",sizeof(Cell) * height,shmID);
   if((grid.grid = (Cell**)shmat(shmID, NULL, 0))==NULL){ 
    fprintf(stderr,"Error %s:%d: in AllocateMap %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    exit(EXIT_FAILURE);
   }
  if ((shmctl(shmID, IPC_RMID, NULL))==-1){  /* delete after use */
    fprintf(stderr,"Error %s:%d: in AllocateMap %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    exit(EXIT_FAILURE);
  } 
  if (verbose) printf("The grid of the grid has been allocated successfully =D\n");

  if ((shmID = shmget(IPC_PRIVATE, sizeof(Cell) * height * width, IPC_CREAT | 0644))==-1){
    fprintf(stderr,"Error %s:%d: in AllocateMap %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (verbose) printf("Finding %lu bytes the big Array, found at place %d\n",sizeof(Cell) * height * width,shmID);
  grid.grid[0] = (Cell*)shmat(shmID, NULL, 0);

  if ((shmctl(shmID, IPC_RMID, NULL))==-1){  /* delete after use */
    fprintf(stderr,"Error %s:%d: in AllocateMap %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    exit(EXIT_FAILURE);
  } 

  if (verbose) printf("The grid of the grid has been allocated successfully =D\nIndexing Now!\n");

  for(i = 0; i < height; i++)
  {
    grid.grid[i] = *grid.grid + (i * width);
  }

  for(i = 0; i < height; i++)
  {
    for(j = 0; j < width;j++)
    {

      grid.grid[i][j].x = i;
      grid.grid[i][j].y = j;
      grid.grid[i][j].available = TRUE;
      grid.grid[i][j].source = FALSE;
      if(maxCap == minCap) grid.grid[i][j].capacity = maxCap;
      else grid.grid[i][j].capacity = (rand() % (maxCap-minCap)) + minCap;
      if(maxDelay == minDelay) grid.grid[i][j].delay = maxDelay;
      else grid.grid[i][j].delay = (rand() % (maxDelay-minDelay)) + minDelay;
      grid.grid[i][j].crossings = 0;
      grid.grid[i][j].taken = FALSE;

      if (verbose) printf("Allocated (%d,%d) cell:\n",i,j);
      if (verbose) printCell(grid.grid[i][j],0, FALSE); 
      if (verbose) printf("\n");
    }
  }
  if (verbose) printf("Done Allocating!\n");
  return grid;
}

Boolean canBeHole(Grid grid, Cell c)
{
  int i=0,j=0;
  if(c.available == FALSE) return FALSE;
  else
  {
    for(i = c.x-1; i <= c.x+1; i++)
    {
      for(j = c.y-1; j <= c.y+1; j++)
      {
        if(i >= 0 && j>= 0 && i < grid.height && j < grid.width) /* We check if we are not out of bounds */
        {
          if(i == c.x && j == c.y) continue;
          else if(grid.grid[i][j].available == FALSE) return FALSE;
        }
      }
    }
    return TRUE;
  }
}

void placeHoles(Grid* grid, int numberOfHoles)
{
  time_t t;
  int i=0,j=0,stop=pow((grid->height*grid->width),2);
  

  srand((unsigned) time(&t)); /* Initializing the seed */

  while(numberOfHoles > 0 && stop > 0)/*Number of holes will keep track of how many holes are still to be placed*/
  {
    i = rand() % grid->height;
    j = rand() % grid->width;

    if(canBeHole(*grid,grid->grid[i][j]))
    {
      grid->grid[i][j].available = FALSE;
      numberOfHoles--;
    }
    stop--;
  }
  if(stop==0 && numberOfHoles!=0)
  {
    fprintf(stderr,"**** ERROR LOOP in placeHoles Restart TheGame. ****\n");
    exit(EXIT_FAILURE);
  }
}


void placeSources(Grid* grid, int Sour)
{
  int i,j;
  time_t t;
  srand((unsigned) time(&t)); /* Initializing the seed */

  while(Sour > 0)/*Number of holes will keep track of how many holes are still to be placed*/
  {
    i = rand() % grid->height;
    j = rand() % grid->width;

    if(grid->grid[i][j].available && !grid->grid[i][j].source)
    {
      grid->grid[i][j].source = TRUE;
      Sour--;
    }
  }
}

int initSem (Grid* grid, Boolean mutex)
{
  int i,j,ret;
  if((ret = semget(IPC_PRIVATE,grid->height * grid->width, IPC_CREAT | 0600/*Read and alter*/))==-1){
    fprintf(stderr,"Error %s:%d: in initSem %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    exit(EXIT_FAILURE);
  }
  for(i = 0; i < grid->height; i++)
  {
    for(j = 0; j < grid->width; j++)
    {
      if(mutex){ 
        if((semctl(ret, /* semnum= */ cellToSemNum(grid->grid[i][j],grid->width), SETVAL, 1))==-1){
          fprintf(stderr,"Error %s:%d: in initSem %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          exit(EXIT_FAILURE);
        }
      }

      else {
        if((semctl(ret, /* semnum= */ cellToSemNum(grid->grid[i][j],grid->width), SETVAL, grid->grid[i][j].capacity))==-1){
          fprintf(stderr,"Error %s:%d: in initSem %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
          exit(EXIT_FAILURE);
        }
      }
    }
  }
  return ret;
}

Grid* generateMap(int height, int width, int numberOfHoles,int numberOfSources, int minCap, int maxCap, int minDelay, int maxDelay)
{
  Grid* grid;
  if((grid = malloc(sizeof(Grid)))==NULL){
    fprintf(stderr,"Error %s:%d: in generateMap %d (%s)\n",__FILE__,__LINE__,errno,strerror(errno));
    exit(EXIT_FAILURE);
  }
  *grid = AllocateMap(height,width, minCap, maxCap, minDelay, maxDelay, FALSE);
  placeHoles(grid, numberOfHoles);
  placeSources(grid,numberOfSources);
  /*printMap(*grid);*/
  return grid;
}
