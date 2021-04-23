/*
 * Header file for shared memory
 * and message queues
 *
*/

#ifndef SHARED_H
#define SHARED_H

#define MAX_USER_PROCESS 18
#define MEMORY_SIZE 256

//Structure for Message Queue
typedef struct {
	long mtype;
	char mtext[100];
} Message;

//Structure for Simulated Clock
typedef struct {
	unsigned int sec;
	unsigned int ns;
} SimulatedClock;

//Structure for shared memory for PCB and simulated clock
//missing more stuff will figure out friday
typedef struct {
	int userPID[MAX_USER_PROCESS];
} SharedMemory;

//Structure for page table
typedef struct {
	int pages[32];
	int delimiter;
} PageTable;

//Structure for frame table
typedef struct {
	int frames[MEMORY_SIZE];
	int dirtyBit[MEMORY_SIZE];
	int secondChanceBit[MEMORY_SIZE];
} FrameTable;

void allocateSharedMemory();
void releaseSharedMemory();
void deleteSharedMemory();

void allocateMessageQueues();
void deleteMessageQueues();

SharedMemory* shmemPtr();
int childMsgQptr();
int parentMsgQptr();

#endif
