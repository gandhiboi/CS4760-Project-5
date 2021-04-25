/*
	Kenan Krijestorac
	Project 5 - Memory Management
	Due: 26 April 2021
*/

#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/msg.h>

#include "shared.h"
#include "queue.h"

//macros
#define MAX_PROCESS 18
#define SECONDS 5

//tracker var
int numLines;

//var for shared memory; file pointer; msg q
SharedMemory * shared = NULL;
Queue * fifoQ;
FILE * fp = NULL;
Message msg;

//frame table and array to keep track of process ids
FrameTable ftable[256];
int pids[MAX_PROCESS];

//msg q ids
static int pMsgQID;
static int cMsgQID;

//statistics variables
float numMemoryAccessPerSec;
float numPageFaultsPerMemoryAccess;
float avgMemoryAccessSpeed;
float numSegFaultsPerMemoryAccess;

//function prototypes
void memoryManager();
void initPCB();
void logCheck(char*);
void usage();
void allocation();
void setTimer(int);
void signalHandler(int);
void displayStatistics();
void displayMemoryMap();

int main(int argc, char* argv[]) {

	signal(SIGINT, signalHandler);
	signal(SIGALRM, signalHandler);

	int opt;
	int m = 18;
	char * fileName = "output.log";
	
	numLines = 0;
	
	while((opt = getopt(argc, argv, "hm:")) != -1) {
		switch(opt) {	
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
			
			case 'm':
				if((m = atoi(optarg)) < 0 || (m = atoi(optarg)) > 18) {
					perror("oss.c: error: input beyond max process size; set to default 18");
					m = 18;
				}
				else {
					perror("oss.c: error: invalid process size");
					usage();
					exit(EXIT_SUCCESS);
				}
				break;
			default:
				perror("oss.c: error: invalid argument");
				usage();
				exit(EXIT_FAILURE);
		}
		
	}
	
	printf("==============================================\n");
	printf("\t\tMEMORY MANAGER\n");
	printf("==============================================\n");
	
	logCheck(fileName);
	allocation();
	initPCB();
	
	setTimer(SECONDS);
	
	printf("m: %d\n", m);
	printf("fileName: %s\n", fileName);
	
	shared->pcb.ptable.delimiter = 15;
	msg.mtype = 1;
	msg.page = 23;
	msgsnd(pMsgQID, &msg, sizeof(Message), 0);
	memoryManager();
	
	fprintf(fp, "delimiter value: %d\n", shared->pcb.ptable.delimiter);
	
	execl("./user", "user", (char*)NULL);
	
	fclose(fp);
	releaseSharedMemory();
	deleteMessageQueues();
	
	return EXIT_SUCCESS;
}

void memoryManager() {

	//while(1) {
		//numLines++;
		
		if(numLines >= 5000) {
			printf("Max lines (5000) reached in 'output.log': terminating program");
			kill(-getpid(), SIGINT);
			//break;
		}
	//}
	

}

void initPCB() {
	int i;
	for(i = 0; i < 256; i++) {
		ftable[i].frames = -1;
		ftable[i].dirtyBit = 0;
		ftable[i].referenceBit = 0;
		ftable[i].pid = -1;
	
	}

}

//allocates/attaches to shared memory; also creates fifo queue
void allocation() {
	allocateSharedMemory();
	allocateMessageQueues();
	
	shared = shmemPtr();
	
	pMsgQID = parentMsgQptr();
	cMsgQID = childMsgQptr();
	
	fifoQ = createQueue(256);
}

//checks to see if log file can be opened
void logCheck(char * file) {
	fp = fopen(file, "w");

	if(fp == NULL) {
		perror("oss.c: error: failed to open log file");
		exit(EXIT_FAILURE);
	}
}

//signal handler for ctrl + c and timer
void signalHandler(int signal) {
	fclose(fp);
	releaseSharedMemory();
	deleteMessageQueues();

	if(signal == SIGINT) {
		printf("oss.c: terminating: ctrl + c signal handler\n");
		fflush(stdout);
	}
	else if(signal == SIGALRM) {
		printf("oss.c: terminating: timer signal handler\n");
		fflush(stdout);
	}
	
	int i;
	for(i = 0; i < 18; i++) {
		kill(pids[i], SIGTERM);
	}
	
	while(wait(NULL) > 0);
	exit(EXIT_SUCCESS);
}

//timer for terminating program; not needed but just in case
void setTimer(int seconds) {
	struct sigaction act;
	act.sa_handler = &signalHandler;
	act.sa_flags = SA_RESTART;
	
	if(sigaction(SIGALRM, &act, NULL) == -1) {
		perror("oss.c: error: failed to set up sigaction handler for setTimer()");
		exit(EXIT_FAILURE);
	}
	
	struct itimerval value;
	value.it_interval.tv_sec = 0;
	value.it_interval.tv_usec = 0;
	
	value.it_value.tv_sec = seconds;
	value.it_value.tv_usec = 0;
	
	if(setitimer(ITIMER_REAL, &value, NULL)) {
		perror("oss.c: error: failed to set the timer");
		exit(EXIT_FAILURE);
	}
}

//help menu
void usage() {
	printf("======================================================================\n");
        printf("\t\t\t\tUSAGE\n");
        printf("======================================================================\n");
        printf("./oss -h [-s t] [-l f]\n");
        printf("run as: ./oss [options]\n");
        printf("ex. : ./oss -m 10\n");
        printf("if [-m t] is not defined it is set as default 18\n");
        printf("======================================================================\n");
        printf("-h              :       Describe how project should be run and then, terminate\n");
        printf("-m t            :       Indicate the number of \n");
        printf("======================================================================\n");
}
