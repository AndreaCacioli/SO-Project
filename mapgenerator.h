#ifndef _Some_Difficult_Name_Impossible_To_Guess___
#define _Some_Difficult_Name_Impossible_To_Guess___
#include "grid.h"
#define Boolean int

extern Grid* generateMap(int Height, int Width, int numberOfHoles,int numberOfSources , int minCap, int maxCap, int minDelay, int maxDelay);
extern void printMap(Grid grid,int semKey,Boolean compact);
extern void deallocateAllSHM(Grid* grid);
extern int initSem (Grid* grid);
extern void handle_sig_st(int signum);


#endif
