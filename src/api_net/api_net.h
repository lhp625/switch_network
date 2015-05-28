#ifndef __NET__
#define __NET__

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>
#include "../ap_error/ap_error.h"
#include "../ap_log.h"
#include "../app/debug.h"
#include "../app/typedefs.h"
#include "../fifo_queue/fifo_queue.h"
#include "../list/list.h"


#define bit_get(p,m) ((p) & (m))
#define bit_is_set(p,m) (((p) & (m)) != 0)
#define bit_set(p,m) ((p) |= (m))
#define bit_clear(p,m) ((p) &= ~(m))
#define bit_flip(p,m) ((p) ^= (m))
#define bit_write(c,p,m) (c ? bit_set(p,m) : bit_clear(p,m))
#define BIT(x) (1 << (x))

#define MAX_PER_CLIENT	10000

typedef enum
{
	API_NET_POOL_FLAGS_TCP = 1,						// Pool is of TCP type. Absence of this flag means UDP pool
	API_NET_POOL_FLAGS_ASYNC = 2,					//Perform fully asynchronous I/O operations
	API_NET_POOL_FLAGS_IPV6 = 4						//Pool is in IPv6 mode. Absence of this flag means IPv4 mode. Have sense in server mode
}API_NET_POOL_FLAGS;

typedef struct
{
    //unsigned conn_count; 							//Lifetime connections count
    unsigned timedout;   							//How many times connections was expired
    //unsigned queue_full_count;  					//Count of dropped connections because of queue full
    //unsigned active_conn_count; 					//A sum of active pool's connections at the time of newly created. use for average_conn_count = active_conn_count / conn_count
    //struct timespec total_time;  						//Total connected time for all past connections
} stApiNetStat;

//brief Poller data structur. Stores settings, events etc...
typedef struct 
{
    //struct stApiNetConnPool *parent; 				//Parent pool. can be NULL for stand alone 
    struct epoll_event *events; 					//Current events storage. 
    int max_events; 								//Maximum events that can be stored and reported
    int events_count; 								//Used for stand alone poller, current events count
    int epoll_fd; 									//poll descriptor for epoll() calls
    //int listen_socket_fd; 							// Listener socket descriptor. Copy of pool's descriptor if not stand-alone
}stApiNetPoll;

typedef struct
{
	int Count;
	int Error;
	int Error1;                   //recv len <= 0
	int fdError;                   //fd error
}stCount;

//brief Connections pool data structure
typedef struct 
{
	char recv_buf[BUFFER_LEN_1024];
	int max_connections; 							//Connections array size
	unsigned flags; 								//AP_NET_POOL_FLAGS_
	unsigned state; 								//AP_NET_ST_

	//stApiNetPoll *poller;
	struct
	{
		int sock; 									//less than 0 if no bind was made 
		union 										//local address. filled on connect() 
		{
			struct sockaddr_in  addr4; 				//IPv4 
			struct sockaddr_in6 addr6; 				//IPv6 
		};
	} listener; 									//< Listener socket setup for server side pool 

	//ap_net_conn_pool_callback_func callbackFunc; 	//Callback function pointer for ap_net_conn_pool_poll()
	stApiNetStat stat; 								//Statistics

	struct sockaddr_in MulticastAddr4;

	stCount Recv;
	stCount Send;
}stApiNetConnPool;

typedef struct
{
	stApiNetConnPool *(*ApiNetConnPoolCreate)(int, int);
	int (*ApiNetSetIp4Addr)(struct sockaddr_in *, in_addr_t, int);
	int (*ApiNetConnPoolListenerCreate)(stApiNetConnPool *, int, int);
	int (*ApiNetConnPoolRecv)(stApiNetConnPool *);
	//int (*ApiNetConnPoolPoll)(stApiNetConnPool *);
	int (*ApiNetSetClientIp4)(struct sockaddr_in *, void *, int);
	int (*ApiNetSendToClient)(int, struct sockaddr_in, void *, int, stApiNetConnPool *);
	void (*ApiNetDestroy)(stApiNetConnPool *, int);
	stApiNetPoll *(*ApiNetPollCreate)(int);
	int (*ApiNetPollControl)(int, stApiNetPoll *);
	int (*ApiNetPoolPollWait)(stApiNetPoll *);
}structApiNet;

structApiNet stApiNetWork;

int SetClientIp4(struct sockaddr_in *addr4,  void *ipString, int port);

#endif

