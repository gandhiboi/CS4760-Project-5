#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "shared.h"

//variables for shmem and msg q
SharedMemory * shared = NULL;
Message msg;

//message queue ids
static int pMsgQID;
static int cMsgQID;

void allocation();

int main(int argc, char * argv[]) {

	allocation();							//allocates shared memory/message queues
	int cPID = getpid();
	srand(cPID);							//seeded wtih pid
	
	int terminate;							//term chance 10% or 15% idk yet
	int segFault;							//seg fault 5% chance
	int read;							//70% chance read, 30% chance write
	int write;
	int memoryReference;						//either read or write; read: 1, write: 0
	int accessPage;						//page being accessed
	int numPages = (rand() % 32);
	int temp = 0;
	bool flag = true;
	
	while(flag) {
	
		temp++;
		printf("user.c: temp: %d\n", temp);
	
		read = (rand() % 100) + 1;
		write = (rand() % 100) + 1;
		segFault = (rand() % 100) + 1;
		terminate = (rand() % 100) + 1;
	
		//15% (atm) to terminate and WILL SEND MSG TO OSS THAT ITS TERMINATING
		if(terminate > 85) {
			printf("user.c: %d is terminating\n", cPID);
		}
		
		//SIMULATES CHANCE FOR SEGFAULT 5%; ELSE ACCESS RANDOM PAGE
		if(segFault > 95) {
			accessPage = numPages + 1;
		}
		else {
			accessPage = (rand() % numPages);
		}
		
		if(write < 30) {
			memoryReference = 0;
		}
		else if (read > 30) {
			memoryReference = 1;
		}
		
		if(temp >= 10) {
			flag = false;
		}

	
	}
	
	printf("user.c: delimter value: %d\n", shared->pcb.ptable.delimiter);
	
	releaseSharedMemory();
	deleteMessageQueues();
	
	return EXIT_SUCCESS;

}

//allocates/attaches to shared memory
void allocation() {

	allocateSharedMemory();
	allocateMessageQueues();
	
	shared = shmemPtr();
	
	pMsgQID = parentMsgQptr();
	cMsgQID = childMsgQptr();

}
