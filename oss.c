//Tom Grossman
//CS4760 - Operating Systems
//Project 2
//04/08/17
//Copyright © 2017 Tom Grossman. All Rights Reserved.



#include "project3.h"

struct SharedMemory *shm;
int msgid_one, msgid_two, shmid;
FILE *fp;

int main(int argc, char* argv[]) {
	signal(SIGINT, signalHandler);

	char *fileName = "test.out";
	const char *PATH = "./user";
	
	int op, processes, totalProcesses,
		maxWrites = 3,
		slaveProcesses = 5,
		terminationTime = 20;

	// Taken from https://www.gnu.org/software/libc/manual/html_node/Example-of-Getopt.html#Example-of-Getopt
	// Modified to fit what I needed
	while ((op = getopt (argc, argv, "hs:l:t:")) != -1) {
		switch (op) {
			case 'h':
				printHelp();
				exit(EXIT_SUCCESS);
				
			case 's':
				if(isdigit(*optarg)) {
					if(atoi(optarg) < 21) {
						slaveProcesses = atoi(optarg);
					} else {
						printf("Cannot spawn more than 20 processes. Please try again with a number between 1-20... defaulting to 5 slaves\n");
					}
				} else {
					fprintf(stderr, "'-s' expects an integer value\n", optarg);
					exit(EXIT_FAILURE);
				}
				break;
				
			case 'l':
				fileName = optarg;
				break;

			case 't':
				if(isdigit(*optarg)) {
					terminationTime = atoi(optarg);
				} else {
					fprintf(stderr, "'-t' expects an integer value\n", optarg);
					exit(EXIT_FAILURE);
				}
				break;
				
			case '?':
				if(optopt == 's' || optopt == 'l' || optopt == 'i' || optopt == 't') {
					fprintf(stderr, "-%c requires an argument!\n", optopt);
				} else if(isprint(optopt)) {
					fprintf(stderr, "-%c is an unknown flag\n", optopt);
				} else {
					fprintf(stderr, "%s is unknown\n", argv[optind - 1]);
				}
				
			default:
				printf("-%c is not an option.\n", optarg);
				printHelp();
		}
	}

	if((shmid = shmget(key, sizeof(struct SharedMemory) * 2, IPC_CREAT | 0644)) < 0) {
		perror("shmget");
		fprintf(stderr, "shmget() returned an error! Program terminating...\n");
		exit(EXIT_FAILURE);
	}
	
    if((shm = (struct SharedMemory *)shmat(shmid, NULL, 0)) == (struct SharedMemory *) -1) {
		perror("shmat");
        fprintf(stderr, "shmat() returned an error! Program terminating...\n");
        exit(EXIT_FAILURE); 
    }
	
	// Message passing variables
    int msgflg = IPC_CREAT | 0666;
    struct msg_buf msgbuff_one, msgbuff_two;
	
    // Sending queue
    if ((msgid_one = msgget(send_key, msgflg)) < 0) {
        perror("msgget");
        exit(1);
    }
    
	// Receiving queue
    if ((msgid_two = msgget(recieve_key, msgflg)) < 0) {
        perror("msgget");
        exit(1);
    }
	
	pid_t pid, wpid, slaves[slaveProcesses];
	int index = 0,
		processesRemaining = 100,
		status;
		
	fp = fopen(fileName, "a");
	if(fp == NULL) {
		printf("Couldn't open file");
		errno = ENOENT;
		killAll();
		exit(EXIT_FAILURE);
	}


	// Start Clock
	clock_gettime(CLOCK_MONOTONIC, &shm->timeStart);
	clock_gettime(CLOCK_MONOTONIC, &shm->timeNow);
	shm->timePassedSec = shm->timeNow.tv_sec - shm->timeStart.tv_sec;
	shm->timePassedNansec = shm->timeNow.tv_nsec - shm->timeStart.tv_nsec / 1E9;

	while(1) {
		if((index < slaveProcesses) && (totalProcesses < 100)) {
			pid = fork();
			totalProcesses++;
			index++;
			
			if (pid == 0) {
			
			// Spawn slave process
			char *id;
			sprintf(id, "%i", index);

			//printf("Creating Slave Process\n");
			//https://linux.die.net/man/3/execl
			//execl(PATH, argv[0], argv[1], argv[2], argv[3])
			execl(PATH, id, fileName, (char *)NULL);

			//If child program exec fails, _exit()
			_exit(EXIT_FAILURE);
			}
			
		} else {
			if(!(msgrcv(msgid_one, &msgbuff_one, MSGSZ, 1, IPC_NOWAIT) < 0)) {
				index--;
				//printf("Msg recieved\n");
				
				//fflush(stdout);
				fprintf(fp, "Child %i is terminating at my time %i.%u because it reached %s\n", msgbuff_one.pid, shm->timePassedSec, shm->timePassedNansec, msgbuff_one.mtext);
				printf("Child %i is terminating at my time %i.%u because it reached %s\n", msgbuff_one.pid, shm->timePassedSec, shm->timePassedNansec, msgbuff_one.mtext);
				
				msgbuff_two.mType = msgbuff_one.pid;
				sprintf(msgbuff_two.mtext, "You were supposed to bring balance to the force %l, not leave it in darkness!", msgbuff_one.pid);
				
				if(msgsnd(msgid_two, &msgbuff_two, MSGSZ, IPC_NOWAIT) < 0) {
					perror("msgsnd");
					printf("The reply to child did not send\n");
					signalHandler();
				} //else {
				//	processesRemaining--;
				//}
				
				if(totalProcesses == 100) {
				//	if(processesRemaining == 0) {
						break;
				//	}
				}
			}
		}
		
		clock_gettime(CLOCK_MONOTONIC, &shm->timeNow);
		shm->timePassedSec = shm->timeNow.tv_sec - shm->timeStart.tv_sec;
		shm->timePassedNansec = shm->timeNow.tv_nsec - shm->timeStart.tv_nsec / 1E9;
		wpid = waitpid(-1, &status, WNOHANG);
		
		if(shm->timePassedSec > terminationTime) {
			printf("Termination time reached. Ending program...\n");
			signalHandler();
		}
	}
	
	// Let the children finish playing if the while loop exited normally
	// like a nice parent
	int status2;
	while ((wpid = wait(&status2)) > 0);
	
	killAll();
	printf("Exiting normally\n");
	sleep(1);
	return 0;
}

