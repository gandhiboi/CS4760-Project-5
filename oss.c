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
#include <time.h>
#include <string.h>

#include "shared.h"
#include "queue.h"

//macros
#define MAX_PROCESS 18
#define SECONDS 20

//tracker var
pid_t pid;
int numLines;
int numSwaps = 0;

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
int numMemoryAccess;
int numPageFaults;

//function prototypes
void spawnUser(int);
void memoryManager();
void initPCB();
void logCheck(char*);
void usage();
void allocation();
void setTimer(int);
void signalHandler(int);
void displayStatistics();
void displayMemoryMap();
void incrementSimClock(SimulatedClock*, int);

int main(int argc, char* argv[]) {

	//signal handler for timer and ctrl+c
	signal(SIGINT, signalHandler);
	signal(SIGALRM, signalHandler);
	srand(time(NULL));

	//basic control variables for program
	int opt;
	int m = 18;
	char * fileName = "output.log";
	
	//variable for number of lines in log file
	numLines = 0;
	
	//sets max option for processes
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
	
	//checks to see if log file can be opened
	logCheck(fileName);
	
	//allocates/attaches to shared memory and sets up message queues
	allocation();
	
	//initializes variables in process control block to default
	initPCB();

	shared->simClock.sec = 0;
	shared->simClock.ns = 0;
	
	//printf("simClock: %d:%d\n", shared->simClock.sec, shared->simClock.ns);
	
	setTimer(SECONDS);
	
	//printf("m: %d\n", m);
	//printf("fileName: %s\n", fileName);
	
	memoryManager();
	
	fprintf(fp, "Max lines reached in 'output.log': terminating program");
	printf("==============================================\n");
	
	displayMemoryMap();
	displayStatistics();
	
	fclose(fp);
	releaseSharedMemory();
	deleteMessageQueues();
	
	return EXIT_SUCCESS;
}

