/***********************************************************************
* $Id$
* Project:	 switch_network  
* File:list/list.c		 
* Description: brief Part of list. 
*
* @version:		V1.0.0
* @date:		20th August 2014
*
* Copyright (C) 2014 SFARDS Co., Ltd. All rights reserved.
*-----------------------------------------------------------------
*
* Revise:  
* 		 date		 version 		author 		description
*  		2014/08/20	 1.0.0		zhengchao	create list
*
*
*
**********************************************************************/


#include "list.h"
#include "../app/debug.h"
#include <stdio.h>
//#include "../api_comm/api_login.h"

/*pthread_mutex_t gMinerListLenLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCellListLenLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gMinerRemoveLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCellRemoveLock = PTHREAD_MUTEX_INITIALIZER;*/

pthread_mutex_t gDelCellIdListLenLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCellIdRemoveLock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t gBTCPoolConfListLenLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gBTCPoolConfRemoveLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gLTCPoolConfListLenLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gLTCPoolConfRemoveLock = PTHREAD_MUTEX_INITIALIZER;

//pthread_mutex_t gPoolConfListLenLock = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t gPoolConfRemoveLock = PTHREAD_MUTEX_INITIALIZER;


/*
 * Allocate a new list_t. NULL on failure.
 */

list_t *list_new()
{
	list_t *self;
	
	if (!(self = LIST_MALLOC(sizeof(list_t))))
		return NULL;
	
	self->head = NULL;
	self->tail = NULL;
	self->free = NULL;
	self->match = NULL;
	self->len = 0;
	
	return self;
}

/*
 * Free the list.
 */

void list_destroy(list_t *self)
{
	unsigned int len = self->len;
	list_node_t *next;
	list_node_t *curr = self->head;

	while (len--)
	{
		next = curr->next;
		if (self->free) self->free(curr->val);
		LIST_FREE(curr);
		curr = next;
	}

	LIST_FREE(self);
}

/*
 * Append the given node to the list
 * and return the node, NULL on failure.
 */

list_node_t *list_rpush(list_t *self, list_node_t *node, pthread_mutex_t *ListLenLock)
{	
	if (!node) 
		return NULL;

	if (self->len)
	{
		node->prev = self->tail;
		node->next = NULL;
		self->tail->next = node;
		self->tail = node;
	} 
	else 
	{
		self->head = self->tail = node;
		node->prev = node->next = NULL;
	}
	pthread_mutex_lock(ListLenLock);
	++self->len;
	pthread_mutex_unlock(ListLenLock);
	
	return node;
}

/*
 * Return / detach the last node in the list, or NULL.
 */

list_node_t *list_rpop(list_t *self)
{
	if (!self->len) return NULL;

	list_node_t *node = self->tail;

	if (--self->len)
	{
		(self->tail = node->prev)->next = NULL;
	} 
	else 
	{
		self->tail = self->head = NULL;
	}

	node->next = node->prev = NULL;
	return node;
}

/*
 * Return / detach the first node in the list, or NULL.
 */

list_node_t *list_lpop(list_t *self)
{
	if (!self->len) return NULL;

	list_node_t *node = self->head;

	if (--self->len)
	{
		(self->head = node->next)->prev = NULL;
	} 
	else 
	{
		self->head = self->tail = NULL;
	}

	node->next = node->prev = NULL;
	
	return node;
}

/*
 * Prepend the given node to the list 
 * and return the node, NULL on failure.
 */

list_node_t *list_lpush(list_t *self, list_node_t *node)
{
	if (!node) return NULL;

	if (self->len) 
	{
		node->next = self->head;
		node->prev = NULL;
		self->head->prev = node;
		self->head = node;
	} 
	else 
	{
		self->head = self->tail = node;
		node->prev = node->next = NULL;
	}

	++self->len;
	return node;
}

/*
 * Return the node associated to val or NULL.
 */

list_node_t *list_find(list_t *self, void *val)
{
	list_iterator_t *it = list_iterator_new(self, LIST_HEAD);
	list_node_t *node;

	while ((node = list_iterator_next(it)))
	{
		if (self->match)
		{
			DEBUGMSG(0,("222 %s...\r\n", __FUNCTION__));
			if (self->match(val, node->val)) 
			{
				/*DEBUGMSG(0,("val:%s, node->val:%s, %d, @:%p\r\n", 
					(char *)val, (char *)node->val,
					((stCellConn *)node->val)->CellRes.iConnCellIndex, &((stCellConn *)node->val)->CellRes.iConnCellIndex));
				*/
				list_iterator_destroy(it);
				return node;
			}
		}
		else
		{
			if (val == node->val)
			{
				list_iterator_destroy(it);
				return node;
			}
		}
	}

	list_iterator_destroy(it);
	return NULL;
}

/*
 * Return the node at the given index or NULL.
 */

list_node_t *list_at(list_t *self, int index)
{
	list_direction_t direction = LIST_HEAD;

	if (index < 0)
	{
		direction = LIST_TAIL;
		index = ~index;
	}

	if (index < self->len) 
	{
		list_iterator_t *it = list_iterator_new(self, direction);
		list_node_t *node = list_iterator_next(it);
		while (index--) node = list_iterator_next(it);
		list_iterator_destroy(it);
		return node;
	}

	return NULL;
}

