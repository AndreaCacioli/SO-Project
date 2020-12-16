#ifndef _Some_Difficult_Name_Impossible_To_Guess___
#define _Some_Difficult_Name_Impossible_To_Guess___
#include "grid.h"

extern Grid* generateMap(int Height, int Width, int numberOfHoles,int numberOfSources , int minCap, int maxCap, int minDelay, int maxDelay);
extern void printMap(Grid grid);
extern void deallocateAllSHM(Grid* grid);

#endif
