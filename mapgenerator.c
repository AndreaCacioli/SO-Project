#include "cell.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>

Cell** AllocateMap(int Height, int Width)
{
  printf("MapGenerator: Calling the shmget function...");
  int shmID = shmget(777, sizeof(Cell) * Height * Width, IPC_CREAT | 644);

  if(shmID == -1)
  {
    fprintf(stderr, "Error while allocating the shared memory segment\n");
    exit(1);
  }

  Cell grid[Height][Width] = shmat(shmID, NULL, 0); /*NULL: non need to attach to a specific location
                                                         0: non need for specific flags              */
  return grid;
}

void generateMap(int Height, int Width)
{
  Cell grid[Height][Width] = AllocateMap(Height,Width);

}