/*
 * Remove the given node from the list, freeing it and it's value.
 */

void list_remove(list_t *self, list_node_t *node) 
{
	node->prev
		? (node->prev->next = node->next)
		: (self->head = node->next);

	node->next
		? (node->next->prev = node->prev)
		: (self->tail = node->prev);

	if (self->free) self->free(node->val);

	LIST_FREE(node);
	--self->len;
}

/*structList stMinerList = {
	//.MinerList = NULL,
	.ListLenLock = &gMinerListLenLock,
	.RemoveLock = &gMinerRemoveLock,
	.ListNodeNew = list_node_new,
	.ListNew = list_new,
	.ListRpush = list_rpush,
	.ListLpush = list_lpush,	
	.ListFind = list_find,
	.ListAt = list_at,	
	.ListRpop = list_rpop,
	.ListLpop = list_lpop,
	.ListRemove = list_remove,
	.ListDestroy = list_destroy,

	.ListIteratorNew = list_iterator_new,
	.ListIteratorNewFromNode = list_iterator_new_from_node,
	.ListIteratorNext = list_iterator_next,
	.ListIteratorDestroy = list_iterator_destroy
};

structList stCellList = {
	//.CellList = NULL,
	.ListLenLock = &gCellListLenLock,
	.RemoveLock = &gCellRemoveLock,
	.ListNodeNew = list_node_new,
	.ListNew = list_new,
	.ListRpush = list_rpush,
	.ListLpush = list_lpush,	
	.ListFind = list_find,
	.ListAt = list_at,	
	.ListRpop = list_rpop,
	.ListLpop = list_lpop,
	.ListRemove = list_remove,
	.ListDestroy = list_destroy,

	.ListIteratorNew = list_iterator_new,
	.ListIteratorNewFromNode = list_iterator_new_from_node,
	.ListIteratorNext = list_iterator_next,
	.ListIteratorDestroy = list_iterator_destroy	
};*/

structList stDelCellIdList = {
	//.CellList = NULL,
	.ListLenLock = &gDelCellIdListLenLock,
	.RemoveLock = &gCellIdRemoveLock,
	.ListNodeNew = list_node_new,
	.ListNew = list_new,
	.ListRpush = list_rpush,
	.ListLpush = list_lpush,	
	.ListFind = list_find,
	.ListAt = list_at,	
	.ListRpop = list_rpop,
	.ListLpop = list_lpop,
	.ListRemove = list_remove,
	.ListDestroy = list_destroy,

	.ListIteratorNew = list_iterator_new,
	.ListIteratorNewFromNode = list_iterator_new_from_node,
	.ListIteratorNext = list_iterator_next,
	.ListIteratorDestroy = list_iterator_destroy	
};

structList stBTCPoolConfList = {
	//.CellList = NULL,
	.ListLenLock = &gBTCPoolConfListLenLock,
	.RemoveLock = &gBTCPoolConfRemoveLock,
	.ListNodeNew = list_node_new,
	.ListNew = list_new,
	.ListRpush = list_rpush,
	.ListLpush = list_lpush,	
	.ListFind = list_find,
	.ListAt = list_at,	
	.ListRpop = list_rpop,
	.ListLpop = list_lpop,
	.ListRemove = list_remove,
	.ListDestroy = list_destroy,

	.ListIteratorNew = list_iterator_new,
	.ListIteratorNewFromNode = list_iterator_new_from_node,
	.ListIteratorNext = list_iterator_next,
	.ListIteratorDestroy = list_iterator_destroy	
};

structList stLTCPoolConfList = {
	//.CellList = NULL,
	.ListLenLock = &gLTCPoolConfListLenLock,
	.RemoveLock = &gLTCPoolConfRemoveLock,
	.ListNodeNew = list_node_new,
	.ListNew = list_new,
	.ListRpush = list_rpush,
	.ListLpush = list_lpush,	
	.ListFind = list_find,
	.ListAt = list_at,	
	.ListRpop = list_rpop,
	.ListLpop = list_lpop,
	.ListRemove = list_remove,
	.ListDestroy = list_destroy,

	.ListIteratorNew = list_iterator_new,
	.ListIteratorNewFromNode = list_iterator_new_from_node,
	.ListIteratorNext = list_iterator_next,
	.ListIteratorDestroy = list_iterator_destroy	
};

/*structList stPoolConfList = {
	//.CellList = NULL,
	.ListLenLock = &gPoolConfListLenLock,
	.RemoveLock = &gPoolConfRemoveLock,
	.ListNodeNew = list_node_new,
	.ListNew = list_new,
	.ListRpush = list_rpush,
	.ListLpush = list_lpush,	
	.ListFind = list_find,
	.ListAt = list_at,	
	.ListRpop = list_rpop,
	.ListLpop = list_lpop,
	.ListRemove = list_remove,
	.ListDestroy = list_destroy,

	.ListIteratorNew = list_iterator_new,
	.ListIteratorNewFromNode = list_iterator_new_from_node,
	.ListIteratorNext = list_iterator_next,
	.ListIteratorDestroy = list_iterator_destroy	
};*/


