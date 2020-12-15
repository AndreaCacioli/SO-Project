#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <unistd.h>
#include "taxi.h"
#include "cell.h"
#include "mapgenerator.h"
#define FALSE 0
#define TRUE !FALSE
#define SO_HEIGHT 7
#define SO_WIDTH 3

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
    Grid* MAPPA;
    char buf[500] = "";
    pid_t pid;
    int outcome = 0;
    char deallocator[50] = "";
    int i = 0;
    int fd[2];
    lettura_file();
    MAPPA = generateMap(SO_HEIGHT,SO_WIDTH,SO_HOLES,SO_SOURCES);

    outcome = pipe(fd);
    if(outcome == -1)
    {
      fprintf(stderr, "Error creating pipe\n");
      exit(1);
    }


    for(i = 0 ; i < SO_TAXI ; i++)
    {
      if((pid = fork()) == 0)
      {
        char message[50] = "";
        sprintf(message, "Ciao dal figlio %d",MAPPA->grid[0][0].x);
        sendMsgOnPipe(message,fd[0],fd[1]);
        close(fd[1]);
        break;
      }
      else
      {
        close(fd[1]);
        wait(NULL);
        if(read(fd[0], buf, 500) == 0)
        {
          fprintf(stderr, "Could not read from pipe\n");
          exit(1);
        }


        puts(buf);
        close(fd[0]);

      }
    }


    /* TODO: write deallocator function */
    /*

    system("ipcs");
    sprintf(deallocator, "ipcrm -M %d", IPC_PRIVATE);;
    printf("Cleanup...\n");
    system(deallocator);
    system("ipcs");

    */
  deallocateAllSHM(MAPPA);

  exit(EXIT_SUCCESS);
}

void lettura_file(){
  FILE* configFile;
  char* path = "./Input.config";
  char string[20];
  int value;

  if((configFile=fopen(path, "r"))==NULL) {
	printf("Errore nell'apertura del file'");
	exit(1);
  }
  while(fscanf(configFile,"%s %d",string,&value) != EOF){

  	if(strcmp(string,"SO_TAXI")==0){
  		SO_TAXI=value;
  	}
  	else if(strcmp(string,"SO_SOURCES")==0){
  		SO_SOURCES=value;
  	}
  	else if(strcmp(string,"SO_HOLES")==0){
  		SO_HOLES=value;
  	}
  	else if(strcmp(string,"SO_TOP_CELLS")==0){
  		SO_TOP_CELLS=value;
  	}
  	else if(strcmp(string,"SO_CAP_MIN")==0){
  		SO_CAP_MIN=value;
  	}
  	else if(strcmp(string,"SO_CAP_MAX")==0){
  		SO_CAP_MAX=value;
  	}
  	else if(strcmp(string,"SO_TIMENSEC_MIN")==0){
  		SO_TIMENSEC_MIN=value;
  	}
  	else if(strcmp(string,"SO_TIMENSEC_MAX")==0){
  		SO_TIMENSEC_MAX=value;
  	}
  	else if(strcmp(string,"SO_TIMEOUT")==0){
  		SO_TIMEOUT=value;
  	}
  	else if(strcmp(string,"SO_DURATION")==0){
  		SO_DURATION=value;
  	}
  	else
  	    printf("%s non è presente come parametro nel testo\n",string);
  }
  fclose(configFile);
}
