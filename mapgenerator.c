#include "cell.h"
#include "grid.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>

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

Grid AllocateMap(int height, int width)
{
  int shmID;
  Grid grid;
  int i;
  int j;
  Grid* p;

  printf("MapGenerator: Calling the shmget function...\n");
  shmID = shmget(777, sizeof(Grid), IPC_CREAT | 0644);

      if(shmID == -1)
      {
        fprintf(stderr, "Error while allocating the shared memory segment\n");
        exit(1);
      }
      grid.height = height;
      grid.width = width;
      p = shmat(shmID, NULL, 0);
      grid.grid = *p;
  printMap(grid);
  return grid;
}



void generateMap(int height, int width)
{
  Grid grid;
  grid = AllocateMap(height,width);
  printf("The map has been Allocated\n");
}
