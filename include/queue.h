
#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

#define MAX_QUEUE_SIZE 10

//priority: the lower value priority, the higher priority a process have.

struct queue_t {
	struct pcb_t * proc[MAX_QUEUE_SIZE];
	int size;
};

void enqueue(struct queue_t * q, struct pcb_t * proc);

struct pcb_t * dequeue(struct queue_t * q);

//transfer all data from src to des, and also empty src queue.
void tranfer_queue(struct queue_t * des, struct queue_t * src);

int empty(struct queue_t * q);

#endif

