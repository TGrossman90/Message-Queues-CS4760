#include "project3.h"

int send_id, recieve_id, shmid;
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
	
	fflush(stdout);
	
	if((recieve_id = msgget(recieve_key, 0666)) < 0) {
		fprintf(stderr, "Child %i has failed attaching the recieve queue", pid);
		killAll();
		exit(EXIT_FAILURE);
	}
	
	if((send_id = msgget(send_key, 0666)) < 0) {
		fprintf(stderr, "Child %i has failed attaching the sending queue", pid);
		killAll();
		exit(EXIT_FAILURE);
	}
	
	
	
	// **Critical Section**
	struct timespec tm, timePassed;
	clock_gettime(CLOCK_MONOTONIC, &tm);
	srand((unsigned)(tm.tv_sec ^ tm.tv_nsec ^ (tm.tv_nsec >> 31)));
	int duration = (rand() % 100001);
	//printf("PID: %i Duration: %i\n", pid, duration);
	
	clock_gettime(CLOCK_MONOTONIC, &timePassed);
	//printf("Current Time: %i\nDuration Time: %i\nTime+Duration: %i\n\n\n", shm->timePassedNansec, duration, shm->timePassedNansec + duration);
	while(shm->timePassedNansec < (shm->timePassedNansec + duration));
	//printf("Process %i is terminating.\n", pid);
	
	struct msg_buf sendbuf, recievebuf;
	
	sendbuf.mType = 1;
	sendbuf.pid = pid;
	sprintf(sendbuf.mtext, "%i", duration);
	
	if(msgsnd(send_id, &sendbuf, MSGSZ, IPC_NOWAIT) < 0) {
		perror("msgsnd");
		fprintf(stderr, "msgsnd retuned an error in slave.c pid: %i. Program terminating...\n", pid);
		killAll();
		exit(EXIT_FAILURE);
	}
	
	while(msgrcv(recieve_id, &recievebuf, MSGSZ, pid, 0) < 0);
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
	//printf("Signal received... terminating slave with PID: %i\n", id);
	shmdt(shm);
    kill(id, SIGKILL);
}

// Release shared memory
void killAll() {
	shmdt(shm);
	signalHandler();
}
