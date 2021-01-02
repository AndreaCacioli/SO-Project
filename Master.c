#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <math.h>
#include "taxi.h"
#include "cell.h"
#include "mapgenerator.h"
#define FALSE 0
#define TRUE !FALSE
#define SO_HEIGHT 10
#define SO_WIDTH 3
#define MSGLEN 500

#define TEST_ERROR if (errno) {fprintf(stderr,		\
				       "%s:%d: PID=%5d: Error %d (%s)\n", \
				       __FILE__,			\
				       __LINE__,			\
				       getpid(),			\
				       errno,				\
				       strerror(errno)); \
							 semctl(semSetKey,0,IPC_RMID); \
						 	 semctl(semMutex,0,IPC_RMID); \
						   msgctl(msgQId, IPC_RMID, NULL); \
						   exit(EXIT_FAILURE);}


int SO_TAXI;
int SO_SOURCES;
int SO_HOLES;
int SO_TOP_CELLS;
int SO_CAP_MIN; /* Inclusive */
int SO_CAP_MAX; /* Exclusive */
int SO_TIMENSEC_MIN;
int SO_TIMENSEC_MAX;
int SO_TIMEOUT;
int SO_DURATION;

void lettura_file();
void setup();
void cleanup(int signal);
void signal_handler(int signal);
void killAllChildren();
void sourceTakePlace(Cell* myCell); /*TODO Think about moving this to a header file*/
void sourceSendMessage(Cell* myCell);
void dieHandler(int signal);

Grid* MAPPA;
Cell** sources;
pid_t* pid_taxi;
pid_t* pid_sources;
int fd[2];
int rcvsignal = 0;
int semSetKey = 0;
int semMutex = 0;
int msgQId = 0;
int nbyte = 0;
Boolean closing = FALSE;
char* messageFromTaxi;
char buf = ' ';
Taxi taxi;
struct my_msg_ msgQ;

struct my_msg_ {
	long mtype;                       /* type of message */
	char mtext[MSGLEN];         /* user-define message */
};

int main(void)
{
    pid_t pid;
    int i = 0;
		Cell prova;
		prova.x = 0;
		prova.y = 1;
		printf("[%d M] STARTING...\n",getpid());
    setup();
		for(i = 0 ; i < SO_SOURCES; i++)                     /*SOURCES*/
    {
    	switch(pid_sources[i]=pid=fork())
			{
    	case -1:
    	{
    		TEST_ERROR;
				exit(EXIT_FAILURE);
    	}
    	case 0: /*Code of the source*/
    	{
				Cell* myCell = sources[i];
				int i = 0;
				struct sembuf sem_op;
				if(close(fd[1]) == -1 || close(fd[0]) == -1)TEST_ERROR /*Sources don't use PIPE*/

				myCell->taken = TRUE;
				printf("[%d S]Found Place at: (%d,%d)\n",getpid(), myCell->x, myCell->y);

				while(1)
				{
						sourceSendMessage(myCell);
						i++;
						sleep(1);
				}

				exit(EXIT_SUCCESS);
		  }

		    	default:
		    		break;
		  }
		}

    for(i = 0 ; i < SO_TAXI ; i++) 									/*TAXI*/
    {
    	switch(pid_taxi[i]=pid=fork())
			{
    	case -1:
    	{
    		TEST_ERROR;
				exit(EXIT_FAILURE);
    	}
    	case 0:
    	{

				char message[50] = "";
				int nextDestX, nextDestY;

				close(fd[0]);

				initTaxi(&taxi,MAPPA, signal_handler, dieHandler); /*We initialize the taxi structure*/

				findNearestSource(&taxi, sources, SO_SOURCES);

				printf("A new taxi has been born!\n");
				printTaxi(taxi);

				while(!closing)
				{	nextDestX = 0 ;
					nextDestY = 0;
					findNearestSource(&taxi, sources, SO_SOURCES);
					printf("[%d]Going to Source: %d %d\n",getpid(),taxi.destination.x, taxi.destination.y);
					moveTo(&taxi, MAPPA,semSetKey,taxi.busy);
					if(msgrcv(msgQId, &msgQ,MSGLEN,cellToSemNum(taxi.position, MAPPA->width)+1,IPC_NOWAIT) < 0)
					{
						continue; /*Not handling Error cause it is possible for a queue to not have any request!*/
					}
					sscanf(msgQ.mtext, "%d%d",&nextDestX, &nextDestY);
					printf("[%d]New dest from msgQ (%d,%d)\n",getpid(),nextDestX,nextDestY);
					setDestination(&taxi,MAPPA->grid[nextDestX][nextDestY]);
					taxi.busy=TRUE;
					moveTo(&taxi, MAPPA,semSetKey,taxi.busy);
					taxi.busy=FALSE;
				}
			}
				default:
		    		break; /* Exit parent*/
		  }
		}


		    if(pid != 0) /* Working area of the parent after fork a child */
		    {
					int i = 0;
		    			/*HANDLER SIGINT & SIGINT*/
					struct sigaction SigHandler;
					bzero(&SigHandler, sizeof(SigHandler));
					SigHandler.sa_handler = signal_handler;
					if(sigaction(SIGINT, &SigHandler, NULL) == -1) TEST_ERROR
					if(sigaction(SIGALRM, &SigHandler, NULL) == -1) TEST_ERROR
			  	alarm(SO_DURATION);

		      while(1)
		      {
						/*
						i = 0;

		        while( buf != '\n')  cycle of reading a message
		        {
		          read(fd[0], &buf, sizeof(char));
							messageFromTaxi[i] = buf;
							i++;
		        }
		        buf = ' ';
						printf("Message sent from taxi on PIPE: %s\n",messageFromTaxi);
						messageFromTaxi = "";	*/
		      }
		    }

		  	exit(EXIT_SUCCESS);
}

