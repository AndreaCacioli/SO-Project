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
void initTaxi(Taxi* taxi);
void cleanup(int signal);
void signal_handler(int signal);

Grid* MAPPA;
int fd[2];
int rcvsignal = 0;
int semSetKey = 0;
int msgQId = 0;
int nbyte = 0;
struct my_msg_ {
	long mtype;                       /* type of message */
	char mtext[MSGLEN];         /* user-define message */
};

int main(void)
{
    char buf[1] = " ";
    pid_t pid;
    int i = 0;
		struct my_msg_ msgQ;
		msgQId = msgget(IPC_PRIVATE, IPC_CREAT | IPC_EXCL | 0600);
		if (msgQId < 0) TEST_ERROR
	
    /*system("sudo sysctl kern.sysv.shmseg=256");*/
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

				close(fd[0]);
				initTaxi(&taxi); /*We initialize the taxi structure*/
				setDestination(&taxi,MAPPA->grid[SO_HEIGHT-1][SO_WIDTH-1]);
				printTaxi(taxi);
				while(move(&taxi,MAPPA,fd[1]) == 0)
				{

				}
				if(msgrcv(msgQId, &msgQ,MSGLEN,0,0) < 0)
				{
					TEST_ERROR
				}
				printf("Ho ricevuto un messaggio sulla coda: %s\n", msgQ.mtext);
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
	
				close(fd[1]); /*Sources don't use PIPE*/
			  	close(fd[0]);
				
				msgQ.mtype = i;
				nbyte = sprintf(msgQ.mtext,"CHILD PID %5d:Sono a BOLOGNA\n",getpid());
				nbyte++;/* counting 0 term */
			

				if(msgsnd(msgQId, &msgQ, nbyte, 0) < 0)
				{
					TEST_ERROR
				}

				printf("Ho mandato un messaggio sulla coda\n");

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
		      cleanup(-1); /* CLEANUP */
		    }


		  	exit(EXIT_SUCCESS);
}

void signal_handler(int signal){
    switch(signal){ /* printf AS-Unsafe more info on man signal-safety*/
        case SIGINT:
            cleanup(signal);
            break;
        case SIGALRM:
            cleanup(signal);
            break;
    }
}

int cmpfunc (const void * a, const void * b) /*returns a > b only used for qsort*/
{
 return ( (*(Cell*)b).crossings - (*(Cell*)a).crossings  );
}

void printTopCells(int nTopCells)
{
	int i, j;
	Cell crox[MAPPA->width * MAPPA->height];
	for(i = 0; i <  MAPPA->height; i++)
	{
		for(j = 0; j <  MAPPA->width; j++)
		{
			crox[ cellToSemNum(MAPPA->grid[i][j], MAPPA->width) ] = MAPPA->grid[i][j];
		}
	}
	qsort(crox, MAPPA->width * MAPPA->height, sizeof(Cell), cmpfunc);

	printf("****PRINTING TOPCELLS****\n",i+1);
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
  printTopCells(SO_TOP_CELLS);
  printMap(*MAPPA);
  close(fd[1]);
  close(fd[0]);
  deallocateAllSHM(MAPPA);
  semctl(semSetKey,0,IPC_RMID); /* rm sem */
  msgctl(msgQId, IPC_RMID, NULL); /* rm msg */
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
	TEST_ERROR
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
