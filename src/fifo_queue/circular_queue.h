#ifndef __CIRCULAR_QUEUE__
#define __CIRCULAR_QUEUE__

#include "../app/typedefs.h"

typedef struct
{
	void* data;
	int capacity;
	int elementSize;
	int front;
	int rear;
}stQueue;

struct _stCirQueue
{
	stQueue *(*QueCreate)(int, int);			//create and return the queue	
	int (*QuePush)(stQueue *, void *);			//enque the data into queue,data is expected to a pointer to a heap allocated memory
	void *(*QuePop)(stQueue *);					//return the data from the que (FIFO) ,and free up all the internally allocated memory,but the user have to free the returning data pointer
	void (*QueDestroy)(stQueue *);				//destory the queue (free all the memory associate with the que even the data)
};

typedef struct _stCirQueue stCirQueue;
//stCirQueue LoginCirFifoQueue;
//stCirQueue CommandCirFifoQueue;

stCirQueue CirFifoQueue;


#endif
