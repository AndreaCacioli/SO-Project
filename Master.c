#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>
#include "taxi.h"
#include "cell.h"
#include "mapgenerator.h"
#define FALSE 0
#define TRUE !FALSE
#define SO_HEIGHT 5
#define SO_WIDTH 5

int SO_TAXI;
int SO_SOURCES;
int SO_HOLES;
int SO_TOP_CELLS;
int SO_CAP_MIN; /* Inclusive */
int SO_CAP_MAX; /* Esclusive */
int SO_TIMENSEC_MIN;
int SO_TIMENSEC_MAX;
int SO_TIMEOUT;
int SO_DURATION;

void lettura_file();
void cleanup();
void setup();
void initTaxi(Taxi* taxi);

Grid* MAPPA;
int fd[2];
int semSetKey = 0;

int main(void)
{
    char buf[1] = " ";
    pid_t pid;
    int i = 0;
    system("sudo sysctl kern.sysv.shmseg=256");
    setup();

    for(i = 0 ; i < SO_TAXI ; i++) /*Zona di creazione dei processi TAXI*/
    {
      pid = fork();
      if(pid == 0) /* TAXI */
      {
        Taxi taxi;
        char message[50] = "";

        close(fd[0]);
        initTaxi(&taxi); /*We initialise the taxi structure*/
        setDestination(&taxi,MAPPA->grid[SO_HEIGHT-1][SO_WIDTH-1]);
        printTaxi(taxi);
        while(move(&taxi,MAPPA,fd[1]) == 0)
        {

        }
        printTaxi(taxi);
        close(fd[1]);
        exit(EXIT_SUCCESS);
      }
    }

    if(pid != 0) /* Zona del master dopo aver creato i figli */
    {
      signal(SIGALRM, cleanup); /*Installato Handler*/
      alarm(SO_DURATION);
      while(1) /* Gestione segnale di terminazione */
      {
        while( buf[0] != '\n') /*Ciclo di lettura di UN messaggio*/
        {
          read(fd[0], buf, 1);
          printf("%s", buf);
        }
        buf[0] = ' ';

      }
      /* CLEANUP */
      cleanup();

    }
  exit(EXIT_SUCCESS);
}



void cleanup()
{
  printMap(*MAPPA);
  close(fd[1]);
  close(fd[0]);
  deallocateAllSHM(MAPPA);
  semctl(semSetKey,0,IPC_RMID);
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


void setup()
{
  int i = 0,j = 0;
  int outcome = 0;

  lettura_file();
  MAPPA = generateMap(SO_HEIGHT,SO_WIDTH,SO_HOLES,SO_SOURCES,SO_CAP_MIN,SO_CAP_MAX,SO_TIMENSEC_MIN,SO_TIMENSEC_MAX);
  semSetKey = initSem(MAPPA);

  outcome = pipe(fd);
  if(outcome == -1)
  {
    fprintf(stderr, "Error creating pipe\n");
    exit(1);
  }

  for(i = 0; i < MAPPA->height; i++)
  {
    for(j = 0; j < MAPPA->width; j++)
    {
      printf("%d\t", semctl(semSetKey, /* semnum= */ cellToSemNum(MAPPA->grid[i][j],MAPPA->width), GETVAL));
    }
    printf("\n");
  }
}


void initTaxi(Taxi* taxi)
{
  int x,y;
  srand(getpid()); /* Initializing the seed to pid*/
  do
  {
    x = (rand() % MAPPA->height);
    y = (rand() % MAPPA->width);
  } while(!MAPPA->grid[x][y].available);

  taxi->position = MAPPA->grid[x][y]; /* TODO verifica che non serva un puntatore */
  taxi->busy = FALSE;
  printf("\n");
  taxi->destination = MAPPA->grid[0][0]; /* TODO inizializzare destination per farlo andare alla source */
  taxi->TTD = 0;
  taxi->TLT = 0;
  taxi->totalTrips = 0;
}
