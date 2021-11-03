#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

//we'll try to construct a MAX-heap. the top of the heap is the highest
//priority process that we need to execute.

int empty(struct queue_t * q) {
	return (q->size == 0);
}

void max_heap_reheap_up(struct queue_t * q, int index) {
	int parent_index;
	struct pcb_t * temp;
	while (index > 0) {
		parent_index = (index - 1)/2;
		if (q->proc[index]->priority > q->proc[parent_index]->priority) {
			//swap
			temp = q->proc[index];
			q->proc[index] = q->proc[parent_index];
			q->proc[parent_index] = temp;
			//new index
			index = parent_index;
		} else return;
	}
}

void max_heap_reheap_down(struct queue_t * q, int index) {
	 int size = q->size;
	 struct pcb_t * temp;
	 while (index < size) {
        int lc = (2 * index + 1) < size ? (2 * index + 1) : -1;
        int rc = (2 * index + 2) < size ? (2 * index + 2) : -1;
        int maxChild = lc;
        if (lc == -1) {		//if left child does not exist
            return;
        }
        else if (rc == -1) {	//if it have a lc but don't have rc.
            maxChild = lc;
        }
        else { //lc != -1 && rc != -1
            maxChild = (q->proc[lc]->priority >= q->proc[rc]->priority) ? lc : rc;
        }
        if (q->proc[maxChild]->priority >= q->proc[index]->priority) {
			//swap
           	temp = q->proc[index];
			q->proc[index] = q->proc[maxChild];
			q->proc[maxChild] = temp;
			//new index
            index = maxChild;
        }
        else {
            return;
        }
    }
}

void enqueue(struct queue_t * q, struct pcb_t * proc) {
	/* TODO: put a new process to queue [q] */	
	if (q->size < MAX_QUEUE_SIZE) {
		q->proc[q->size] = proc;
		q->size++;
		max_heap_reheap_up(q, q->size - 1);
	}
}

struct pcb_t * dequeue(struct queue_t * q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	//neu cai queue khong co gi ca thi dequeue lam gi?
	if (q->size == 0) return NULL;
	struct pcb_t* return_val = q->proc[0];

	//bring the last process in the heap to the top:
	q->proc[0] = q->proc[q->size-1];
	q->size--;
	max_heap_reheap_down(q, 0);
	return return_val;
}

void tranfer_queue(struct queue_t * des, struct queue_t * src) {
	int size = src->size;
	for(int i = 0; i < size; i++) {
		des->proc[i] = src->proc[i];
		src->proc[i] = NULL;
	}
	des->size = size;
	src->size = 0;
}

