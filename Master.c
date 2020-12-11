#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include "taxi.h"
#include "cell.h"
#include "mapgenerator.h"
#define FALSE 0
#define TRUE !FALSE

int main(void)
{
  FILE* configFile = NULL;
  char* path = "./Input.config";
  char deallocator[50] = "";

  /* TODO: set up config file reading  */

  generateMap(7,3,6);


    /* TODO: write deallocator function */
    /*

    system("ipcs");
    sprintf(deallocator, "ipcrm -M %d", IPC_PRIVATE);;
    printf("Cleanup...\n");
    system(deallocator);
    system("ipcs");

    */



  exit(EXIT_SUCCESS);
}
