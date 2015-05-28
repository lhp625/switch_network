
//
// list.h
//
// Copyright (c) 2014

#ifndef LIST_H
#define LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <pthread.h>

// Library version

#define LIST_VERSION "0.0.5"

// Memory management macros

#ifndef LIST_MALLOC
#define LIST_MALLOC malloc
#endif

#ifndef LIST_FREE
#define LIST_FREE free
#endif

/*
 * list_t iterator direction.
 */

typedef enum {
    LIST_HEAD
  , LIST_TAIL
} list_direction_t;

/*
 * list_t node struct.
 */

typedef struct list_node {
  struct list_node *prev;
  struct list_node *next;
  void *val;
} list_node_t;

/*
 * list_t struct.
 */

typedef struct {
  list_node_t *head;
  list_node_t *tail;
  unsigned int len;
  void (*free)(void *val);
  int (*match)(void *a, void *b);
} list_t;

/*
 * list_t iterator struct.
 */

typedef struct {
  list_node_t *next;
  list_direction_t direction;
} list_iterator_t;

list_node_t *list_node_new(void *val);
list_iterator_t *list_iterator_new(list_t *list, list_direction_t direction);
list_iterator_t *list_iterator_new_from_node(list_node_t *node, list_direction_t direction);
list_node_t *list_iterator_next(list_iterator_t *self);
void list_iterator_destroy(list_iterator_t *self);

#ifdef __cplusplus
}
#endif

typedef struct
{
	pthread_mutex_t *ListLenLock;
	pthread_mutex_t *RemoveLock;
	list_t *List;
	list_node_t *(*ListNodeNew)(void *);			// Node prototypes.
	list_t *(*ListNew)();
	list_node_t *(*ListRpush)(list_t *, list_node_t *, pthread_mutex_t *);
	list_node_t *(*ListLpush)(list_t *, list_node_t *);	
	list_node_t *(*ListFind)(list_t *, void *);
	list_node_t *(*ListAt)(list_t *, int);	
	list_node_t *(*ListRpop)(list_t *);
	list_node_t *(*ListLpop)(list_t *);
	void (*ListRemove)(list_t *, list_node_t *);
	void (*ListDestroy)(list_t *);
	
	// list_t iterator prototypes.
	list_iterator_t *ListIterator;
	list_iterator_t *(*ListIteratorNew)(list_t *, list_direction_t );	
	list_iterator_t *(*ListIteratorNewFromNode)(list_node_t *, list_direction_t);	
	list_node_t *(*ListIteratorNext)(list_iterator_t *);	
	void (*ListIteratorDestroy)(list_iterator_t *);
}structList;


//structList stMinerList;
//structList stCellList;
structList stDelCellIdList;
structList stBTCPoolConfList;
structList stLTCPoolConfList;

#endif /* LIST_H */
