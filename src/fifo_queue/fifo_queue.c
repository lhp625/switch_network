#include "fifo_queue.h"
#include <time.h>
#include "../ap_log.h"
#include "../app/debug.h"

//#define DEBUG
//#undef DEBUG /*  uncomment when you are done! */
  
#ifdef DEBUG
    #define print printf
#else
    #define print(...)
#endif

//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/*****************************************
struct _node {
    void *data;
    struct _node * next;
};
typedef struct _node node;

struct _end_q {
   node * first;
   node * last;
};
typedef struct _end_q queue;

****************************************/

/**
 * create and return a new queue
 **/
queue * queue_create()
{
	DEBUGMSG(0,("%s... 11111\r\n", __FUNCTION__));
	queue * new_queue = malloc(sizeof(queue));
	DEBUGMSG(0,("%s... 22222\r\n", __FUNCTION__));
	if(new_queue == NULL) {
		fprintf(stderr, "Malloc failed creating the que\r\n");
		return NULL;
	}
	new_queue->first = NULL;
	new_queue->last = NULL;

	GetLocalTimeLog();
	ap_log_debug_log("\t\t%s, %s, %d, queue_create Generated the que @ %p!\r\n",  __FILE__, __FUNCTION__, __LINE__, new_queue);

	return new_queue;
}

void queue_destroy(queue * que, pthread_mutex_t *lock)
{
    
    //ap_log_debug_log("Enterd to queue_destroy\r\n");

    if(que == NULL) {
        return;
    }

	GetLocalTimeLog();
	ap_log_debug_log("\t\t%s, %s, %d, que is not null, que = %p!\r\n",  __FILE__, __FUNCTION__, __LINE__, que);

    pthread_mutex_lock(lock);
    if(que->first == NULL) {
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, queue_destroy que->first == NULL!\r\n",  __FILE__, __FUNCTION__, __LINE__);
        free(que);
        pthread_mutex_unlock(lock);
        return;
    }

    //ap_log_debug_log("que is there lets try to free it...\r\n");

    node * _node = que->first;

    while(_node != NULL) {
        // freeing the data coz it's on the heap and no one to free it
        // except for this one
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, freeing : %s!\r\n",  __FILE__, __FUNCTION__, __LINE__, (char *)_node->data);
        free(_node->data);
        node *tmp = _node->next;
        free(_node);
        _node = tmp;
    }

    free(que);

    pthread_mutex_unlock(lock);
}

/**
 * que is a queue pointer
 * data is a heap allocated memory pointer
 */
int queue_push(queue * que, void * data, pthread_mutex_t *lock)
{
    node * new_node = malloc(sizeof(node));
	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));
	
    if(new_node == NULL) {
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Malloc failed creating a node!\r\n",  __FILE__, __FUNCTION__, __LINE__);
        return -1;
    }

    // assumming data is in heap
    new_node->data = data;
    new_node->next = NULL;
	DEBUGMSG(0,("1 ^^^^^\r\n"));
    pthread_mutex_lock(lock);
	DEBUGMSG(0,("2 ^^^^^\r\n"));
    if (que->first == NULL) {
        // new que
        que->first = new_node;
        que->last = new_node;
    } else {
        que->last->next = new_node;
        que->last = new_node;
    }
    pthread_mutex_unlock(lock);
	DEBUGMSG(1,("---last->data:%s, @last->data:%p\r\n", (char *)que->last->data, que->last->data));

    return 0;
}

void *queue_pop(queue * que, pthread_mutex_t *lock) 
{
    //ap_log_debug_log("Entered to deque\r\n");
    if (que == NULL) {
        //ap_log_debug_log("queue is null exiting...\r\n");
        DEBUGMSG(0,("queue is null exiting...\r\n"));
        return NULL;
    }
	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));

    pthread_mutex_lock(lock);
    if (que->first == NULL) {
        pthread_mutex_unlock(lock);
        //ap_log_debug_log("queue_pop que->first is null exiting.\r\n");
		DEBUGMSG(0,("que->first is null exiting.\r\n"));
        return NULL;
    }

    void * data;
    node * _node = que->first;
    if (que->first == que->last) {
        que->first = NULL;
        que->last = NULL;
    } else {
        que->first = _node->next;
    }

    data = _node->data;
	
    //ap_log_debug_log("Freeing _node@ %p", _node);
    free(_node);
    pthread_mutex_unlock(lock);
	DEBUGMSG(0,("3 ++++data:%s, @data:%p\r\n", (char *)data, data));
    //ap_log_debug_log("Exiting deque\r\n");
    
    return data;
}

void queue_malloc(char **message, int BUFFER_LEN)
{
	DEBUGMSG(0,("%s...\r\n",  __FUNCTION__));

	if (*message != NULL)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, queue_malloc is not null!\r\n",  __FILE__, __FUNCTION__, __LINE__);		
		*message = NULL;
	}

	*message = malloc(BUFFER_LEN);

	DEBUGMSG(0,("BUFFER_LEN:%d, %p, %s\r\n",BUFFER_LEN, *message, *message));
}

void queue_free(char *message)
{
	if (message != NULL)
		free(message);
}

/*stFifoQuque CommandFifoQueue = {
	.QueCreate = queue_create,
	.QueDestroy = queue_destroy,
	.QuePush = queue_push,
	.QuePop = queue_pop,
	.QueMalloc = queue_malloc,
	.QueFree = queue_free,
};*/

stFifoQuque LoginFifoQueue = {
	.QueCreate = queue_create,
	.QueDestroy = queue_destroy,
	.QuePush = queue_push,
	.QuePop = queue_pop,
	.QueMalloc = queue_malloc,
	.QueFree = queue_free,
};


