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

#include "shared.h"

#define MAX_PROCESS 18
#define SECONDS 5

SharedMemory * shared = NULL;
FILE * fp = NULL;
Message msg;

FrameTable ftable[256];
int pids[MAX_PROCESS];

static int pMsgQID;
static int cMsgQID;

void logCheck(char*);
void usage();
void allocation();
void setTimer(int);
void signalHandler(int);

int main(int argc, char* argv[]) {

	signal(SIGINT, signalHandler);
	signal(SIGALRM, signalHandler);

	int opt;
	int m = 18;
	char * fileName = "output.log";
	
	while((opt = getopt(argc, argv, "hm:")) != -1) {
		switch(opt) {	
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
			
			case 'm':
				if(!isdigit(*optarg) || (m = atoi(optarg)) < 0 || (m = atoi(optarg)) > 18) {
					perror("oss.c: error: invalid max process");
					usage();
					exit(EXIT_FAILURE);
				}
				break;
			default:
				perror("oss.c: error: invalid argument");
				usage();
				exit(EXIT_FAILURE);
		}
		
	}
	
	logCheck(fileName);
	allocation();
	
	setTimer(SECONDS);
	
	sleep(10);
	
	printf("m: %d\n", m);
	printf("fileName: %s\n", fileName);	
	
	fprintf(fp, "i like dogs\n");
	
	shared->pcb.ptable.delimiter = 15;
	
	fprintf(fp, "delimter value: %d\n", shared->pcb.ptable.delimiter);
	
	fclose(fp);
	
	releaseSharedMemory();
	deleteMessageQueues();
	
	return EXIT_SUCCESS;
}

void allocation() {

	allocateSharedMemory();
	allocateMessageQueues();
	
	shared = shmemPtr();
	
	pMsgQID = parentMsgQptr();
	cMsgQID = childMsgQptr();

}

void logCheck(char * file) {
	fp = fopen(file, "w");

	if(fp == NULL) {
		perror("oss.c: error: failed to open log file");
		exit(EXIT_FAILURE);
	}
}

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