void setup()
{
  int i = 0,j = 0, k=0;
  int outcome = 0;
  int Max = (int)pow(ceil(((int)floor(sqrt(SO_HEIGHT * SO_WIDTH)))/2.0) ,2);
  lettura_file();
  	if( SO_HOLES >= Max) {
  	printf("Error: Too many holes, rerun the program with less holes %d \n", Max);
  	exit(EXIT_FAILURE);}

  	if( SO_HOLES > (SO_HEIGHT*SO_WIDTH/9)) printf("Warning: The program might crash due to the number of holes \n");

	pid_taxi = calloc(sizeof(pid_t), SO_TAXI);
	pid_sources = calloc(sizeof(pid_t), SO_SOURCES);

	sources = (Cell**)calloc(SO_SOURCES, sizeof(Cell*));
	if((messageFromTaxi = malloc(100)) == NULL) TEST_ERROR

	if(sources == NULL) TEST_ERROR

	msgQId = msgget(ftok("./Input.config", 1), IPC_CREAT | IPC_EXCL | 0600); /*Creo la MSGQ*/
	if (msgQId < 0) TEST_ERROR

   MAPPA = generateMap(SO_HEIGHT,SO_WIDTH,SO_HOLES,SO_SOURCES,SO_CAP_MIN,SO_CAP_MAX,SO_TIMENSEC_MIN,SO_TIMENSEC_MAX);/*Creo la Mappa*/
	/*Qua la mappa é Accessibile!!!*/

  semSetKey = initSem(MAPPA); /*Creo e inizializzo un semaforo per ogni Cell*/
	semMutex = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600/*Read and alter*/);
	if(semMutex == -1) TEST_ERROR
	if(semctl(semMutex, 0, SETVAL, 1) == -1) TEST_ERROR

  outcome = pipe(fd);
  if(outcome == -1)
  {
    fprintf(stderr, "Error creating pipe\n");
		TEST_ERROR
    exit(1);
  }

  for(i = 0; i < MAPPA->height; i++) /*Per qualche motivo 'sto doppio ciclo rompe la mappa*/
  {
    for(j = 0; j < MAPPA->width; j++)
    {
      printf("%d\t", semctl(semSetKey, /*semnum=*/cellToSemNum(MAPPA->grid[i][j],MAPPA->width), GETVAL));
			if(MAPPA->grid[i][j].source)
			{
				sources[k] = &MAPPA->grid[i][j];
				k++;
			}
    }
    printf("\n");
  }
}

void signal_handler(int signal){
    switch(signal){ /* printf AS-Unsafe more info on man signal-safety*/
        case SIGINT: /*Only master handles this signal*/
            cleanup(signal);
            break;
        case SIGALRM: /*Only master handles this signal*/
            cleanup(signal);
            break;
		    case SIGUSR1:  /*Only taxi handles this signal*/
            /*TODO REMOVE CLOSING var*/
						taxiDie(taxi, fd[0], fd[1]);
            break;
    }
}

int cmpfunc (const void * a, const void * b) /*returns a > b only used for qsort*/
{
 return ( (*(Cell*)b).crossings - (*(Cell*)a).crossings );
}

