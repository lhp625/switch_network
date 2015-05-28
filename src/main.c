/***********************************************************************
* $Id$
* Project:	 switch_network
* File:		 main.c		 
* Description: brief Part of  main process file. 
*
* @version:		V1.0.0
* @date:		5th September 2014
*
* Copyright (C) 2014 SFARDS Co., Ltd. All rights reserved.
*-----------------------------------------------------------------
*
* Revise:  
* 		 date		 version 		author 		description
*  		2014/09/16	 1.0.0		zhengchao	create main process
*
*
*
**********************************************************************/
#include <assert.h>
#include <fcntl.h>
//#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
//#include <arpa/inet.h>
#include "./ap_log.h"
#include "./list/list.h"
#include "./fifo_queue/fifo_queue.h"
#include "./fifo_queue/circular_queue.h"
#include "./app/typedefs.h"
#include "./api_comm/api_login.h"
#include "./download/download.h"
#include "./api_comm/api_command.h"
//#include "./api_comm/api_assign_resource.h"
#include "./api_comm/api_shell_terminal.h"
#include "alarm/alarm_pro.h"
#include "./db_op/db_mysql.h"
//#include "./api_comm/api_divid_xnonce2.h"
//#include "./api_net/api_net.h"
//#include "./api_net/connection_pool/connection_pool.h"
//#include "./api_net/connection_pool/tcp_client.h"
#include "./api_comm/api_monitor.h"

#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <netdb.h>        /* netbd.h is needed for struct hostent =) */
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>


#define SERVER_DEBUG_LEVEL 0

const char *log_file_name = "switch_debug.log";
//const char *localhost_str = "172.16.16.12";

void InitAllInfo()
{
	glStartTime = GetTime();
	//DEBUGMSG(1,("glStartTime:%f\r\n", ((float)glStartTime)/ CLOCKS_PER_SEC));	
}

//char *host_name2 = "172.168.0.75";

/* ******************************************************* */
int main(void)
{
	int rtn = 0, log_file_handle;
	//pthread_t LoginRecvPth, CommandRecvPth;
	pthread_t RecvPth;
	pthread_t LoginParsePth, CommandPackPth, CommandParsePth, ShellTerminalRecvPth;
	pthread_t AlarmPth;
	pthread_t MysqlPth;
	pthread_t MonitorPth;
	int DbOK = 0;





	//printf("host_name2:%s, %ld\r\n", host_name2, (unsigned long)ntohl(inet_addr(host_name2)));


	//return 0;
	
	
    ap_log_debug_to_tty = 0; 						//we like to see immediately if some trouble happens 
	
    log_file_handle = open(log_file_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    if ( log_file_handle <= 0 )
    {
        printf("Failed to open log file: %s %s\r\n", log_file_name, strerror(errno));
        exit(1);
    }

    if ( ! ap_log_add_debug_handle(log_file_handle) )
    {
        printf("Failed to register log file handle for debug output: %s\r\n", ap_error_get_string());
        exit(1);
    }

    ap_log_debug_level = SERVER_DEBUG_LEVEL;

	DEBUGMSG(0,("%s..., %d\r\n", __FUNCTION__, __LINE__));
	ap_log_debug_log("\r\n+---------------------------------------------------------+ \r\n");
	ap_log_debug_log("|    Hello, switch_network @ %s %s         | \r\n", __DATE__, __TIME__);
	ap_log_debug_log("+---------------------------------------------------------+ \r\n");

	InitAllInfo();
	//InitAssign();

	//Main start
	//read configure
	if ((rtn = StrIniRead()) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, rtn:%d!\r\n",  __FILE__, __FUNCTION__, __LINE__, rtn);			
	}
	//AssignResource((void *)&gCfgDataDL);
	//InitDivid((void *)&gCfgDataDL);
	InitLogin();
	InitCommand();
	InitShell();
	DbOK = InitDbAndDev();	
	InitMonitor();

	pthread_create(&RecvPth, NULL, PollRecvProcess, NULL);
	pthread_create(&LoginParsePth, NULL, LoginParseProcess, NULL);
	pthread_create(&CommandPackPth, NULL, CommandPackProcess, NULL);
	pthread_create(&CommandParsePth, NULL, CommandParseProcess, NULL);	
	pthread_create(&ShellTerminalRecvPth, NULL, ShellTerminalProcess, NULL);

	pthread_create(&AlarmPth, NULL, AlarmProcess, NULL);

#if 1
	if (!DbOK)
		pthread_create(&MysqlPth, NULL, MysqlProcess, NULL);
#endif

	pthread_create(&MonitorPth, NULL, MonitorProcess, NULL);



	pthread_join(RecvPth, NULL);
	pthread_join(LoginParsePth, NULL);

	pthread_join(CommandPackPth, NULL);
	pthread_join(CommandParsePth, NULL);
	pthread_join(ShellTerminalRecvPth, NULL);
	pthread_join(AlarmPth, NULL);
#if 1	
	pthread_join(MysqlPth, NULL);
#endif

	pthread_join(MonitorPth, NULL);

	//ÊÍ·Å
	//FreeRes(gLoginResIndex.iMinerNum);	
	ShellTeiminalDestroy();
	TCPClientDestroy();
	CirFifoQueue.QueDestroy(gpCommandQue);
	CirFifoQueue.QueDestroy(gpLoginQue);
	//CirFifoQueue.QueDestroy(gpMinerStatusDbQue);
	//CirFifoQueue.QueDestroy(gpCellStatusDbQue);

    return 0;
}

