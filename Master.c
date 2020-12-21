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
#include "taxi.h"
#include "cell.h"
#include "mapgenerator.h"
#define FALSE 0
#define TRUE !FALSE
#define SO_HEIGHT 5
#define SO_WIDTH 3
#define MSGLEN 500

#define TEST_ERROR if (errno) {fprintf(stderr,		\
				       "%s:%d: PID=%5d: Error %d (%s)\n", \
				       __FILE__,			\
				       __LINE__,			\
				       getpid(),			\
				       errno,				\
				       strerror(errno));}


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
void setup();
void cleanup(int signal);
void signal_handler(int signal);
void killAllChildren();
void sourceTakePlace(Cell* myCell); /*TODO Think about moving this to a header file*/
void sourceSendMessage(Cell* myCell);

Grid* MAPPA;
Cell* sources;
int fd[2];
int rcvsignal = 0;
int semSetKey = 0;
int msgQId = 0;
int nbyte = 0;
struct my_msg_ msgQ;



struct my_msg_ {
	long mtype;                       /* type of message */
	char mtext[MSGLEN];         /* user-define message */
};

int main(void)
{
    char buf[1] = " ";
    pid_t pid;
    int i = 0;
		Cell prova;
		prova.x = 0;
		prova.y = 1;

    setup();

    for(i = 0 ; i < SO_TAXI ; i++) 									/*TAXI*/
    {
    	switch(pid=fork())
			{
    	case -1:
    	{
    		TEST_ERROR;
				exit(EXIT_FAILURE);
    	}
    	case 0:
    	{
    		Taxi taxi;
				char message[50] = "";
				int nextDestX, nextDestY;


				close(fd[0]);

				initTaxi(&taxi,MAPPA, signal_handler); /*We initialize the taxi structure*/
				printf("A new taxi has been born!\n");
				printTaxi(taxi);
				while(1)
				{
					findNearestSource(&taxi, sources, SO_SOURCES);
					printf("[%d]My destination is %d %d\n",getpid(),taxi.destination.x, taxi.destination.y);
					moveTo(&taxi, MAPPA);
					if(msgrcv(msgQId, &msgQ,MSGLEN,cellToSemNum(taxi.position, MAPPA->width)+1,IPC_NOWAIT) < 0)
					{
						continue; /*Not handling Error cause it is possible for a queue to not have any request!*/
					}
					sscanf(msgQ.mtext, "%d%d",&nextDestX, &nextDestY);
					printf("[%d]New dest from msgQ (%d,%d)\n",getpid(),nextDestX,nextDestY);
					moveTo(&taxi, MAPPA);
				}

				printTaxi(taxi);

				close(fd[1]);
				exit(EXIT_SUCCESS);
		  }
		    	default:
		    		break; /* Exit parent*/
		  }
		}
		for(i = 0 ; i < SO_SOURCES ; i++)                     /*SOURCES*/
    {
    	switch(pid=fork())
			{
    	case -1:
    	{
    		TEST_ERROR;
				exit(EXIT_FAILURE);
    	}
    	case 0: /*Code of the source*/
    	{
				Cell* myCell = NULL;
				int i=0;
				close(fd[1]); /*Sources don't use PIPE*/
			  close(fd[0]);

				sourceTakePlace(myCell);

				while(i<200)
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

		    if(pid != 0) /* Working area of the parent after fork a child */
		    {
		    			/*HANDLER SIGINT & SIGINT*/
					struct sigaction SigHandler;

					bzero(&SigHandler, sizeof(SigHandler));
					SigHandler.sa_handler = signal_handler;
					sigaction(SIGINT, &SigHandler, NULL);;
					sigaction(SIGALRM, &SigHandler, NULL);

			  	alarm(SO_DURATION);

		      while(1)
		      {
		        while( buf[0] != '\n') /* cycle of reading a message */
		        {
		          read(fd[0], buf, 1);
		          printf("%s", buf);
		        }
		        buf[0] = ' ';
		      }
		    }
		  	exit(EXIT_SUCCESS);
}

void setup()
{
  int i = 0,j = 0, k=0;
  int outcome = 0;
  lettura_file();

	sources = (Cell*)calloc(SO_SOURCES, sizeof(Cell));

	msgQId = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0600); /*Creo la MSGQ*/
	if (msgQId < 0) TEST_ERROR

  MAPPA = generateMap(SO_HEIGHT,SO_WIDTH,SO_HOLES,SO_SOURCES,SO_CAP_MIN,SO_CAP_MAX,SO_TIMENSEC_MIN,SO_TIMENSEC_MAX);/*Creo la Mappa*/
	/*Qua la mappa é Accessibile!!!*/

  semSetKey = initSem(MAPPA); /*Creo e inizializzo un semaforo per ogni Cell*/

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
				sources[k] = MAPPA->grid[i][j];
				k++;
			}
    }
    printf("\n");
  }
}

void signal_handler(int signal){
    switch(signal){ /* printf AS-Unsafe more info on man signal-safety*/
        case SIGINT:
            cleanup(signal);
            break;
        case SIGALRM:
            cleanup(signal);
            break;
				case SIGUSR1:
						printf("[%d] La mia triste vita non ha alcun senso\n", getpid());
						exit(EXIT_SUCCESS);
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
	printf("Starting cleanup!\n");
	killAllChildren();
	printf("All child processes killed\n");
  close(fd[1]);
  close(fd[0]);
	printf("Pipe closed!\n");
	printf("All SHM has been detatched!\n");
  semctl(semSetKey,0,IPC_RMID); /* rm sem */
  msgctl(msgQId, IPC_RMID, NULL); /* rm msg */
	printf("Semaphore And Q marked as deletable\n");
	printTopCells(SO_TOP_CELLS);
	free(sources);
	printf("Array of sources freed!\n");
  if(signal != -1)
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
  	    printf("%s non è presente come parametro nel configFile\n",string);
  }
  fclose(configFile);
}




void killAllChildren()
{
	int parent = 0, child = 0;
	FILE* out = popen("ps -e -o ppid= -o pid=", "r");
	while(fscanf(out, "%d%d\n",&parent, &child ) != EOF)
	{
		if(parent == getpid())
		{
			printf("killing %d...\n",child);
			kill(child, SIGUSR1);
		}
	}
	pclose(out);
}

void sourceTakePlace(Cell* myCell)
{
	int i,j;

	for(i = 0; i < MAPPA->height;i++)
	{
		for(j = 0; j < MAPPA->width;j++)
		{
			if (MAPPA->grid[i][j].source && !MAPPA->grid[i][j].taken)
			{
				MAPPA->grid[i][j].taken = TRUE;
				myCell = &MAPPA->grid[i][j];
			}
		}
	}
}

void sourceSendMessage(Cell* myCell)
{
	int x = 0, y = 0;

	msgQ.mtype = cellToSemNum(*myCell, MAPPA->width)+1;
	do
	{
		x = rand() % MAPPA->height;
		y = rand() % MAPPA->width;
	}while(!MAPPA->grid[x][y].available);

	nbyte = sprintf(msgQ.mtext,"%d %d\n",x,y);
	nbyte++;/* counting 0 term */


	if(msgsnd(msgQId, &msgQ, nbyte, 0) < 0)
	{
		TEST_ERROR
	}

	printf("[%d]Richiesta immessa sulla coda\n",getpid());
}