//responsible for doing all the work
void memoryManager() {

	int i;
	bool active, full;
	numPageFaults = 0; 

	while(1) {
		
		//generates and increments clock random time of 1-500 ms
		int randomIncrement = (rand() % 500000000) + 1;
		incrementSimClock(&(shared->simClock), randomIncrement);
		
		//spawn flag is used to determine if a user process will be launched
		bool spawn = false;
		
		//searches the pids array to see if there is an open spot; if so then fork
		for(i = 0; i < MAX_PROCESS; i++) {
			if(pids[i] == 0) {
				spawn = true;
				pid = fork();
				break;
			}
		}
			
		//if flag is set to true, then it will call spawnUser to launce user.c
		if(spawn == true) {
			if(pid == 0) {
				spawnUser(i);
			}
			else if(pid == -1) {
				perror("oss.c: error: failed to complete fork");
				exit(EXIT_FAILURE);
			}
			
			//sets pid to pid array and prints the relative information to the log file
			pids[i] = pid;
			fprintf(fp, "OSS: P%d created [%d:%d]\t Local Address: %d\n", pid, shared->simClock.sec, shared->simClock.ns, i);
			numLines++;
		}
		
		//loop because msg doesn't get received in time; did same thing in last project
		int q = 0;
		while(q<9999999) q++;
		
		msgrcv(cMsgQID, &msg, sizeof(Message), getpid(), 1);
		
		//receiving message from user implies that it has done a memory access
		numMemoryAccess++;
		
		full = false;								//flag to see if the the frame table is full or not
		active = false;							//flag to see if the memory access is in the frame table
		
		fprintf(fp, "OSS: P%d referencing page %d\t Delimiter: %d\n", msg.pid, msg.page, shared[msg.address].pcb.ptable.delimiter);
		numLines++;
		
		//if msg q sends true, then send terminate signal to the pid
		if(msg.terminate == true) {
			fprintf(fp, "OSS: P%d has terminated.\n", msg.pid);
			numLines++;

			pids[msg.address] = 0;
			kill(msg.pid, SIGTERM);
			wait(NULL);
			
			continue;
		}
		
		//checks whether there is a page fault or not
		if(shared[msg.address].pcb.ptable.pages[msg.page] != -1) {
			active = true;						//sets flag active to true 
			
			fprintf(fp, "OSS: P%d is referencing page in memory. Reference type is %d\n", msg.pid, msg.readOrWrite);	
			numLines++;
			
			ftable[shared[msg.address].pcb.ptable.pages[msg.readOrWrite]].referenceBit = 1;
			
			if(msg.readOrWrite == 0) {
				fprintf(fp,"OSS: P%d requesting write of page %d at time [%d:%d]\n", msg.pid, msg.page, shared->simClock.sec, shared->simClock.ns);
				numLines++;

				ftable[shared[msg.address].pcb.ptable.pages[msg.readOrWrite]].dirtyBit = 1;
				ftable[shared[msg.address].pcb.ptable.pages[msg.readOrWrite]].SC = 1;
			}
			else {
				fprintf(fp,"OSS: P%d requesting read request of page %d at time [%d:%d]\n", msg.pid, msg.page, shared->simClock.sec, shared->simClock.ns);
				numLines++;

				ftable[shared[msg.address].pcb.ptable.pages[msg.readOrWrite]].dirtyBit = 0;
			}
			
			//no page fault, therefore increment by 10 ns
			incrementSimClock(&(shared->simClock), 10);
		}
		
		if(!active) {
			numPageFaults++;
			for(i = 0; i < 256; i++) {
				if(ftable[i].frames == "No") {
					//used for memory map purposes and to keep track of bits for second chance policy
					ftable[i].frames = "Yes";
					ftable[i].pid = msg.pid;					
					ftable[i].referenceBit = 0;
					ftable[i].dirtyBit = 0;
					
					enqueue(fifoQ, i);						//queues up the pid that should be put in frame table
					shared[msg.address].pcb.ptable.pages[msg.address] = i;		
					
					fprintf(fp, "OSS: Page fault; adding %d to queue\n", i);
					fprintf(fp, "OSS: P%d page %d written to frame %d in frame table at time [%d:%d]\n", msg.pid, msg.address, i, shared->simClock.sec, shared->simClock.ns);
					numLines += 2;
					
					incrementSimClock(&(shared->simClock), 15000000);
					break;
				}
				else if(i == 255) {
					full = true;				//sets the flag to true if there is no space in the frame table
				}
			}
			
			//deals with when the frame table is full
			if(full) {
				int replace;							//var for the frame that will be replaced
				fprintf(fp, "OSS: Looking for frame to swap...\n");
				bool swap = true;
				int toSwap;							//used to swap the frames
				numLines++;				

				//swaps the first object at the fifo q and then requeues the frame trying to be placed in the
				//frame table to the end
				do {
					toSwap = front(fifoQ);
					dequeue(fifoQ);
					
					//selects the frame that will be swapped by checking if the dirty bit is set to 0
					if(ftable[toSwap].referenceBit == 0) {
						if(ftable[toSwap].dirtyBit == 0) {
							replace = toSwap;
							fprintf(fp, "OSS: Selected frame %d for swapping\n", replace);
							swap = false;
							break;
						}
					}
					
					//increments the clock for having to swap the dirty bit
					if(ftable[toSwap].dirtyBit == 1) {
						ftable[toSwap].dirtyBit = 0;
						fprintf(fp, "OSS: Dirty bit of frame %d set, adding additional time to the clock", replace);
						incrementSimClock(&(shared->simClock), 15000000);
					}
					
					//reference bit gets flipped due to being re-referenced
					if(ftable[toSwap].referenceBit == 1) {
						ftable[toSwap].referenceBit = 0;
					}
					//queues up the frame to be swapped
					enqueue(fifoQ, toSwap);
				} while(swap);
				
				fprintf(fp, "OSS: Frame %d being swapped at time [%d:%d]\n", replace, shared->simClock.sec, shared->simClock.ns);
				numLines++;
				numSwaps++;
				
				//resets the number of bits and pages to be used; also increments the clock for having to perform those operations
				shared[msg.address].pcb.ptable.pages[replace] = -1;
				incrementSimClock(&(shared->simClock), 15000000);
				ftable[replace].frames = "Yes";
				ftable[replace].pid = msg.pid;
				ftable[replace].referenceBit = 0;
				ftable[replace].dirtyBit = 0;
				
				shared[msg.address].pcb.ptable.pages[msg.page] = replace;
				fprintf(fp, "OSS: P%d page %d swapped to frame %d at time [%d:%d]\n", msg.pid, msg.page, replace, shared->simClock.sec, shared->simClock.ns);
				enqueue(fifoQ, replace);
				numLines++;
			}
			
			//sends a message to the wake the proper pid
			msg.mtype = msg.pid;
			if(msgsnd(pMsgQID, &msg, sizeof(Message), 0) == -1) {
				perror("oss.c: error: failed to send message to user process\n");
			}
		}
		
		//if number of lines is equal to or exceeds 1000, then break
		if(numLines >= 1000) {
			break;
		}
	}
	

}

