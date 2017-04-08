#ifndef PROJECT2_H
#define PROJECT2_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/ipc.h>

#define MSGSZ 128

void signalHandler();
void killAll();
void printHelp();

struct SharedMemory {
	struct timespec timeStart, timeNow, timePassed;
	int timePassedSec;
	int timePassedNansec;
};

struct msg_buf {
	long mType;
	int pid;
	char mtext[MSGSZ];
};

enum state {idle, want_in, in_cs};

key_t key = 1337;
key_t send_key = 7545;
key_t recieve_key = 5457;

#endif