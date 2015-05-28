/** \file ap_net/conn_pool_recv.c
 * \brief Part of AP's toolkit. Networking module, Connection pool: incoming conection's data storeage procedures
 */
#include "conn_pool_internals.h"

static const char *_func_name = "ap_net_conn_pool_recv()";

/* ********************************************************************** */
/** \brief receives available data into internal buffer
 *
 * \param pool struct ap_net_conn_pool_t*
 * \param conn_idx int
 * \return int count of bytes received, 0 if no space in buffer, -1 on error, -2 on connection shutdown
 *
 * this should be safe to call from outside of library,
 * but it's really internal thing
 */
int ap_net_conn_pool_recv(struct ap_net_conn_pool_t *pool, int conn_idx)
{
    int n;
    socklen_t slen;
    struct ap_net_connection_t *conn;

	DEBUGMSG(1,("%s..., conn_idx:%d\r\n", __FUNCTION__, conn_idx));
    ap_error_clear();

    conn = &pool->conns[conn_idx];
	DEBUGMSG(0,("conn_idx:%d, max_connections:%d, conn->state:%d\r\n", conn_idx, pool->max_connections, conn->state));
    if ( conn_idx < 0 || conn_idx > pool->max_connections || ! bit_is_set(conn->state, AP_NET_ST_CONNECTED) )
    {
        ap_error_set_detailed(_func_name, AP_ERRNO_INVALID_CONN_INDEX, "%d", conn_idx);
        return -1;
    }
	DEBUGMSG(0,("1 ###\r\n"));
    /*if ( conn->bufpos < 0 || conn->buffill < 0 || conn->bufpos >= conn->bufsize || conn->bufpos >= conn->buffill )
    {
        conn->bufpos = 0;
        conn->buffill = 0;
    }
	DEBUGMSG(1,("2 ###\r\n"));
    if ( conn->bufpos > (conn->bufsize - conn->bufsize / 3) ) //take 2/3 fullness as signal to move data to beginning 
    {
       n = conn->buffill - conn->bufpos;
       memmove(conn->buf, conn->buf + conn->bufpos, n);
       conn->buffill = n;
       conn->bufpos = 0;
    }
	
    space_left = conn->bufsize - conn->buffill;
	DEBUGMSG(1,("2 space_left:%d, conn->bufsize:%d, conn->buffill:%d, conn->bufpos:%d\r\n", space_left, conn->bufsize, conn->buffill, conn->bufpos));

    if ( space_left == 0 )
    	return 0;*/

    conn->state |= AP_NET_ST_IN;

    if ( bit_is_set(conn->flags, AP_NET_CONN_FLAGS_UDP_IN) )
    {
        /* this UDP connection is outgoing only and we're doing trick with data moving from listener socket into this conn's buffer */
    	slen = (conn->remote.af == AF_INET6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr_in));

        n = recvfrom(pool->listener.sock, conn->recebuf, sizeof(conn->recebuf), MSG_DONTWAIT | MSG_NOSIGNAL,
        		(struct sockaddr*)&conn->remote, &slen);
		
		//DEBUGMSG(1,("sin_port:%d, sin_addr:%s\r\n", conn->remote.addr4.sin_port, inet_ntoa(conn->remote.addr4.sin_addr)));
    }
    else
    {
       	n = ap_net_recv(conn->fd, conn->recebuf, sizeof(conn->recebuf), 0);
    }
	DEBUGMSG(1,("recebuf:%s\r\n", conn->recebuf));
    bit_clear(conn->state, AP_NET_ST_IN);
	DEBUGMSG(1,("--n--:%d, errno:%d, EAGAIN:%d, EWOULDBLOCK:%d\r\n", n, errno, EAGAIN, EWOULDBLOCK));
    if ( n == 0 && errno != EAGAIN && errno != EWOULDBLOCK )//remote is orderly shut down 
        return -2;

    if ( n == -1 )
    {
        if (ap_log_debug_level)
            ap_log_debug_log("? Connection [%d] is dead prematurely. retcode %d, error %s\r\n", conn_idx, n, EBADF, ap_error_get_string());

        ap_net_conn_pool_close_connection(pool, conn_idx);

        return -1;
    }

    return n;
}

