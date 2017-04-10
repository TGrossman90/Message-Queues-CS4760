//Tom Grossman
//CS4760 - Operating Systems
//Project 3 - Message Queues
//04/10/17
//Copyright © 2017 Tom Grossman. All Rights Reserved.

#include "project3.h"

int msgid_one, msgid_two, shmid;
struct SharedMemory *shm;
char *fileName;

int main(int argc, char* argv[]) {
	// Signal Handler
	signal(SIGINT, signalHandler);	
	pid_t pid = getpid();
	
	// Get passed arguments from execl()
	int i = atoi(argv[0]);	
	fileName = argv[1];
	
	
	// Get shared memory id that was created by the master process	
	if((shmid = shmget(key, sizeof(struct SharedMemory) * 2, 0666)) < 0) {
		perror("shmget");
		fprintf(stderr, "Child: shmget() returned an error! Program terminating...\n");
		killAll();
		exit(EXIT_FAILURE);
	}
	
	// Attach the shared memory to the slave process
    if ((shm = (struct SharedMemory *)shmat(shmid, NULL, 0)) == (struct SharedMemory *) -1) {
		perror("shmat");
        fprintf(stderr, "shmat() returned an error! Program terminating...\n");
		killAll();
        exit(EXIT_FAILURE);
    }
	
	//fflush(stdout);
	
	if((msgid_two = msgget(recieve_key, 0666)) < 0) {
		fprintf(stderr, "Child %i has failed attaching the recieve queue", pid);
		killAll();
		exit(EXIT_FAILURE);
	}
	
	if((msgid_one = msgget(send_key, 0666)) < 0) {
		fprintf(stderr, "Child %i has failed attaching the sending queue", pid);
		killAll();
		exit(EXIT_FAILURE);
	}
	
	
	
	// **Critical Section**
	struct timespec tm, timePassed;
	clock_gettime(CLOCK_MONOTONIC, &tm);
	
	srand((unsigned)(tm.tv_sec ^ tm.tv_nsec ^ (tm.tv_nsec >> pid)));
	int duration = (rand() % 100001);
	//printf("PID: %i Duration: %i\n", pid, duration);
	
	clock_gettime(CLOCK_MONOTONIC, &timePassed);
	//printf("Current Time: %i\nDuration Time: %i\nTime+Duration: %i\n\n\n", shm->timePassedNansec, duration, shm->timePassedNansec + duration);
	while(shm->timePassedNansec < (shm->timePassedNansec + duration));
	
	struct msg_buf sendbuf, msgbuff_two;
	
	sendbuf.mType = 1;
	sendbuf.pid = pid;
	sprintf(sendbuf.mtext, "%i.%u", shm->timePassedSec, shm->timePassedNansec);
	
	if(msgsnd(msgid_one, &sendbuf, MSGSZ, IPC_NOWAIT) < 0) {
		perror("msgsnd");
		fprintf(stderr, "msgsnd retuned an error in slave.c pid: %i. Program terminating...\n", pid);
		killAll();
		exit(EXIT_FAILURE);
	}
	
	// Make process wait until message comes
	while(msgrcv(msgid_two, &msgbuff_two, MSGSZ, pid, 0) < 0);
	//printf("Child Message recieved\n");
	
	//printf("Process %i is terminating.\n", pid);
	//msgrcv(msqid, &msg, sizeof msg.mID, 1, 0);
	//printf("Received message from child with PID %i\n", msg.mID);
	
	// Cleanup and Exit
	killAll();
	exit(3);
}

// Kills all when signal is received
void signalHandler() {
    pid_t id = getpid();
	printf("Signal received... terminating slave with PID: %i\n", id);
	killAll();
    killpg(id, SIGINT);
    exit(EXIT_SUCCESS);
}

// Release shared memory
void killAll() {
	shmdt(shm);
}