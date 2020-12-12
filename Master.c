#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include "taxi.h"
#include "cell.h"
#include "mapgenerator.h"
#define FALSE 0
#define TRUE !FALSE 
int SO_TAXI;
int SO_SOURCES;
int SO_HOLES;
int SO_TOP_CELLS; 
int SO_CAP_MIN;
int SO_CAP_MAX;
int SO_TIMENSEC_MIN; 
int SO_TIMENSEC_MAX; 
int SO_TIMEOUT; 
int SO_DURATION; 

void lettura_file();

int main(void)
{ 
    char deallocator[50] = "";
    lettura_file();
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

void lettura_file(){
  int i;
  FILE* configFile;
  char* path = "./Input.config";
  char string[15]; 
  int parametri[10];
  int value;
   
  i=0;
  
  if((configFile=fopen(path, "r"))==NULL) {
	printf("Errore nell'apertura del file'");
	exit(1);
  }
  while(!feof(configFile)){
  	fscanf(configFile,"%s %d",string,&value);
  	parametri[i]=value;
  	i++;
  }
  fclose(configFile);
  
  SO_TAXI=parametri[0];
  SO_SOURCES=parametri[1];
  SO_TOP_CELLS=parametri[2];
  SO_CAP_MIN=parametri[3];
  SO_CAP_MAX=parametri[4];
  SO_TIMENSEC_MIN=parametri[5];
  SO_TIMENSEC_MAX=parametri[6];
  SO_TIMEOUT= parametri[7];
  SO_DURATION=parametri[8];

}
