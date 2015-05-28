/***********************************************************************
* $Id$
* Project:	 switch_network  
* File:api_net/api_net.c		 
* Description: brief Part of net api. 
*
* @version:		V1.0.0
* @date:		09th September 2014
*
* Copyright (C) 2014 SFARDS Co., Ltd. All rights reserved.
*-----------------------------------------------------------------
*
* Revise:  
* 		 date		 version 		author 		description
*  		2014/09/09	 1.0.0		zhengchao	create Net
*
*
*
**********************************************************************/
#include "api_net.h"

static const char *err_bad_port = "bad port: %d";


stApiNetPoll *api_pollcreate(int max_connections)
{
   	stApiNetPoll *poller;
    int epoll_fd;

    /* maybe we should switch to something like 'ivykis' - library for asynchronous I/O readiness notification */
    ap_error_clear();

    epoll_fd = epoll_create(max_connections);

    if (epoll_fd == -1)
    {
        ap_error_set(__FUNCTION__, AP_ERRNO_SYSTEM);
        return NULL;
    }

    poller = malloc(sizeof(stApiNetPoll));

    if (poller == NULL)
    {
        close(epoll_fd);
        return NULL;
    }

    poller->events = malloc(max_connections * sizeof(struct epoll_event));

    if ( poller->events == NULL )
    {
        close(epoll_fd);
        free(poller);
        return NULL;
    }

    poller->max_events = max_connections;
    poller->epoll_fd = epoll_fd;
    //poller->listen_socket_fd = listen_socket_fd;
    poller->events_count = 0;

	return poller;
}

int api_pollcontrol(int listen_socket_fd, stApiNetPoll *poller)
{
    struct epoll_event ev;

	ev.events = EPOLLIN;
	ev.data.fd = listen_socket_fd;
	
	if (epoll_ctl(poller->epoll_fd, EPOLL_CTL_ADD, listen_socket_fd, &ev) == -1)
	{
		ap_error_set_detailed(__FUNCTION__, AP_ERRNO_SYSTEM, "epoll_ctl() add listen_socket_fd");
		close(poller->epoll_fd);
		free(poller->events);
		free(poller);
	
		return -1;
	}

	return 0;
}

int api_pool_pollwait(stApiNetPoll *poller)
{
	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));

    poller->events_count = epoll_wait(poller->epoll_fd, poller->events, poller->max_events, 0);
	DEBUGMSG(0,("2 data.fd:%d, events:%p, events_count:%d\r\n", 
		poller->events[0].data.fd, poller->events, poller->events_count));
	
    if (poller->events_count == -1)
    {
        ap_error_clear();
        ap_error_set_detailed(__FUNCTION__, AP_ERRNO_SYSTEM, "epoll_wait() -1");
        return -1;
    }

    if (poller->events_count == 0)
    {
        ap_error_clear();
        ap_error_set_detailed(__FUNCTION__, AP_ERRNO_SYSTEM, "epoll_wait() 0");
        return -2;
    }
	
	return 0;
}

//brief Creates poller structure, adding the listener socket if set
/*stApiNetPoll *api_net_poller_create(int listen_socket_fd, int max_connections)
{
   	stApiNetPoll *poller;
    struct epoll_event ev;
    int epoll_fd;


    //maybe we should switch to something like 'ivykis' - library for asynchronous I/O readiness notification 
    ap_error_clear();

    if (listen_socket_fd > 0)
        ++max_connections;

    epoll_fd = epoll_create(max_connections);

    if (epoll_fd == -1)
    {
        ap_error_set(__FUNCTION__, AP_ERRNO_SYSTEM);
        return NULL;
    }

    poller = malloc(sizeof(stApiNetPoll));

    if (poller == NULL)
    {
        close(epoll_fd);
        return NULL;
    }

    poller->events = malloc(max_connections * sizeof(struct epoll_event));

    if ( poller->events == NULL )
    {
        close(epoll_fd);
        free(poller);
        return NULL;
    }

    poller->max_events = max_connections;
    poller->epoll_fd = epoll_fd;
    poller->listen_socket_fd = listen_socket_fd;
    poller->events_count = 0;

    if ( listen_socket_fd >= 0 )
    {
        ev.events = EPOLLIN;
        ev.data.fd = listen_socket_fd;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_socket_fd, &ev) == -1)
        {
            ap_error_set_detailed(__FUNCTION__, AP_ERRNO_SYSTEM, "epoll_ctl() add listen_socket_fd");
            close(epoll_fd);
            free(poller->events);
            free(poller);

            return NULL;
        }
    }

    return poller;
}*/

