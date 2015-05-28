/***********************************************************************
* $Id$
* Project:	 switch_network
* File:		 fifo_queue/circular_queue.c		 
* Description: brief Part of circular queue implementation.
*
* @version:		V1.0.0
* @date:		9th October 2014
*
* Copyright (C) 2014 SFARD Co., Ltd. All rights reserved.
*-----------------------------------------------------------------
*
* Revise:  
* 		 date		 version 		author 		description
*  		2014/10/09	 1.0.0		zhengchao	 A simple FIFO circular queue implementation
*
*
*
**********************************************************************/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "circular_queue.h"
#include "../app/debug.h"
#include "../ap_log.h"

int isFull(stQueue* queue)
{
	if(queue->front == queue->rear) 
		return 1;
	
	return 0;
}

int isEmpty(stQueue* queue)
{
	if(queue->front == -1) 
		return 1;

	return 0;
}

stQueue* cirqueue_create(int capacity, int elementSize)
{
	stQueue* queue = calloc(1, sizeof(stQueue));
	queue->data = calloc(capacity, elementSize);
	queue->capacity = capacity;
	queue->elementSize = elementSize;
	queue->front = -1;
	queue->rear = 0;
	
	GetLocalTimeLog();
	ap_log_debug_log("\t\t%s, %s, %d, cirqueue_create Generated the que!\r\n",  
						__FILE__, __FUNCTION__, __LINE__);

	return queue;
}

int cirqueue_push(stQueue* queue, void* element)
{
	if(isFull(queue)) 
		return 1;
	if(queue->front == -1)
		queue->front = 0;
	memcpy(queue->data+(queue->rear*queue->elementSize),element,queue->elementSize);
	//DEBUGMSG(1,("+++rear:%d, rear*elementSize:%d, element:%d\r\n", queue->rear, queue->rear*queue->elementSize, *(int *)element));
	queue->rear =(queue->rear + 1)%queue->capacity;
	
	return 0;
}

void* cirqueue_pop(stQueue *queue)
{
	void *result;
	
	if(isEmpty(queue)) 
		return NULL;
	result = calloc(1, queue->elementSize);
	memcpy(result, queue->data+(queue->front*queue->elementSize), queue->elementSize);
	memset(queue->data+(queue->front*queue->elementSize), 0, queue->elementSize);
	DEBUGMSG(0,("---front:%d, front*elementSize:%d, result:%s\r\n", queue->front, queue->front*queue->elementSize, (char *)result));
	queue->front = (queue->front + 1)%queue->capacity;
	if(queue->front == queue->rear)
	{
		queue->front = -1;
		queue->rear = 0;
	}
	
	return result;
}

void cirqueue_destroy(stQueue* queue)
{
	free(queue->data);
	free(queue);
}

/*stCirQueue LoginCirFifoQueue = {
	.QueCreate = cirqueue_create,
	.QueDestroy = cirqueue_destroy,
	.QuePush = cirqueue_push,
	.QuePop = cirqueue_pop,
};

stCirQueue CommandCirFifoQueue = {
	.QueCreate = cirqueue_create,
	.QueDestroy = cirqueue_destroy,
	.QuePush = cirqueue_push,
	.QuePop = cirqueue_pop,
};*/

stCirQueue CirFifoQueue = {
	.QueCreate = cirqueue_create,
	.QueDestroy = cirqueue_destroy,
	.QuePush = cirqueue_push,
	.QuePop = cirqueue_pop,
};



