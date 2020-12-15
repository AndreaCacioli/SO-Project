#include "cell.h"
#include "grid.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#define Boolean int
#define FALSE 0
#define TRUE !FALSE

void printMap(Grid grid)
{
  size_t i,j;

  printf("Printing the GRID:\n");
  for (i = 0; i < grid.height; i++)
  {
    for (j = 0; j < grid.width; j++)
    {
      printCell(grid.grid[i][j]);
    }
    printf("\n");
  }
}

Grid* AllocateMap(int height, int width)
{
  int shmID;
  Grid* grid;
  int i;
  int j;

  shmID = shmget(IPC_PRIVATE, sizeof(Grid), IPC_CREAT | 0644);
  printf("Finding %lu bytes for grid object, found at place %d\n",sizeof(Grid),shmID);
  if (shmID == -1)
  {
    fprintf(stderr,"shmget failed");
    exit(1);
  }
  grid = (Grid*)shmat(shmID, NULL, 0);
  grid->height = height;
  grid->width = width;
  shmctl(shmID, IPC_RMID, NULL); /* market disallocazione del puntatore */


  shmID = shmget(IPC_PRIVATE, sizeof(Cell) * height * width, IPC_CREAT | 0644);
  printf("Finding %lu bytes the actual grid, found at place %d\n",sizeof(Cell) * height * width,shmID);
  grid->grid = (Cell**)shmat(shmID, NULL, 0);
  shmctl(shmID, IPC_RMID, NULL);  /* market di disallocazione della griglia */
  printf("The grid of the grid has been allocated successfully =D\n");

  /* Allocating each row */
  for(i=0; i < height; i++)
  {
    printf("Allocating #%d row\n",i);
    shmID = shmget(IPC_PRIVATE, sizeof(Cell) * width , IPC_CREAT | 0644);
    printf("Finding %lu bytes the #%d row, found at place %d\n",sizeof(Cell) * width,i,shmID);
    if (shmID == -1)
    {
      fprintf(stderr,"shmget failed");
      exit(1);
    }
    grid->grid[i] = (Cell*)shmat(shmID, NULL, 0);
    shmctl(shmID, IPC_RMID, NULL); /* market di disallocazione di ogni riga*/
    printf("#%d row attatched\n",i);
    if (grid->grid[i] == NULL)
    {
      fprintf(stderr,"shmat failed");
      exit(1);
    }
    for(j = 0; j<width;j++)
    {
      shmID = shmget(IPC_PRIVATE, sizeof(Cell), IPC_CREAT | 0644);
      printf("Finding %lu bytes for cell at (%d,%d), found at place %d\n",sizeof(Cell),i,j,shmID);
      if (shmID == -1)
      {
        fprintf(stderr,"shmget failed");
        exit(1);
      }
      grid->grid[i][j] = *((Cell*)shmat(shmID, NULL, 0));
      if (grid->grid[i] == NULL)
      {
        fprintf(stderr,"shmat failed");
        exit(1);
      }
      grid->grid[i][j].x = i;
      grid->grid[i][j].y = j;
      grid->grid[i][j].available = TRUE;
      printf("Allocated (%d,%d) cell:\n",i,j);
      printCell(grid->grid[i][j]);
      printf("\n");
      shmctl(shmID, IPC_RMID, NULL); /* market di disallocazione di ogni colonna*/
    }
  }
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
  int i,j;
  time_t t;
  srand((unsigned) time(&t)); /* Initializing the seed */

  while(numberOfHoles > 0)/*Number of holes will keep track of how many holes are still to be placed*/
  {
    i = rand() % grid->height;
    j = rand() % grid->width;

    if(canBeHole(*grid,grid->grid[i][j]))
    {
      grid->grid[i][j].available = FALSE;
      numberOfHoles--;
    }
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
Grid* generateMap(int height, int width, int numberOfHoles,int numberOfSources)
{
  Grid* grid;
  grid = AllocateMap(height,width);
  printf("The map has been Allocated\n Starting to place holes!\n");
  placeHoles(grid, numberOfHoles);
  printf("Holes placed done, now placing sources\n");
  placeSources(grid,numberOfSources);
  printMap(*grid);
  return grid;
}