stApiNetConnPool *api_net_conn_pool_create(int flags, int max_connections)
{
    stApiNetConnPool *pool;

    ap_error_clear();

    pool = malloc(sizeof(stApiNetConnPool));
    if ( pool == NULL )
    {
        ap_error_set(__FUNCTION__, AP_ERRNO_OOM);
        return NULL;
    }

    memset(&pool->listener, 0, sizeof(pool->listener));

    pool->listener.sock = -1;
    pool->flags = flags;
    pool->stat.timedout = 0;
	pool->max_connections = max_connections;

    return pool;
}


//brief Fills struct sockaddr_in with provided data
int api_net_set_ip4_addr(struct sockaddr_in *destination, in_addr_t address4, int port)
{
	int rtn = 0;
	if ( port <= 0 || port > 65535 )
    {
         ap_error_set_custom("ApiNetSetIp4Addr()", (char*)err_bad_port, port);
         return rtn;
    }

	destination->sin_family = AF_INET;
	destination->sin_port = htons(port);
	destination->sin_addr.s_addr = htonl(address4);

	DEBUGMSG(1,("server sin_addr:%s, sin_port:%d\r\n", inet_ntoa(((struct sockaddr_in*)&address4)->sin_addr), port));

    return (rtn = 1);
}

//brief Creates and starts listener socket for incoming connections
int api_net_conn_pool_listener_create(stApiNetConnPool *pool, int max_tries, int retry_sleep)
{
	size_t addr_len;
	struct sockaddr *addr;
	int nSendBuf = 10240 * 1024; 						   //发送缓冲区设置为10M
	int nRecvBuf = 1024 * 1024; 						   //接受缓冲区设置为1M

    ap_error_clear();

    if ( -1 != pool->listener.sock )
    {
        ap_error_set_custom(__FUNCTION__, "listener is already active");
        return -1;
    }

    pool->listener.sock = socket(bit_is_set(pool->flags, API_NET_POOL_FLAGS_IPV6) ? AF_INET6 : AF_INET,
                                  bit_is_set(pool->flags, API_NET_POOL_FLAGS_TCP) ? SOCK_STREAM : SOCK_DGRAM, 0);

    if ( -1 == pool->listener.sock )
    {
        ap_error_set_detailed(__FUNCTION__, AP_ERRNO_SYSTEM, "socket()");
        return -1;
    }

    if (bit_is_set(pool->flags, API_NET_POOL_FLAGS_IPV6))
    {
    	addr = (struct sockaddr *)(&pool->listener.addr6);
    	addr_len = sizeof(struct sockaddr_in6);
    }
    else
    {
    	addr = (struct sockaddr *)(&pool->listener.addr4);
    	addr_len = sizeof(struct sockaddr_in);
    }

	//设置发送和接受缓冲区
	if (setsockopt(pool->listener.sock, SOL_SOCKET, SO_SNDBUF, (const char*)&nSendBuf, sizeof(int)) < 0)
	{
		ap_error_set_detailed(__FUNCTION__, AP_ERRNO_SYSTEM, "nSendBuf setsockopt()");
		close(pool->listener.sock);
		pool->listener.sock = -1;
		return -1;
	}

	if (setsockopt(pool->listener.sock, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int)) < 0)
	{
		ap_error_set_detailed(__FUNCTION__, AP_ERRNO_SYSTEM, "nRecvBuf setsockopt()");
		close(pool->listener.sock);
		pool->listener.sock = -1;
		return -1;
	}

    for(;;)
    {
        if ( -1 != bind( pool->listener.sock, addr, addr_len ) )
            break; /*  bind OK */

        if ( --max_tries == 0 )
        {
            ap_error_set_detailed(__FUNCTION__, AP_ERRNO_SYSTEM, "bind()");
            close(pool->listener.sock);
            pool->listener.sock = -1;
            return -1;
        }

        ap_log_do_syslog(LOG_ERR, "%s: %m: retries left: %d, sleeping for %d sec(s)", __FUNCTION__, max_tries, retry_sleep);

        sleep(retry_sleep);
    }

   	fcntl(pool->listener.sock, F_SETFL, fcntl(pool->listener.sock, F_GETFL) | O_NONBLOCK);


	//pool->poller = api_net_poller_create(pool->listener.sock, pool->max_connections);
	//if (pool->poller == NULL)
	//	return -1;

	/*if ( bit_is_set(pool->flags, API_NET_POOL_FLAGS_TCP)
	 && 0 != listen(pool->listener.sock, pool->max_connections) )
	{
		ap_error_set_detailed(__FUNCTION__, AP_ERRNO_SYSTEM, "listen()");
		close(pool->listener.sock);
		pool->listener.sock = -1;
		return -1;
	}*/

    return pool->listener.sock;
}


