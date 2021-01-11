#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/errno.h>
#define SO_HEIGHT 3
#define SO_WIDTH 3
#define MSGLEN 500

#define TEST_ERROR if (errno) {fprintf(stderr,		\
				       "%s:%d: PID=%5d: Error %d (%s)\n", \
				       __FILE__,			\
				       __LINE__,			\
				       getpid(),			\
				       errno,				\
				       strerror(errno)); \
						   exit(EXIT_FAILURE);}


 struct my_msg_
 {
 	long mtype;                       /* type of message */
 	char mtext[MSGLEN];         /* user-define message */
 };

int main(void)
{
  struct my_msg_ msgQ;
  int msgQId = 0;
  char* x;
  char* y;
  x = malloc(128);
  y = malloc(128);
  msgQId = msgget(ftok("./Input.config", 1),0600);
  if (msgQId == -1) TEST_ERROR

  printf("Insert a Request: [Format: NumberOfSourceCell x y]\n");
  printf("NumberOfSourceCell = Cell.x * width + Cell.y + 1\n");

  scanf("%ld %s %s",&msgQ.mtype,x,y);
  if(msgQ.mtype>SO_WIDTH*SO_HEIGHT){
		do{
			printf("Error: cell doesn't exist\n");
	        printf("NumberOfSourceCell = Cell.x * width + Cell.y + 1\n");
			scanf("%ld %s %s",&msgQ.mtype,x,y);
		}
		while(msgQ.mtype>SO_WIDTH*SO_HEIGHT);
	}
  printf("Done reading:\nx:%s\ny:%s\nType:%ld\n", x, y, msgQ.mtype);
  x = strcat(x," ");
  x = strcat(x,y);
  x = strcat(x," ");
  strcpy(msgQ.mtext, x);
  printf("Sending a message of type: %ld\nMessage: %s\n",msgQ.mtype,msgQ.mtext);
  if(msgsnd(msgQId, &msgQ, strlen(msgQ.mtext), 0) < 0) TEST_ERROR
  free(x);
  free(y);
	return 0;
}