void printTopCells(int nTopCells)
{
	int i, j;
	Cell* crox = malloc(MAPPA->width * MAPPA->height * sizeof(Cell));
	if (crox == NULL) TEST_ERROR
	/*printf("Heap space for crox has been allocated!\n");*/
	for(i = 0; i <  MAPPA->height; i++)
	{
		for(j = 0; j <  MAPPA->width; j++)
		{
			/*printf("Crox[%d] now is:\t",cellToSemNum(MAPPA->grid[i][j], MAPPA->width));*/
			crox[ cellToSemNum(MAPPA->grid[i][j], MAPPA->width) ] = MAPPA->grid[i][j];
		}
	}
	/*printf("Sorting by crossings\n");*/
	qsort(crox, MAPPA->width * MAPPA->height, sizeof(Cell), cmpfunc);

	printf("**** PRINTING TOP CELLS ****\n");
	for(i = 0; i < nTopCells ; i++)
	{
		printf("%d: ",i+1);
		printCell(crox[i]);
		printf("\n");
	}
	printf("\n");
}

void cleanup(int signal)
{
	FILE* fp = fdopen(fd[0], "r");
	killAllChildren();
	printf("All child processes killed\n");
	printf("Starting cleanup!\n");
	printMap(*MAPPA);
	printf("All SHM has been detatched!\n");
  if(semctl(semSetKey,0,IPC_RMID) == -1) TEST_ERROR /* rm sem */
	if(semctl(semMutex,0,IPC_RMID) == -1) TEST_ERROR /* rm sem */
  if(msgctl(msgQId, IPC_RMID, NULL) == -1) TEST_ERROR /* rm msg */
	printf("Semaphore And Q marked as deletable\n");
	printTopCells(SO_TOP_CELLS);
	free(sources);
	printf("Array of sources freed!\n");
  if(signal != -1)
  printf("Handling signal #%d (%s)\n",signal, strsignal(signal));

	while(fgets(messageFromTaxi, 100, fp) != NULL)
	{
		printf("Message sent from taxi on PIPE: %s\n",messageFromTaxi);
		messageFromTaxi = "";
	}
	fclose(fp);
	printf("***************FINE LETTURAAAAAAA***********\n");
	if(close(fd[1]) == -1 || close(fd[0])) TEST_ERROR
	printf("Pipe closed!\n");
	free(pid_sources);
	free(pid_taxi);
	free(messageFromTaxi);
  exit(EXIT_SUCCESS);
}

void lettura_file(){
  FILE* configFile;
  char* path = "./Input.config";
  char string[20];
  int value;

  if((configFile=fopen(path, "r"))==NULL) {
	TEST_ERROR
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
  	    printf("%s it's not a parameter inside configFile\n",string);
  }
  fclose(configFile);
}

Boolean contains(int* array, int pid, int size)
{
	int i = 0;
	for( i = 0; i < size; i++)
	{
		if(array[i] == pid) return TRUE;
	}
	return FALSE;
}


void killAllChildren()
{
	int parent = 0, child = 0;
	FILE* out = popen("ps -e -o ppid= -o pid=", "r");
	if(out == NULL) TEST_ERROR
	while(fscanf(out, "%d%d",&parent, &child ) != EOF)
	{
		if(parent == getpid() && contains(pid_sources, child, SO_SOURCES))
		{
			printf("killing source %d...\n",child);
			kill(child, SIGTERM);
		}
		else if(parent == getpid() && contains(pid_taxi, child, SO_TAXI)){
			printf("killing taxi %d...\n",child);
			kill(child, SIGUSR1);
		}
		else if(parent == getpid())
		{
			printf("killing Pipe process %d...\n",child);
			kill(child, SIGTERM);
		}
	}
	pclose(out);
}

void sourceSendMessage(Cell* myCell)
{
	int x = 0, y = 0;
	msgQ.mtype = cellToSemNum(*myCell, MAPPA->width)+1;
	srand(time(NULL)+getpid()+rand());
	do
	{
		x = rand() % MAPPA->height;
		y = rand() % MAPPA->width;

	}while(!MAPPA->grid[x][y].available);

	nbyte = sprintf(msgQ.mtext,"%d %d\n",x,y);
	nbyte++;/* counting 0 term */

	if(msgsnd(msgQId, &msgQ, nbyte, 0) < 0) TEST_ERROR

	/*printf("[%d S]Richiesta immessa sulla coda\n",getpid());*/
}

void dieHandler(int signal)
{
	kill(getpid(), SIGUSR1);
}