//brief receives available data into internal queue
int api_net_conn_pool_recv(stApiNetConnPool *pool)
{
	int n = 0;
	int iLen = 0;
	int rtn = 0;
    socklen_t addr_len;
    struct sockaddr_storage addr;
	char buf_src[BUFFER_LEN_512] = {0};

	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));
    ap_error_clear();

	//UDP pool
	addr_len = sizeof(addr);
	n = recvfrom(pool->listener.sock, buf_src, sizeof(buf_src), MSG_DONTWAIT | MSG_NOSIGNAL, (struct sockaddr *)&addr, &addr_len);
	DEBUGMSG(0,("#### recvfrom n:%d, buf_src:%s***\r\n", n, buf_src));

	if ( n == -1 )
	{
		pool->Recv.Error++;
		ap_error_set(__FUNCTION__, AP_ERRNO_SYSTEM);
		return (rtn = 1);
	}
	else
	{
		pool->Recv.Count++;
	}
	
	DEBUGMSG(0,("1 listener.sock:%d\n", pool->listener.sock));
	iLen = snprintf(pool->recv_buf,sizeof(pool->recv_buf),"%s %s %d %d", 
		buf_src, inet_ntoa(((struct sockaddr_in*)&addr)->sin_addr), 
		ntohs(((struct sockaddr_in*)&addr)->sin_port), pool->listener.sock);
	DEBUGMSG(0,("2 listener.sock:%d\n", pool->listener.sock));
	if (iLen < 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d,	iLen:%d, buf_src to recv_buf error!\r\n", __FILE__, __FUNCTION__, __LINE__, iLen);
	}
	
	DEBUGMSG(0,("pool->recv_buf:\r\n%s\r\n", pool->recv_buf));

	
	//if (n > 42)
	//	printf("LoginCount:%d, LoginError:%d, n:%d\r\n", pool->Recv.Count, pool->Recv.Error, n);
	
    return rtn;
}