//increments the simulated clock
void incrementSimClock(SimulatedClock* timeStruct, int increment) {
	int nanoSec = timeStruct->ns + increment;
	
	while(nanoSec >= 1000000000) {
		nanoSec -= 1000000000;
		(timeStruct->sec)++;
	}
	timeStruct->ns = nanoSec;
}

//displays the required statistics
void displayStatistics() {
	float fNumMemoryAccess = numMemoryAccess;
	float fNumPageFaults = numPageFaults;
	float fNumSegFaults = shared->segFault;

	numMemoryAccessPerSec = fNumMemoryAccess / shared->simClock.sec;
	numPageFaultsPerMemoryAccess = fNumPageFaults / fNumMemoryAccess;
	numSegFaultsPerMemoryAccess = fNumSegFaults / fNumMemoryAccess;

	printf("==============================================\n");
	printf("\t\tMEMORY STATISTICS\n");
	printf("Simulated System Clock: %d:%d\n", shared->simClock.sec, shared->simClock.ns);
	printf("Number of Segmentation Faults: %d\n", shared->segFault);
	printf("Number of Swaps: %d\n", numSwaps);
	printf("Number of Page Faults: %d\n", numPageFaults);
	printf("Number of Memory Accesses Per Second: %f\n", numMemoryAccessPerSec);
	printf("Number of Page Faults Per Memory Access: %f\n", numPageFaultsPerMemoryAccess);
	printf("Average Memory Access Speed: %f\n", fNumMemoryAccess);
	printf("Average Number of Seg Faults Per Memory Access: %f\n", fNumSegFaults / fNumMemoryAccess);
	printf("==============================================\n");
}

//displays the memory map
void displayMemoryMap() {
	int i;
	printf("Current Memory Layout at Time %d:%d is:\n", shared->simClock.sec, shared->simClock.ns);
	printf("\t\tOccupied\tDirtyBit\tSC\n");
	for(i = 0; i < 256; i++) {
		printf("Frame %d:\t%s\t\t%d\t\t%d\n", i, ftable[i].frames, ftable[i].dirtyBit, ftable[i].SC);
	}
}

//responsible for launching user process if it can be launched
void spawnUser(int index) {
	int length = snprintf(NULL, 0, "%d", index);
	char * xx = (char*)malloc(length + 1);
	snprintf(xx, length + 1, "%d", index);
	
	execl("./user", "user", xx, (char*)NULL);
	
	free(xx);
	xx = NULL;
}

//initializes frame table values to defaults
void initPCB() {
	int i;
	for(i = 0; i < 256; i++) {
		ftable[i].frames = "No";
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