int slaveChecker(pid_t slaves[], int size) {
	
	int c;
	for(c = 0; c < size; c++) {
		pid_t wid;
		wid = waitpid(slaves[c], 0, WNOHANG);
		
		if(wid != 0) {
			slaves[c] = 0;
		}
	}
	
	for(c = 0; c < size; c++) {
		if(slaves[c] == 0)
			continue;
		else
			return 0;
	}	
		return 1;
}

void printHelp() {
	printf("\nCS4760 Project 3 Help!\n");
	printf("-h flag prints this help message\n");
	printf("-s [int x] will spawn x slave processes\n");
	printf("-l [string fileName] will change the output filename to something of your choosing\n");
	printf("-t [int x] will change how long the program can execute before being forcefully terminated\n\n");
}

void signalHandler() {
	//printf("Signal received... terminating master\n");
	msgctl(msgid_one, IPC_RMID, NULL);
	msgctl(msgid_two, IPC_RMID, NULL);
	shmdt(shm);
	shmctl(shmid, IPC_RMID, NULL);
    pid_t id = getpgrp();
    killpg(id, SIGINT);
	sleep(1);
    exit(EXIT_SUCCESS);
}

void killAll() {
	msgctl(msgid_one, IPC_RMID, NULL);
	msgctl(msgid_two, IPC_RMID, NULL);
	shmdt(shm);
	shmctl(shmid, IPC_RMID, NULL);
	fclose(fp);
}