/*int api_net_conn_pool_poll(stApiNetConnPool *pool)
{
	int rtn = 0;
	int event_idx;
    stApiNetPoll *poller;

	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));
    ap_error_clear();

    poller = pool->poller;

    poller->events_count = epoll_wait(poller->epoll_fd, poller->events, poller->max_events, 0);
	DEBUGMSG(0,("2 data.fd:%d, events:%p, events_count:%d\r\n", 
		poller->events[0].data.fd, poller->events, poller->events_count));
	
    if (poller->events_count == -1)
    {
        ap_error_set_detailed(__FUNCTION__, AP_ERRNO_SYSTEM, "epoll_wait() -1");
        return (rtn = 1);
    }

    if (poller->events_count == 0)
    {
        ap_error_set_detailed(__FUNCTION__, AP_ERRNO_SYSTEM, "epoll_wait() 0");
        return (rtn = 2);
    }


    if ( pool->listener.sock != -1 )
    {
        for ( event_idx = 0; event_idx < poller->events_count; ++event_idx)
        {

			DEBUGMSG(0,("sock:%d, fd:%d\r\n", pool->listener.sock, pool->poller->events[event_idx].data.fd));
			if (pool->listener.sock == pool->poller->events[event_idx].data.fd)
			{
				api_net_conn_pool_recv(pool);
			}
			else
			{
				pool->Recv.fdError++;
			}
        }

    }

	DEBUGMSG(0,("rtn:%d\r\n", rtn));
	
	return rtn;
}*/

int api_net_set_client_ip4(struct sockaddr_in *addr4,  void *ipString, int port)
{
	addr4->sin_family = AF_INET;
	addr4->sin_port = htonl(port);

	inet_pton(AF_INET, ipString, &addr4->sin_addr);

	return 0;
}

int api_net_send_to_client(int fd, struct sockaddr_in addr4, void *sendBuf, int iSendChunk, stApiNetConnPool *pool)
{
	int n = 0;
	socklen_t slen;
	int time = 0;

	DEBUGMSG(1, ("%s... iSendChunk:%d, fd:%d\r\n", __FUNCTION__, iSendChunk, fd));
	
	slen = sizeof(struct sockaddr_in);

    for(;;)
    {
    	n = sendto(fd, sendBuf, iSendChunk, 0, (struct sockaddr *)&addr4, slen);
		if (n > 0)
		{
			pool->Send.Count++;
			break;
		}
		/*else
		{
			stApiNetWork.LoginSend.Error++;
		}*/

		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			if (time++ >= 10)//if (iSendChunk < 10) /*  well, we tried */
				break;
			usleep(time*5);
			//iSendChunk /= 2;
			pool->Send.Error++;
			continue;
		}
		else if ((n == -1 && errno == EPIPE) || n == 0)
		{
			if (ap_log_debug_level)
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, SendToClient() is error!\r\n", __FILE__, __FUNCTION__, __LINE__);
			}
			pool->Send.Error1++;
			
			break;
		}
    }


	return n;
}

void api_net_poller_destroy(stApiNetPoll *poller)
{
	close(poller->epoll_fd);
	free(poller->events);
	free(poller);
}

void api_net_conn_pool_destroy(stApiNetConnPool *pool, int free_this)
{
    if ( pool->listener.sock != -1 )
    	close(pool->listener.sock);

    //if ( pool->poller != NULL )
    //	api_net_poller_destroy(pool->poller);

    if (free_this)
    	free(pool);
}

int SetClientIp4(struct sockaddr_in *addr4,  void *ipString, int port)
{
	addr4->sin_family = AF_INET;
	addr4->sin_port = htons(port);
	//conn->remote.addr4.sin_addr.s_addr = htonl(address4);

	inet_pton(AF_INET, ipString, &addr4->sin_addr);

	return 0;
}

structApiNet stApiNetWork = 
{
	.ApiNetConnPoolCreate = api_net_conn_pool_create,
	.ApiNetSetIp4Addr = api_net_set_ip4_addr,
	.ApiNetConnPoolListenerCreate = api_net_conn_pool_listener_create,
	.ApiNetConnPoolRecv = api_net_conn_pool_recv,
	//.ApiNetConnPoolPoll = api_net_conn_pool_poll,
	.ApiNetSetClientIp4 = api_net_set_client_ip4,
	.ApiNetSendToClient = api_net_send_to_client,
	.ApiNetDestroy = api_net_conn_pool_destroy,
	
	.ApiNetPollCreate = api_pollcreate,
	.ApiNetPollControl = api_pollcontrol,
	.ApiNetPoolPollWait = api_pool_pollwait
};



