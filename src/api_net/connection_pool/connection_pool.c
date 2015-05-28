
#include "connection_pool.h"


structApiNetTCP stApiNetShell =
{
	.ConnectionPoolCreate = ap_net_conn_pool_create,
	.ConnectionPoolSetStrAddr = ap_net_conn_pool_set_str_addr,
	.ConnectionPoolListenerCreate = ap_net_conn_pool_listener_create,
	.ConnectionPoolPoll = ap_net_conn_pool_poll,
	.ConnectionPoolSend = ap_net_conn_pool_send,
	.ConnectionPoolClose = ap_net_conn_pool_close_connection,
	.ConnectionPoolSetAddr = ap_net_set_ip4_addr,
	.ConnectionPoolDestroy = ap_net_conn_pool_destroy,
	.ConnectionPoolConnect = ap_net_conn_pool_connect_straddr,
	.ConnectionPoolpollerCreate = ap_net_conn_pool_poller_create
};

structApiNetTCP stApiNetMonitor =
{
	.ConnectionPoolCreate = ap_net_conn_pool_create,
	.ConnectionPoolSetStrAddr = ap_net_conn_pool_set_str_addr,
	.ConnectionPoolListenerCreate = ap_net_conn_pool_listener_create,
	.ConnectionPoolPoll = ap_net_conn_pool_poll,
	.ConnectionPoolSend = ap_net_conn_pool_send,
	.ConnectionPoolClose = ap_net_conn_pool_close_connection,
	.ConnectionPoolSetAddr = ap_net_set_ip4_addr,
	.ConnectionPoolDestroy = ap_net_conn_pool_destroy,
	.ConnectionPoolConnect = ap_net_conn_pool_connect_straddr,
	.ConnectionPoolpollerCreate = ap_net_conn_pool_poller_create
};


