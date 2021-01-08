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
#define SO_WIDTH 10
#define MSGLEN 500
#define ReadEnd 0
#define WriteEnd 1

#define TEST_ERROR if (errno) {fprintf(stderr,		\
				       "%s:%d: PID=%5d: Error %d (%s)\n", \
				       __FILE__,			\
				       __LINE__,			\
				       getpid(),			\
				       errno,				\
				       strerror(errno)); \
							semctl(semSetKey,0,IPC_RMID); \
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
void compareTaxi(Taxi* compTaxi);
void sourceTakePlace(Cell* myCell);
void sourceSendMessage(Cell* myCell);
void dieHandler(int signal);
Cell semNumToCell(int num, Grid Mappa);
void everySecond(Grid* Mappa);
void taxiWork();

Grid* MAPPA;
Cell** sources;
Taxi* bestTaxis;
Taxi taxi;
pid_t* pid_sources;
int fd[2];
int rcvsignal = 0;
int semSetKey = 0;		/*TODO Remove in case of error*/
int semMutexKey = 0;
int semStartKey = 0;
int msgQId = 0;
int nbyte = 0;
int taxiNumber = 0;
int* bornTaxi = NULL;
char* messageFromTaxi;
struct my_msg_ msgQ;

struct my_msg_ {
	long mtype;                       /* type of message */
	char mtext[MSGLEN];         /* user-define message */
};

int main(void)
{
    pid_t pid;
    int i = 0;

    setup();
	printMap(*MAPPA, semSetKey, FALSE);
	
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
				if(close(fd[WriteEnd]) == -1 || close(fd[ReadEnd]) == -1)TEST_ERROR /*Sources don't use PIPE*/

				myCell->taken = TRUE;
			
				

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
		bornTaxi = (int*)realloc(bornTaxi, (i + 1) * sizeof(int));
		
    	switch(bornTaxi[i] = pid = fork())
		{
    	case -1:
    	{
    		TEST_ERROR;
				exit(EXIT_FAILURE);
    	}
    	case 0:
    	{
				taxiWork();
		}
				default:
		    		break; /* Exit parent*/
		  }
		}


		    if(pid != 0) /* Working area of the parent after fork a child */
		    {
				int i = 0;
				struct timespec lastPrintTime;
				struct sembuf sem_op;
				FILE* fp = fdopen(fd[ReadEnd], "r");

				/*HANDLER SIGINT & SIGALARM*/
				struct sigaction SigHandler;
				bzero(&SigHandler, sizeof(SigHandler));
				SigHandler.sa_handler = signal_handler;
				if(sigaction(SIGINT, &SigHandler, NULL) == -1) TEST_ERROR
				if(sigaction(SIGALRM, &SigHandler, NULL) == -1) TEST_ERROR
				
				/*
				 *	Informo i taxi che possono partire!
				 */
    			sem_op.sem_num  = 0;
    			sem_op.sem_op   = SO_TAXI;
    			sem_op.sem_flg = 0;
    			semop(semStartKey, &sem_op, 1);

			  	alarm(SO_DURATION);


				clock_gettime(CLOCK_REALTIME, &lastPrintTime);
		      	while(1)
		      	{
					struct timespec currentTime;
					clock_gettime(CLOCK_REALTIME, &currentTime);
					if(currentTime.tv_sec - lastPrintTime.tv_sec >= 1)
					{
						everySecond(MAPPA);
						clock_gettime(CLOCK_REALTIME, &lastPrintTime);
					}

					/*Detect if taxi died and in case it did make it respawn*/
					if(fgets(messageFromTaxi, 100, fp) != NULL && !feof(fp))
					{
						if((bestTaxis = (Taxi*) realloc(bestTaxis, (taxiNumber + 1) * sizeof(Taxi))) == NULL) TEST_ERROR
						sscanf(messageFromTaxi, "%d %d %d %d %d %d %d %f %d", &bestTaxis[taxiNumber].pid, &bestTaxis[taxiNumber].position.x, &bestTaxis[taxiNumber].position.y, &bestTaxis[taxiNumber].destination.x, &bestTaxis[taxiNumber].destination.y, &bestTaxis[taxiNumber].busy, &bestTaxis[taxiNumber].TTD, &bestTaxis[taxiNumber].TLT, &bestTaxis[taxiNumber].totalTrips); /*Storing all information sent from Taxi process*/
						strcpy(messageFromTaxi, ""); /*Using strcpy otherwise we lose malloc*/
						inc_sem(semStartKey, 0); /*So that new taxi can immediately start*/

						bornTaxi = (int*) realloc(bornTaxi, (SO_TAXI + taxiNumber + 1) * sizeof(int));
						switch (bornTaxi[SO_TAXI + taxiNumber] = fork())
						{
							case 0:
								taxiWork();
								break;
							case -1:
								TEST_ERROR
								break;
							default:
								taxiNumber++;
								continue;

						}
						
					}

					
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

	if( SO_HOLES >= Max)
	{
		printf("Error: Too many holes, rerun the program with less holes %d \n", Max);
		exit(EXIT_FAILURE);
	}

	if( SO_HOLES > (SO_HEIGHT*SO_WIDTH/9)) printf("Warning: The program might crash due to the number of holes \n");

	pid_sources = calloc(sizeof(pid_t), SO_SOURCES);

	
	if((sources = (Cell**)calloc(SO_SOURCES, sizeof(Cell*)))== NULL) TEST_ERROR
	if((messageFromTaxi = malloc(100)) == NULL) TEST_ERROR

	msgQId = msgget(ftok("./Input.config", 1), IPC_CREAT | IPC_EXCL | 0600); /*Creo la MSGQ*/
	if (msgQId < 0) TEST_ERROR

    MAPPA = generateMap(SO_HEIGHT,SO_WIDTH,SO_HOLES,SO_SOURCES,SO_CAP_MIN,SO_CAP_MAX,SO_TIMENSEC_MIN,SO_TIMENSEC_MAX);/*Creo la Mappa*/

  	semSetKey = initSem(MAPPA, FALSE); /*Creo e inizializzo un semaforo per ogni Cell*/
	semMutexKey = initSem(MAPPA, TRUE);/*Creo e inizializzo un semaforo per ogni Cell per impedire ai taxi di scrivere male i crossings*/
	semStartKey = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);/*Create semaphore to make every taxi start at the same time*/
	if(semctl(semStartKey, 0, SETVAL, 0)) TEST_ERROR /*Initialize to 0 so every taxi starts asleep*/

	outcome = pipe(fd);
	if(outcome == -1)
	{
		fprintf(stderr, "Error creating pipe\n");
		TEST_ERROR
		exit(1);
	}

  for(i = 0; i < MAPPA->height; i++) 
  {
    for(j = 0; j < MAPPA->width; j++)
    {
      /*printf("%d\t", semctl(semSetKey, cellToSemNum(MAPPA->grid[i][j],MAPPA->width), GETVAL));*/
			if(MAPPA->grid[i][j].source)
			{
				sources[k] = &MAPPA->grid[i][j];
				k++;
			}
    }
    /*printf("\n");*/
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
			taxiDie(taxi, fd[WriteEnd], *MAPPA, semSetKey);
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
		printCell(crox[i],0, FALSE); /* no taxi alive */
		printf("\n");
	}
	printf("\n\n");
}

