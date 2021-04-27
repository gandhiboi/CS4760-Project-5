#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/msg.h>

#include "shared.h"

//variables for shmem and msg q
SharedMemory * shared = NULL;
Message msg;

//message queue ids
static int pMsgQID;
static int cMsgQID;

void allocation();

int main(int argc, char * argv[]) {

	allocation();								//allocates shared memory/message queues
	pid_t cPID = getpid();
	srand(cPID);								//seeded wtih pid
	
	int location = atoi(argv[1]);
	int terminateChance;							//term chance 10% or 15% idk yet
	int segFaultChance;							//seg fault 5% chance
	int readChance;							//70% chance read
	int memoryReferenceFlag;						//either read or write; read: 1, write: 0
	int accessPage;							//page being accessed
	int numPages = (rand() % 32) + 1;					//determines number of pages needed
	
	int i;
	for(i = 0; i < 31; i++) {
		shared[location].pcb.ptable.pages[i] = -1;
	}
	
	shared[location].pcb.ptable.delimiter = numPages;
	
	//printf("user.c: Process %d has %d pages\n", getpid(), shared[location].pcb.ptable.delimiter);
	
	//sets basic msg q components
	msg.mtype = cPID;
	msg.pid = cPID;
	msg.address = location;
	
	while(1) {
	
		//generates random amount to see percent read/write, seg fault, and termination
		readChance = (rand() % 100) + 1;
		segFaultChance = (rand() % 100) + 1;
		terminateChance = (rand() % 100) + 1;
	
		//15% (atm) TO TERMINATE AND WILL SEND MSG TO OSS THAT ITS TERMINATING
		if(terminateChance > 90) {
			msg.terminate = true;
			msg.mtype = getppid();
			msgsnd(cMsgQID, &msg, sizeof(Message), 1);
			break;
			
		}
		
		//SIMULATES CHANCE FOR SEGFAULT 5%; ELSE ACCESS RANDOM PAGE
		if(segFaultChance > 99) {
			shared->segFault += 1;
		}
		else {
			accessPage = (rand() % numPages) + 1;
		}
		
		//DETERMINES IF IT WILL EITHER READ OR WRITE; SENDS MESSAGE VIA MSG Q TO OSS
		if(readChance < 30) {
			memoryReferenceFlag = 0;				//write
		}
		else {
			memoryReferenceFlag = 1;				//read
		}
		
		
		msg.page = accessPage;
		msg.readOrWrite = memoryReferenceFlag;
		msg.mtype = getppid();
		
		//sends message to oss with respective information for frame table
		msgsnd(cMsgQID, &msg, sizeof(Message), 1);
		
		//process is blocked until a message is received from OSS 
		msgrcv(pMsgQID, &msg, sizeof(Message), cPID, 0);
		
	}
	
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
