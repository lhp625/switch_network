#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "../app/typedefs.h"

#ifndef FIFO_QUEUE_H
#define FIFO_QUEUE_H

struct _node 
{
	void *data;
	struct _node * next;
};
typedef struct _node node;
  
struct _end_q 
{
	node * first;
	node * last;
};
typedef struct _end_q queue;

struct _stFifoQuque
{
	pthread_mutex_t *QueLock;

	void (*QueInit)();
	queue *(*QueCreate)(void);									//create and return the queue	
	void (*QueDestroy)(queue *, pthread_mutex_t *);				//destory the queue (free all the memory associate with the que even the data)
	int (*QuePush)(queue *, void *, pthread_mutex_t *);			//enque the data into queue,data is expected to a pointer to a heap allocated memory
	void *(*QuePop)(queue *, pthread_mutex_t *);				//return the data from the que (FIFO) ,and free up all the internally allocated memory,but the user have to free the returning data pointer
	void (*QueMalloc)(char **, int);
	void (*QueFree)(char *);
};
typedef struct _stFifoQuque stFifoQuque;

stFifoQuque LoginFifoQueue;
stFifoQuque CommandFifoQueue;

#endif