void cleanup(int signal)
{
	FILE* fp = fdopen(fd[ReadEnd], "r");
	int i = 0, successfulTotalTrips = 0;
	if(signal==14) printf("\n***TIME IS OVER***\n");
	close(fd[WriteEnd]);
	killAllChildren();
	
  	if(semctl(semSetKey,0,IPC_RMID) == -1) TEST_ERROR /* rm sem */
	if(semctl(semMutexKey,0,IPC_RMID) == -1) TEST_ERROR /* rm sem */
	if(semctl(semStartKey,0,IPC_RMID) == -1) TEST_ERROR /* rm sem */
  	if(msgctl(msgQId, IPC_RMID, NULL) == -1) TEST_ERROR /* rm msg */
	printTopCells(SO_TOP_CELLS);
	free(sources);

	while(fgets(messageFromTaxi, 100, fp) != NULL && !feof(fp))
	{
		sscanf(messageFromTaxi, "%d %d %d %d %d %d %d %f %d", &bestTaxis[taxiNumber].pid, &bestTaxis[taxiNumber].position.x, &bestTaxis[taxiNumber].position.y, &bestTaxis[taxiNumber].destination.x, &bestTaxis[taxiNumber].destination.y, &bestTaxis[taxiNumber].busy, &bestTaxis[taxiNumber].TTD, &bestTaxis[taxiNumber].TLT, &bestTaxis[taxiNumber].totalTrips); /*Storing all information sent from Taxi process*/
		taxiNumber++;
		strcpy(messageFromTaxi, ""); /*Using strcpy otherwise we lose malloc*/
	}

	printf("\n\nUnanswered requests:\n");
	while(msgrcv(msgQId, &msgQ,MSGLEN,0,IPC_NOWAIT) >= 0)
	{
		printf("► %s Created by source (%d, %d)\n", msgQ.mtext, semNumToCell( msgQ.mtype - 1, *MAPPA).x,semNumToCell( msgQ.mtype - 1, *MAPPA).y);
	}
	printf("\n");
	printf("\n\nAborted requests:\n");
	for(i = 0; i <= taxiNumber; i++)
	{
		if(bestTaxis[i].busy)
		{
			printf("The request (%d, %d) was not completed by taxi %d (aborted)\n",bestTaxis[i].destination.x,bestTaxis[i].destination.y,bestTaxis[i].pid);
		}
	}
	for(i = 0; i <= taxiNumber; i++)
	{
		successfulTotalTrips += bestTaxis[i].totalTrips;
	}
	printf("\n\nTotal Completed Trips:\n\t%d\n", successfulTotalTrips);

	
	compareTaxi(bestTaxis);
	fclose(fp);
	close(fd[ReadEnd]);
	free(pid_sources);
	free(messageFromTaxi);
	free(bornTaxi);
	free(bestTaxis);
	printf("Handling signal #%d (%s)\n",signal, strsignal(signal));
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

void compareTaxi(Taxi* compTaxi)
{
	int i = 0;

	Taxi bestTotTrips=compTaxi[0], bestTTD=compTaxi[0], bestTLT=compTaxi[0];

		for(i=0;i<taxiNumber;i++){
			if(compTaxi[i].totalTrips > bestTotTrips.totalTrips){
				bestTotTrips = compTaxi[i];
			}
			if(compTaxi[i].TTD > bestTTD.TTD){
				bestTTD = compTaxi[i];
			}
			if(compTaxi[i].TLT > bestTLT.TLT){
				bestTLT = compTaxi[i];
			}
		}
		printf("Printing The Best TOTAL TRIPS TAXI\n");
		printTaxi(bestTotTrips);
		printf("Printing The Best TTD TAXI\n");
		printTaxi(bestTTD);
		printf("Printing The Best TLT TAXI\n");
		printTaxi(bestTLT);
	
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
	FILE* out = popen("ps -A -o ppid= -o pid=", "r");
	if(out == NULL) TEST_ERROR
	while(fscanf(out, "%d%d",&parent, &child ) != EOF)
	{
		if(parent == getpid() && contains(pid_sources, child, SO_SOURCES))
		{
			/*printf("killing source %d...\n",child);*/
			kill(child, SIGTERM);
		}
		else if(parent == getpid() && contains(bornTaxi, child, SO_TAXI + taxiNumber)){
			/*printf("killing taxi %d...\n",child);*/
			kill(child, SIGUSR1);
		}
		else if(parent == getpid())
		{
			/*printf("killing Pipe process %d...\n",child);*/
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

Cell semNumToCell(int num, Grid Mappa)
{
	int i = 0;
	while (num > Mappa.width)
	{
		num -= Mappa.width;
		i++;
	}
	return Mappa.grid[i][num];
}


void everySecond(Grid* Mappa){
	printMap(*Mappa,semSetKey,TRUE);
}

void taxiWork()
{
	char message[50] = "";
	int nextDestX, nextDestY;
	close(fd[ReadEnd]); /*Closing Read End*/

	initTaxi(&taxi,MAPPA, signal_handler, dieHandler, semSetKey); /*We initialize the taxi structure*/

	findNearestSource(&taxi, sources, SO_SOURCES);

	dec_sem(semStartKey, 0);

	while(1) /*Gets out when receives signal SIGUSR1*/
	{
		nextDestX = 0;
		nextDestY = 0;
		findNearestSource(&taxi, sources, SO_SOURCES);
		/*printf("[%d]Going to Source: %d %d\n",getpid(),taxi.destination.x, taxi.destination.y);*/
		moveTo(&taxi, MAPPA,semSetKey,semMutexKey ,taxi.busy,SO_TIMEOUT);
		if(msgrcv(msgQId, &msgQ,MSGLEN,cellToSemNum(taxi.position, MAPPA->width)+1,IPC_NOWAIT) < 0)
		{
			continue; /*Not handling Error cause it is possible for a queue to not have any request!*/
		}
		sscanf(msgQ.mtext, "%d%d",&nextDestX, &nextDestY);
		/*printf("[%d]New dest from msgQ (%d,%d)\n",getpid(),nextDestX,nextDestY);*/
		setDestination(&taxi,MAPPA->grid[nextDestX][nextDestY]);
		taxi.busy=TRUE;
		moveTo(&taxi, MAPPA,semSetKey,semMutexKey,taxi.busy,SO_TIMEOUT);
		taxi.busy=FALSE;
	}
}
