/***********************************************************************
* $Id$
* Project:	 switch_network
* File:		 api_comm/api_monitor.c		 
* Description: brief Part of Monitor api. 
*
* @version:		V1.0.0
* @date:		23th March 2015
*
* Copyright (C) 2014 SFARDS Co., Ltd. All rights reserved.
*-----------------------------------------------------------------
*
* Revise:  
* 		 date		 version 		author 		description
*  		2015/03/22	 1.0.0		zhengchao	create Monitor communication
*
*
*
**********************************************************************/

#include <stdio.h>     
#include <string.h>
#include <assert.h>
#include "api_monitor.h"
#include "../app/debug.h"
#include "../cJSON/cJSON.h"
#include "./api_command.h"
#include "../md5/md5.h"
#include "../db_op/db_mysql.h"
#include "api_shell_terminal.h"

#define RECONN_INTERVAL_TIME                          8
#define SEND_INIT_INTERVAL_TIME                       2
#define SEND_CELLINFO_INTERVAL_TIME                   60
#define SEND_SWITCHDATA_INTERVAL_TIME                 30
#define SEND_NEWCELLINFO_INTERVAL_TIME                3



//const int client_port = 8989;
//const char *remotehost_str = "192.168.1.100";//"192.168.1.102";//
//const char *remotehost_str = "172.16.16.206";//"192.168.1.102";//
//char token[] = "NLi4oMOt3W2MyR3MHKmsoIC3g86iLlir";
//const int localCliPort = 33004;

pthread_mutex_t gCommMonitorFlagLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCommMonitorUserSeqLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCommMonitorOnineLock = PTHREAD_MUTEX_INITIALIZER;

// 测试用
//int gCliSendMessage_len = 0;
//char gCliSendMessage[BUFFER_LEN_4096] = {0};


UINT gMonitorIdSeq = 1;                                 //全局变量，每发送一包序列号加1
CHAR MonitorBuf[BUFFER_LEN_512];
UINT gCliSendInterval = 0;                              //用户主动发送数据时间间隔
BOOL bConnOK = FALSE;                                   //连接后接受{"conn":1}用

stMonitor MonitorComm[] = 
{
	{"switch.validator", ParseReplay, NULL},                   //switch.validator
	{"cell.info", ParseReplay, NULL},
	{"cell.reboot", ParseRebootCommand, PackCellRebootMonitorToComm},
	{"cell.setled", ParseSetLedCommand, PackCellSetLedMonitorToComm},
	//{"cell.addpool", ParseAddPoolCommand, PackCellAddPoolMonitorToComm},
	//{"cell.delpool", ParseDelPoolCommand, PackCellDelPoolMonitorToComm},
	//{"cell.setdefaultpool", ParseSetdefaultPoolCommand, PackSetdefaultPoolMonitorToComm},
	{"cell.setpool", ParseSetPoolCommand, PackSetPoolMonitorToComm},
	{"cell.setdefaultpool", ParseSetdefaultPoolCommand, NoPackMonitor},
	{"cell.search", ParseCellSearch, NoPackMonitor},
	{"refused", ParseRefuse, NoPackMonitor}

};

stParam ParamMoreTypes[] = 
{
	{"pool_id", SetSpecifyPoolMonitor},
	{"cell_id", SetSpecifyPoolFindCellId}
};

char szPoolParam[][BUFFER_LEN_8] = 
{
	{"url"},
	{"worker"},
	{"pass"},
	{"algo"}	
};

/*stCommandErr MonitorErr[] = 
{
	{"celid does not exist!"},

};*/

stMonitorSrc MoniSrc = {0}; 

void InitMonitor()
{
	MoniSrc.pIdSeq = &gMonitorIdSeq;
	MoniSrc.pBuf = MonitorBuf;
	MoniSrc.pList = &stCellUtlist;
	MoniSrc.pMonitorComm = MonitorComm;
	MoniSrc.pSwiDbInfo = gpSwitchDbInfo;
	MoniSrc.pSendInterval = &gCliSendInterval;
	MoniSrc.iCommLen = dim(MonitorComm);

	memset(MoniSrc.szParseDest, 0, sizeof(MoniSrc.szParseDest));

	CommMonitorFlag.CommMonitorFlagLock = &gCommMonitorFlagLock;
	CommMonitorFlag.UserSeqLock = &gCommMonitorUserSeqLock;
	CommMonitorFlag.OnlineCountLock = &gCommMonitorOnineLock;

}

void CreateTCPClient()
{
	*MoniSrc.pSendInterval = SEND_INIT_INTERVAL_TIME;

	tcp_client = stApiNetMonitor.ConnectionPoolCreate(AP_NET_POOL_FLAGS_TCP, 
											CLIENT_SIZE,
											TCP_CLIENT_TIMEOUT,
											BUFFER_LEN_256,
											client_callback);

	DEBUGMSG(1,("1 tcp_client->conns->fd:%d, state:%d, conns->state:%d\n", tcp_client->conns->fd, tcp_client->state, tcp_client->conns->state));

	//MSleep(2000);
	if (NULL == tcp_client )
		//|| ! stApiNetMonitor.ConnectionPoolSetAddr(&tcp_client->local.addr4, INADDR_ANY, localCliPort))
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, ERROR: Client create set addr: %s\r\n", 
			__FILE__, __FUNCTION__, __LINE__, ap_error_get_string());		
		exit(1);
	}
	DEBUGMSG(1,("2 tcp_client->conns->fd:%d, state:%d, conns->state:%d\n", tcp_client->conns->fd, tcp_client->state, tcp_client->conns->state));
	//创建poller
	if (! stApiNetMonitor.ConnectionPoolpollerCreate(tcp_client))
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, ERROR: Client poller create: %s\r\n", 
			__FILE__, __FUNCTION__, __LINE__, ap_error_get_string());		
		exit(1);
	}

	//第一次连接dataserver
	if (NULL == stApiNetMonitor.ConnectionPoolConnect(tcp_client, 0, (char *)gCfgDataDL.MonitorInfo.szMoniIp, AF_INET, gCfgDataDL.MonitorInfo.iMoniPort, 0))
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, ERROR: Client debug pool connect.: %s\r\n",	
			__FILE__, __FUNCTION__, __LINE__, ap_error_get_string());
		//exit(1);
	}
}

int SetdefaultPoolMonitorToShell
(int argc, char **argv, void *destPackShell, int destLenPackShell, void *destErr, int destLenErr)
{
	int rtn = 0, i = 0;//index = 0, , j = 0;
	//E_ALGO eAlgo;
	int iPoolIdTemp = 0;
	structList *pList = NULL;
	list_node_t *NodeTemp = NULL;
	int iPosPack = 0, iPosErr = 0, iPosBTC = 0, iPosLTC = 0;
	BOOL bErrFlag = FALSE;//bPackBTC = FALSE, bPackLTC = FALSE;
	int iErrIndex = 0;
	char bufBTC[BUFFER_LEN_16] = {0}, bufLTC[BUFFER_LEN_16] = {0};
	int iBTCCount = 0, iLTCCount = 0;

	//iPos = *pPos;
	//btc 或者ltc后面的pool_id 最多3个，最少1个

	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	for (i = 0; i < argc; i++)
	{
		pList = &stBTCPoolConfList;
		//查找pool_id	
		iPoolIdTemp = strtoul(argv[i], 0, 0);
		NodeTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
		if (NodeTemp != NULL)
		{
			//btc 或者ltc后面的pool_id 最多3个，最少1个
			if (!iBTCCount)
			{
				snprintf(bufBTC + iPosBTC, sizeof(bufBTC), "%s,%d", (char *)Algo[0].algo, iPoolIdTemp);
				iPosBTC += strlen(bufBTC + iPosBTC);
				//bPackBTC = TRUE;
				iBTCCount++;
			}
			else
			{	
				if (++iBTCCount > 3)
				{
					iErrIndex = E_POOLCNF_ERROR_TOO_MANYPOOLID;
					break;
				}
				snprintf(bufBTC + iPosBTC, sizeof(bufBTC), ",%d", iPoolIdTemp);
				iPosBTC += strlen(bufBTC + iPosBTC);	
			}		
			continue;
		}
		//else
		pList = &stLTCPoolConfList;
		//查找pool_id	
		iPoolIdTemp = strtoul(argv[i], 0, 0);
		NodeTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
		if (NodeTemp != NULL)
		{
			//btc 或者ltc后面的pool_id 最多3个，最少1个
			if (!iLTCCount)
			{
				snprintf(bufLTC + iPosLTC, sizeof(bufLTC), "%s,%d", (char *)Algo[1].algo, iPoolIdTemp);
				iPosLTC += strlen(bufLTC + iPosLTC);
				//bPackLTC = TRUE;
				iLTCCount++;
			}
			else
			{		
				if (++iLTCCount > 3)
				{
					iErrIndex = E_POOLCNF_ERROR_TOO_MANYPOOLID;
					break;
				}
				snprintf(bufLTC + iPosLTC, sizeof(bufLTC), ",%d", iPoolIdTemp);
				iPosLTC += strlen(bufLTC + iPosLTC);	
			}		
			continue;
		}

		//把错误的pool_id返回
		if (!bErrFlag)
		{
			snprintf((char *)destErr + iPosErr, destLenErr, "%s %s", szRetBuf[3], argv[i]);
			iPosErr += strlen((char *)destErr + iPosErr);
			bErrFlag = TRUE;
			rtn = -1;
		}
		else
		{
			snprintf((char *)destErr + iPosErr, destLenErr, " %s", argv[i]);
			iPosErr += strlen((char *)destErr + iPosErr);
			rtn = -1;
		}

		//if (!bErrFlag)
		//{
		//	bErrFlag = TRUE;
		//iErrIndex =  E_POOLCNF_ERROR_POOLID_EXIST;
		//}
	}

	//btc 或者ltc后面的pool_id 最多3个，最少1个
	DEBUGMSG(1,("iBTCCount:%d, iLTCCount:%d\n", iBTCCount, iLTCCount));
	if ((iBTCCount <= 3 && iLTCCount == 0) || (iBTCCount == 0 && iLTCCount <= 3))
	{
		iErrIndex = E_POOLCNF_ERROR_TOO_FEWPOOLID;
		iPosErr = 0;
		memset((char *)destErr + iPosErr, 0, destLenErr);
	}
	else
	{
		snprintf((char *)destPackShell + iPosPack, destLenPackShell, "%s %s", bufBTC, bufLTC);
		iPosPack += strlen((char *)destPackShell + iPosPack);
	}
		
	if (iErrIndex > 0)
	{
		DEBUGMSG(1,("1 iErrIndex:%d, E_SHELL_ERROR_ADDPOOL:%d\r\n", iErrIndex, E_SHELL_ERROR_SETPOOL));
		if (gUserComdErr[E_SHELL_ERROR_SETPOOL].UserCmdFunc(gPoolConfErr[iErrIndex - 1].cmd, (char *)destErr + iPosErr, destLenErr) < 0)
			return 2;
		else
			return (rtn = E_COMMAND_ERROR_POOL);			
	}
		
	return rtn;
}

int SetSpecifyPoolMonitor
(int argc, char **argv, void *destPackShell, int destLenPackShell, void *destErr, int destLenErr)
{
	int rtn = 0, i = 0;//index = 0, , j = 0;
	//E_ALGO eAlgo;
	int iPoolIdTemp = 0;
	structList *pList = NULL;
	list_node_t *NodeTemp = NULL;
	int iPosErr = 0, iPosBTC = 0, iPosLTC = 0;//iPosPack = 0, 
	BOOL bErrFlag = FALSE;//bPackBTC = FALSE, bPackLTC = FALSE;
	int iErrIndex = 0;
	char bufBTC[BUFFER_LEN_16] = {0}, bufLTC[BUFFER_LEN_16] = {0};
	int iBTCCount = 0, iLTCCount = 0;

	//iPos = *pPos;
	//btc 或者ltc后面的pool_id 最多3个，最少1个

	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	//清空指定池
	memset(&gPoolSpecifyConf, 0, sizeof(stPoolFixedConf)); 

	//pool_id,1,2... 从第二个开始。
	for (i = 1; i < argc; i++)
	{
		//memset(&gPoolSpecifyConf, 0, sizeof(stPoolFixedConf));

		
		pList = &stBTCPoolConfList;
		//查找pool_id	
		iPoolIdTemp = strtoul(argv[i], 0, 0);
		DEBUGMSG(1,("iPoolIdTemp:%d\n", iPoolIdTemp));
		NodeTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
		if (NodeTemp != NULL)
		{
			//btc 或者ltc后面的pool_id 最多3个，最少1个
			if (iBTCCount == 0)
			{
		
				//snprintf(bufBTC + iPosBTC, sizeof(bufBTC), "%s,%d", (char *)Algo[0].algo, iPoolIdTemp);
				//iPosBTC += strlen(bufBTC + iPosBTC);
				//bPackBTC = TRUE;
				iBTCCount++;
			}
			else
			{	
				if (++iBTCCount > 3)
				{
					iErrIndex = E_POOLCNF_ERROR_TOO_MANYPOOLID;
					break;
				}
				snprintf(bufBTC + iPosBTC, sizeof(bufBTC), ",%d", iPoolIdTemp);
				iPosBTC += strlen(bufBTC + iPosBTC);	
			}		
			gPoolSpecifyConf.stBTCPoolConf[iBTCCount-1].iPoolId = iPoolIdTemp;
			DEBUGMSG(0,("iPoolId:%d, iPoolIdTemp:%d\n", gPoolSpecifyConf.stBTCPoolConf[iBTCCount-1].iPoolId, iPoolIdTemp));
			strcpy((char *)gPoolSpecifyConf.stBTCPoolConf[iBTCCount-1].szUrl, (char *)((stPoolConfInfo *)NodeTemp->val)->szUrl);
			DEBUGMSG(0,("Spec szUrl:%s, szUrl:%s\n", (char *)gPoolSpecifyConf.stBTCPoolConf[iBTCCount-1].szUrl, (char *)((stPoolConfInfo *)NodeTemp->val)->szUrl));
			strcpy((char *)gPoolSpecifyConf.stBTCPoolConf[iBTCCount-1].szUser, (char *)((stPoolConfInfo *)NodeTemp->val)->szUser);
			strcpy((char *)gPoolSpecifyConf.stBTCPoolConf[iBTCCount-1].szPasswd, (char *)((stPoolConfInfo *)NodeTemp->val)->szPasswd);
			gPoolSpecifyConf.stBTCPoolConf[iBTCCount-1].eAlgo = ((stPoolConfInfo *)NodeTemp->val)->eAlgo;
			DEBUGMSG(0,("eAlgo:%d\n", gPoolSpecifyConf.stBTCPoolConf[iBTCCount-1].eAlgo));

			gPoolSpecifyConf.iBTCPoolCount = iBTCCount;
			continue;
		}
		//else
		pList = &stLTCPoolConfList;
		//查找pool_id	
		//iPoolIdTemp = strtoul(argv[i], 0, 0);
		NodeTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
		if (NodeTemp != NULL)
		{
			//btc 或者ltc后面的pool_id 最多3个，最少1个
			if (iLTCCount == 0)
			{			
				//snprintf(bufLTC + iPosLTC, sizeof(bufLTC), "%s,%d", (char *)Algo[1].algo, iPoolIdTemp);
				//iPosLTC += strlen(bufLTC + iPosLTC);
				//bPackLTC = TRUE;
				iLTCCount++;
			}
			else
			{		
				if (++iLTCCount > 3)
				{
					iErrIndex = E_POOLCNF_ERROR_TOO_MANYPOOLID;
					break;
				}
				snprintf(bufLTC + iPosLTC, sizeof(bufLTC), ",%d", iPoolIdTemp);
				iPosLTC += strlen(bufLTC + iPosLTC);	
			}		
			gPoolSpecifyConf.stLTCPoolConf[iLTCCount-1].iPoolId = iPoolIdTemp;
			strcpy((char *)gPoolSpecifyConf.stLTCPoolConf[iLTCCount-1].szUrl, (char *)((stPoolConfInfo *)NodeTemp->val)->szUrl);
			strcpy((char *)gPoolSpecifyConf.stLTCPoolConf[iLTCCount-1].szUser, (char *)((stPoolConfInfo *)NodeTemp->val)->szUser);
			strcpy((char *)gPoolSpecifyConf.stLTCPoolConf[iLTCCount-1].szPasswd, (char *)((stPoolConfInfo *)NodeTemp->val)->szPasswd);
			gPoolSpecifyConf.stLTCPoolConf[iLTCCount-1].eAlgo = ((stPoolConfInfo *)NodeTemp->val)->eAlgo;
			DEBUGMSG(0,("LTC eAlgo:%d\n", gPoolSpecifyConf.stLTCPoolConf[iLTCCount-1].eAlgo));

			gPoolSpecifyConf.iLTCPoolCount = iLTCCount;			
			continue;
		}


		//把错误的pool_id返回
		if (!bErrFlag)
		{
			snprintf((char *)destErr + iPosErr, destLenErr, "%s %d", szRetBuf[3], iPoolIdTemp);
			iPosErr += strlen((char *)destErr + iPosErr);
			bErrFlag = TRUE;
			rtn = -1;
		}
		else
		{
			snprintf((char *)destErr + iPosErr, destLenErr, " %d", iPoolIdTemp);
			iPosErr += strlen((char *)destErr + iPosErr);
			rtn = -1;
		}

		//if (!bErrFlag)
		//{
		//	bErrFlag = TRUE;
		//iErrIndex =  E_POOLCNF_ERROR_POOLID_EXIST;
		//}
	}

	//dbgdumpGetPooSpecifyConf();

	//btc 或者ltc后面的pool_id 最多3个，最少1个
	DEBUGMSG(0,("iBTCCount:%d, iLTCCount:%d\n", iBTCCount, iLTCCount));
/*if ((iBTCCount <= 3 && iLTCCount == 0) || (iBTCCount == 0 && iLTCCount <= 3))
{
	iErrIndex = E_POOLCNF_ERROR_TOO_FEWPOOLID;
	iPosErr = 0;
	memset((char *)destErr + iPosErr, 0, destLenErr);
}*/
	/*else
	{
		snprintf((char *)destPackShell + iPosPack, destLenPackShell, "%s %s", bufBTC, bufLTC);
		iPosPack += strlen((char *)destPackShell + iPosPack);
	}*/
	DEBUGMSG(0,("iErrIndex:%d\n", iErrIndex));
	if (iErrIndex > 0)
	{
		//错误后清空gPoolSpecifyConf
		memset(&gPoolSpecifyConf, 0, sizeof(stPoolFixedConf));
		DEBUGMSG(0,("1 iErrIndex:%d, E_SHELL_ERROR_ADDPOOL:%d\r\n", iErrIndex, E_SHELL_ERROR_SETPOOL));
		if (gUserComdErr[E_SHELL_ERROR_SETPOOL].UserCmdFunc(gPoolConfErr[iErrIndex - 1].cmd, (char *)destErr + iPosErr, destLenErr) < 0)
			return 2;
		else
			return (rtn = E_COMMAND_ERROR_POOL);			
	}
		
	return rtn;
}

int SetSpecifyPoolToCell(devlist_st *pNode)
{
	int j = 0;
	char bufDest[BUFFER_LEN_576] = {0};
	
	//pthread_mutex_lock(((structList *)pList)->RemoveLock);
	//for (i = 0; i < iListLen; i++)
//{
	//DEBUGMSG(1,("-------iCellId:%d, szDevId:%s\r\n", 
	//	((stCellConn *)pNode->val)->CellData.iCellId, ((stCellConn *)pNode->val)->CellData.szDevId));
	//结点中的pool信息修改完成后，在进行打包发送结点信息
	pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.PoolConfLock);

	//清空结点池信息
	memset(&((stCellConn *)pNode->val)->CellPoolConf, 0, sizeof(stPoolFixedConf)); 
	
	((stCellConn *)pNode->val)->CellPoolConf.iBTCPoolCount = 0;
	for(j = 0;j < gPoolSpecifyConf.iBTCPoolCount && gPoolSpecifyConf.iBTCPoolCount <= MAX_POOL_COUNT;j++)
	{	
		((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].iPoolId = gPoolSpecifyConf.stBTCPoolConf[j].iPoolId;
		DEBUGMSG(0,("BTC iPoolId[%d]:%d\r\n", j, ((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].iPoolId));
		strcpy((char *)((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].szUrl, (char *)gPoolSpecifyConf.stBTCPoolConf[j].szUrl);
		strcpy((char *)((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].szUser, (char *)gPoolSpecifyConf.stBTCPoolConf[j].szUser);
		strcpy((char *)((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].szPasswd, (char *)gPoolSpecifyConf.stBTCPoolConf[j].szPasswd);
		((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].eAlgo = gPoolSpecifyConf.stBTCPoolConf[j].eAlgo;
		((stCellConn *)pNode->val)->CellPoolConf.iBTCPoolCount++;
	}
	
	((stCellConn *)pNode->val)->CellPoolConf.iLTCPoolCount = 0; 		
	for(j = 0;j < gPoolSpecifyConf.iLTCPoolCount && gPoolSpecifyConf.iLTCPoolCount <= MAX_POOL_COUNT;j++)
	{	
		((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].iPoolId = gPoolSpecifyConf.stLTCPoolConf[j].iPoolId;
		DEBUGMSG(0,("LTC iPoolId[%d]:%d\r\n", j, ((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].iPoolId));
		strcpy((char *)((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].szUrl, (char *)gPoolSpecifyConf.stLTCPoolConf[j].szUrl);
		strcpy((char *)((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].szUser, (char *)gPoolSpecifyConf.stLTCPoolConf[j].szUser);
		strcpy((char *)((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].szPasswd, (char *)gPoolSpecifyConf.stLTCPoolConf[j].szPasswd);
		((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].eAlgo = gPoolSpecifyConf.stLTCPoolConf[j].eAlgo;
		((stCellConn *)pNode->val)->CellPoolConf.iLTCPoolCount++;
	}
	pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.PoolConfLock);
	
	//有数据已经插入数据库后，才能进行更新。
	if (((stCellConn *)pNode->val)->eCellMySql != E_QUERY_TYPE_INSERT)					
		((stCellConn *)pNode->val)->eCellMySql = E_QUERY_TYPE_UPDATE;		
	
	DEBUGMSG(1,("bIniCellStatus:%d\r\n", bIniCellBase));
	if (bIniCellBase)
	{
		CellSqlQuery[E_QUERY_TYPE_UPDATE - 1].QueryPackFunc2(
														pNode->val, 
														&gCfgDataDL.CellBaseInfo, 
														&gCfgDataDL.CellColName, 
														bufDest, sizeof(bufDest));
		DEBUGMSG(1,("bufDest:%s\r\n", bufDest));
		//printf("bufDest:%s\r\n", bufDest);
		pthread_mutex_lock(&gCellBaseLock);
		MysqlFunc.dbDoQuery(&myCellBase, bufDest, E_QUERY_TYPE_UPDATE, NULL);
		pthread_mutex_unlock(&gCellBaseLock);
		//memset(bufDest, 0, sizeof(bufDest));	
	}	
	
	return 0;
}

int SetSpecifyPoolFindCellId
(int argc, char **argv, void *destPackShell, int destLenPackShell, void *destErr, int destLenErr)
{
	int rtn = 0, i = 0;//, j = 0;//, iErrIndex = 0;
	//int iId = 0;
	structUthash *pList = NULL;
	int iListLen = 0;//iId = -1;
	devlist_st *pNode = NULL, *tmp = NULL;//, *SelNode = NULL;
	//BYTE timeBuf[BUFFER_LEN_256] = {0};
	//char bufDest[BUFFER_LEN_576] = {0};
	int iPosPack = 0;//iPos = 0, 
	char szCellIdBuf[BUFFER_LEN_32] = {0};

	DEBUGMSG(1,("%s..., argc:%d\n", __FUNCTION__, argc));
	printf("%s..., argc:%d\n", __FUNCTION__, argc);
	pList = &stCellUtlist;

	pthread_mutex_lock(pList->ListLenLock);
	iListLen = pList->CountUsersStr(*pList->StrHUsers);
	pthread_mutex_unlock(pList->ListLenLock);
	DEBUGMSG(1,("iListLen:%d\r\n", iListLen));
	if (iListLen == 0)
		return E_COMMAND_ERROR_EMPTY;
	
	pthread_mutex_lock(pList->MonitorRemLock);
	//cell_id,1,2... 从第二个开始。
	for (i = 1; i < argc; i++)
	{
		DEBUGMSG(1,("argv[%d]:%s\n", i, argv[i]));
		if (strncmp(argv[i], "all", 3) == 0)
		{
			snprintf((char *)destPackShell + iPosPack, destLenPackShell, "%s", argv[i]);
			HASH_ITER(hh, *pList->StrHUsers, pNode, tmp)
			{	
				SetSpecifyPoolToCell(pNode);
			}
			
			pthread_mutex_unlock(pList->MonitorRemLock);
			return rtn;
		}
	
		//iId = strtoul(argv[i], 0, 0);
		strncpy(szCellIdBuf, argv[i], sizeof(szCellIdBuf));
		//查找CellId	
		//pNode = pList->FindUserInt(pList->HashUsers, iId);
		pNode = pList->FindUserStr(pList->StrHUsers, szCellIdBuf);
		if (pNode != NULL)
		{
			SetSpecifyPoolToCell(pNode);
			//保存cell_id，relogin时用
			if (iPosPack == 0)
			{
				snprintf((char *)destPackShell + iPosPack, destLenPackShell, "%s", szCellIdBuf);
			}
			else
			{
				snprintf((char *)destPackShell + iPosPack, destLenPackShell, ",%s", szCellIdBuf);
			}
			iPosPack += strlen((char *)destPackShell + iPosPack);					
		}
		else
		{
			//不存在的cell_id保存，发送给dataserver时用。
			if (CommMonitorFlag.iBufPos == 0)
			{
				snprintf((char *)CommMonitorFlag.szIdbufRtn+CommMonitorFlag.iBufPos, sizeof(CommMonitorFlag.szIdbufRtn), "%s %s", szRetBuf[4], szCellIdBuf);
			}
			else
			{
				snprintf((char *)CommMonitorFlag.szIdbufRtn+CommMonitorFlag.iBufPos, sizeof(CommMonitorFlag.szIdbufRtn), " %s", szCellIdBuf);
			}
			CommMonitorFlag.iBufPos += strlen((char *)CommMonitorFlag.szIdbufRtn+CommMonitorFlag.iBufPos);
			//rtn = E_COMMAND_ERROR_ID;		
			rtn = -1;
		}
	}
	pthread_mutex_unlock(stCellUtlist.MonitorRemLock);

	return rtn;
}
/*int SetAddPoolList(cJSON *pSubArray, stPoolConfInfo *val, int iCnt, int iIndex)
{
	cJSON *pSubArr = NULL;
	cJSON *pSubArrToJson = NULL;

	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	pSubArr = cJSON_GetArrayItem(pSubArray, iCnt);
	if(NULL == pSubArr)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArr!\r\n",  __FILE__, __FUNCTION__, __LINE__);		 
		return E_COMMAND_ERROR_JSON;
	}
	
	pSubArrToJson = cJSON_GetObjectItem(pSubArr, szPoolParam[0]);
	if(NULL == pSubArrToJson)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArrToJson, szPoolParam :%s!\r\n",  
			__FILE__, __FUNCTION__, __LINE__, szPoolParam[0]);
		return E_COMMAND_ERROR_JSON;		
	}
	else
	{
		DEBUGMSG(1,("url:%s\n", pSubArrToJson->valuestring));
		strcpy((char *)val->szUrl, pSubArrToJson->valuestring); 
	}
	
	pSubArrToJson = cJSON_GetObjectItem(pSubArr, szPoolParam[1]);
	if(NULL == pSubArrToJson)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArrToJson, szPoolParam :%s!\r\n",  
			__FILE__, __FUNCTION__, __LINE__, szPoolParam[1]);
		return E_COMMAND_ERROR_JSON;	
	}
	else
	{
		DEBUGMSG(1,("szUser:%s\n", pSubArrToJson->valuestring));
		strcpy((char *)val->szUser, pSubArrToJson->valuestring);
	}
	
	pSubArrToJson = cJSON_GetObjectItem(pSubArr, szPoolParam[2]);
	if(NULL == pSubArrToJson)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArrToJson, szPoolParam :%s!\r\n",  
			__FILE__, __FUNCTION__, __LINE__, szPoolParam[2]);
		return E_COMMAND_ERROR_JSON;		
	}
	else
	{
		DEBUGMSG(1,("szPasswd:%s\n", pSubArrToJson->valuestring));
		strcpy((char *)val->szPasswd, pSubArrToJson->valuestring);
	}

	//插入链表
	node = pList->ListNodeNew(val);
	assert(node->val == val);
	pList->ListRpush(pList->List, node, pList->ListLenLock);


	return 0;
}*/

int SetDelAllPoolList()
{
	int rtn = 0;//, iQueryRtn = 0, iQuerySwitchRtn = 0;
	structList *pList = NULL;
	list_node_t *pNode = NULL;
	int iCount = 0;
	//char bufDest[BUFFER_LEN_512] = {0};
	int  i = 0;//, iPos = 0;
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	pList = &stBTCPoolConfList;
	pthread_mutex_lock(pList->ListLenLock);
	iCount = pList->List->len;
	pthread_mutex_unlock(pList->ListLenLock);

	DEBUGMSG(1,("BTC iCount:%d\n", iCount));
	pthread_mutex_lock(pList->RemoveLock);
	for(i = 0;i < iCount;i++)
	{
		//pList->ListRemove(pList->List, pNode);
		//DEBUGMSG(1,("pList->List->len:%d\n", pList->List->len));
		pNode = pList->ListAt(pList->List, 0);
		if (pNode != 0)
		{
			DEBUGMSG(0,("\tid:%d\r\n\t\turl:'%s'\r\n\t\tuser:'%s'\r\n\t\tpwd:'%s'\r\n", 
				((stPoolConfInfo *)pNode->val)->iPoolId,
				((stPoolConfInfo *)pNode->val)->szUrl, 
				((stPoolConfInfo *)pNode->val)->szUser,
				((stPoolConfInfo *)pNode->val)->szPasswd
				));
			pList->ListRemove(pList->List, pNode);
		}
	}
	//if (iCount != 0)
	//	pList->ListDestroy(pList->List);
	pthread_mutex_unlock(pList->RemoveLock);

	pList = &stLTCPoolConfList;
	iCount = 0;
	pthread_mutex_lock(pList->ListLenLock);
	iCount = pList->List->len;
	pthread_mutex_unlock(pList->ListLenLock);

	DEBUGMSG(1,("LTC iCount:%d\n", iCount));
	pthread_mutex_lock(pList->RemoveLock);
	for(i = 0;i < iCount;i++)
	{
		//pList->ListRemove(pList->List, pNode);
		//DEBUGMSG(1,("pList->List->len:%d\n", pList->List->len));	
		pNode = pList->ListAt(pList->List, 0);
		if (pNode != 0)
		{
			DEBUGMSG(0,("\tid:%d\r\n\t\turl:'%s'\r\n\t\tuser:'%s'\r\n\t\tpwd:'%s'\r\n", 
				((stPoolConfInfo *)pNode->val)->iPoolId,
				((stPoolConfInfo *)pNode->val)->szUrl, 
				((stPoolConfInfo *)pNode->val)->szUser,
				((stPoolConfInfo *)pNode->val)->szPasswd
				)); 	
			pList->ListRemove(pList->List, pNode);
		}
	}
	//if (iCount != 0)
	//	pList->ListDestroy(pList->List);
	pthread_mutex_unlock(pList->RemoveLock);

	return rtn;
}

int SetParseAddPoolToString(void *pJson, void *dest, int destLen, int *pSize, int iCnt)
{
	int rtn = 0;
	//int iParamLen = 0;
	//int iCnt = 0;//iSize = 0, 
	int i = 0;
	//int iValue = 0;
	int iPos = 0;
	cJSON * pSubArr = NULL, *pSubArrToJson = NULL;
	
	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	//cJSON *pSubArray = cJSON_GetObjectItem((cJSON *)pJson, "param");
	/*cJSON *pSubArray = (cJSON *)pJson;
	if(NULL == pSubArray)
	{
		// get "error" from json faild
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;		
	}

	*pSize = cJSON_GetArraySize(pSubArray);
	printf("*pSize:%d\n", *pSize);
	for(iCnt = 0; iCnt < *pSize; iCnt++)
	{
		pSubArr = cJSON_GetArrayItem(pSubArray, iCnt);
		if(NULL == pSubArr)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArr!\r\n",  __FILE__, __FUNCTION__, __LINE__);		 
			continue;
		}*/

		pSubArr = (cJSON *)pJson;

		DEBUGMSG(1,("1 gCrrMaxPoolId:%d\n", gCrrMaxPoolId));
		snprintf((char *)dest+iPos, destLen, "%d\"", ++gCrrMaxPoolId);
		iPos += strlen((char *)dest+iPos);

		for (i = 0; i < dim(szPoolParam) - 1; i++)
		{
			pSubArrToJson = cJSON_GetObjectItem(pSubArr, szPoolParam[i]);
			if(NULL != pSubArrToJson)
			{
				DEBUGMSG(1,("url:%s\n", pSubArrToJson->valuestring));
				snprintf((char *)dest+iPos, destLen, "%s\"", pSubArrToJson->valuestring);
				iPos += strlen((char *)dest+iPos);
			}
			else
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArrToJson, szPoolParam :%s!\r\n",  
					__FILE__, __FUNCTION__, __LINE__, szPoolParam[i]);
				//if (iCnt == 0 && iSubSize != dim(szPoolParam))
				return E_COMMAND_ERROR_JSON;
				//break;
			}
		}
		/*if(NULL == pSubArrToJson)
		{
			continue;
		}*/

		pSubArrToJson = cJSON_GetObjectItem(pSubArr, szPoolParam[3]);
		DEBUGMSG(0,("algo:%d\n", pSubArrToJson->valueint));
		if(NULL == pSubArrToJson)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArrToJson, szPoolParam :%s!\r\n",  
				__FILE__, __FUNCTION__, __LINE__, szPoolParam[3]);	
			return E_COMMAND_ERROR_JSON;
		}
		else
		{
			if (iCnt == *pSize - 1)
			{
				snprintf((char *)dest+iPos, destLen, "%d", pSubArrToJson->valueint);
			}
			else
			{
				snprintf((char *)dest+iPos, destLen, "%d ", pSubArrToJson->valueint);
			}
			iPos += strlen((char *)dest+iPos);
		}
	//} 

	DEBUGMSG(1,("iPos:%d\n", iPos));
	DEBUGMSG(1,("rtn:%d, Dpool dest:%s\n", rtn, (char *)dest));
	
	return rtn;
}

//pool_id 1~6
int SetAddPoolToList(void *destPoolBuf, int destLenPoolBuf)
{
	int rtn = 0, index = 0;
	E_ALGO eAlgo;
	stPoolConfInfo *val = NULL;
	structList *pList = NULL;
	list_node_t *node = NULL, *NodeBTCTemp = NULL, *NodeLTCTemp = NULL;
	int iPoolIdTemp = 0;
	char delim[] = "\"";
	int argc = 0;
	char *argv[BUFFER_LEN_256] = {0};
	int j = 0;

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	DEBUGMSG(1,("destPoolBuf:%s, destLenPoolBuf:%d\n", (char *)destPoolBuf, destLenPoolBuf));
	if (Parse(destPoolBuf, &argc, argv, delim) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return (rtn = 2);
	}
	
	DEBUGMSG(1,("argc :%d\n", argc ));
	for (j =0; j < argc ; j++)
	{
		DEBUGMSG(1,("PackArgv[%d]:%s\n", j, argv[j]));
	}

	if (argc != COMMANDLINE_5)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, PackArgc:%d != COMMANDLINE_5\r\n",  
			__FILE__, __FUNCTION__, __LINE__, argv);	
		return -1;
	}

	//index = ParseAlgo(argc, argv, COMMANDLINE_4, &eAlgo);

	//BTC/LTC  0/1 
	eAlgo = strtoul(argv[COMMANDLINE_4], 0, 0) - 1;
	if (eAlgo > E_ALGO_DUAL || eAlgo < 0)
	{
		return E_POOLCNF_ERROR_ALGO;
	}

	iPoolIdTemp = strtoul(argv[COMMANDLINE_0], 0, 0);

	val = malloc(sizeof(stPoolConfInfo));
	memset(val, 0, sizeof(stPoolConfInfo));

	//pool_id不能为0
	if (!iPoolIdTemp)
	{
		return E_POOLCNF_ERROR_POOLID_NOZERO;
	}
	DEBUGMSG(1,("eAlgo:%d\n", eAlgo));

	/*switch(eAlgo)
	{
	case E_ALGO_BTC:
		val->eAlgo = eAlgo;
		pList = &stLTCPoolConfList;
		val->iPoolId = iPoolIdTemp;
		index = COMMANDLINE_1;

		strncpy((char *)val->szUrl, argv[index++],sizeof(val->szUrl));
		strncpy((char *)val->szUser, argv[index++], sizeof(val->szUser));
		strncpy((char *)val->szPasswd, argv[index++], sizeof(val->szPasswd));
		//插入链表
		node = pList->ListNodeNew(val);
		assert(node->val == val);
		pList->ListRpush(pList->List, node, pList->ListLenLock);
		rtn = 0;
		break;

	case E_ALGO_LTC:
		val->eAlgo = eAlgo;
		pList = &stBTCPoolConfList;		
		//init
		val->iPoolId = iPoolIdTemp;
		index = COMMANDLINE_1;
		
		strncpy((char *)val->szUrl, argv[index++],sizeof(val->szUrl));
		strncpy((char *)val->szUser, argv[index++], sizeof(val->szUser));
		strncpy((char *)val->szPasswd, argv[index++], sizeof(val->szPasswd));		
		//插入链表
		node = pList->ListNodeNew(val);
		assert(node->val == val);
		pList->ListRpush(pList->List, node, pList->ListLenLock);
		rtn = 0;
		break;
		
	case E_ALGO_DUAL:
		free(val);
		rtn = E_POOLCNF_ERROR_DUAL;
		break;

	default:
		free(val);
		break;
	}*/
		

#if 1	
	switch(eAlgo)
	{
	case E_ALGO_BTC:
		val->eAlgo = eAlgo;
		//*(E_ALGO*)eAlgoTemp = eAlgo;

		pList = &stLTCPoolConfList;
		//查找BTC pool_id	
		
		NodeLTCTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
		DEBUGMSG(1,("1 @NodeLTCTemp:%p, iPoolIdTemp:%d\n", NodeLTCTemp, iPoolIdTemp));
		if (NodeLTCTemp != NULL)
		{
			pList->ListRemove(pList->List, NodeLTCTemp);
			NodeLTCTemp = NULL;
		}		
		DEBUGMSG(1,("2 @NodeLTCTemp:%p\n", NodeLTCTemp));

		pList = &stBTCPoolConfList;
 		//查找LTC pool_id	
 		NodeBTCTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);

		DEBUGMSG(1,("1 @NodeBTCTemp:%p, iPoolIdTemp:%d\n", NodeBTCTemp, iPoolIdTemp));

		index = COMMANDLINE_1;
		if (NodeBTCTemp == NULL && NodeLTCTemp == NULL)
		{	
			//init
			val->iPoolId = iPoolIdTemp;
			
			strncpy((char *)val->szUrl, argv[index++],sizeof(val->szUrl));
			strncpy((char *)val->szUser, argv[index++], sizeof(val->szUser));
			strncpy((char *)val->szPasswd, argv[index++], sizeof(val->szPasswd));
			//插入链表
			node = pList->ListNodeNew(val);
			assert(node->val == val);
			pList->ListRpush(pList->List, node, pList->ListLenLock);
			rtn = 0;
		}
		else
		{
			((stPoolConfInfo *)NodeBTCTemp->val)->iPoolId = iPoolIdTemp;
			//index = COMMANDLINE_1;
			strncpy((char *)((stPoolConfInfo *)NodeBTCTemp->val)->szUrl, argv[index++],sizeof(((stPoolConfInfo *)NodeBTCTemp->val)->szUrl));
			strncpy((char *)((stPoolConfInfo *)NodeBTCTemp->val)->szUser, argv[index++], sizeof(((stPoolConfInfo *)NodeBTCTemp->val)->szUser));
			strncpy((char *)((stPoolConfInfo *)NodeBTCTemp->val)->szPasswd, argv[index++], sizeof(((stPoolConfInfo *)NodeBTCTemp->val)->szPasswd));

			((stPoolConfInfo *)NodeBTCTemp->val)->eAlgo = eAlgo;
			free(val);
			//return E_POOLCNF_ERROR_POOLID;
		}
		break;

	case E_ALGO_LTC:
		val->eAlgo = eAlgo;
		//*(E_ALGO*)eAlgoTemp = eAlgo;

		pList = &stBTCPoolConfList;
 		//查找BTC pool_id	
 		NodeBTCTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
		if (NodeBTCTemp != NULL)
		{
			pList->ListRemove(pList->List, NodeBTCTemp);
			NodeBTCTemp = NULL;
		}
		
		pList = &stLTCPoolConfList;
		//查找LTC pool_id	
		//iPoolIdTemp = strtoul(argv[iStartPos], 0, 0);
		NodeLTCTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
		index = COMMANDLINE_1;
		if (NodeLTCTemp == NULL && NodeBTCTemp == NULL)
		{
			//init
			val->iPoolId = iPoolIdTemp;
			//index = 2;
			strncpy((char *)val->szUrl, argv[index++],sizeof(val->szUrl));
			strncpy((char *)val->szUser, argv[index++], sizeof(val->szUser));
			strncpy((char *)val->szPasswd, argv[index++], sizeof(val->szPasswd));		
			//插入链表
			node = pList->ListNodeNew(val);
			assert(node->val == val);
			pList->ListRpush(pList->List, node, pList->ListLenLock);
			rtn = 0;
		}
		else
		{
			((stPoolConfInfo *)NodeLTCTemp->val)->iPoolId = iPoolIdTemp;
			//index = 2;
			strncpy((char *)((stPoolConfInfo *)NodeLTCTemp->val)->szUrl, argv[index++],sizeof(((stPoolConfInfo *)NodeLTCTemp->val)->szUrl));
			strncpy((char *)((stPoolConfInfo *)NodeLTCTemp->val)->szUser, argv[index++], sizeof(((stPoolConfInfo *)NodeLTCTemp->val)->szUser));
			strncpy((char *)((stPoolConfInfo *)NodeLTCTemp->val)->szPasswd, argv[index++], sizeof(((stPoolConfInfo *)NodeLTCTemp->val)->szPasswd));

			((stPoolConfInfo *)NodeLTCTemp->val)->eAlgo = eAlgo;
			free(val);
			//return E_POOLCNF_ERROR_POOLID;	
		}
		break;

	case E_ALGO_DUAL:
		free(val);
		rtn = E_POOLCNF_ERROR_DUAL;
		break;

	default:
		free(val);
		break;
	}
#endif
	return rtn;
}

int SetDelay(stPartData *pParData)
{
	int rtn = 0;
	int iQueryCount = 0;
	
	//id处理 太多 怎么办，等等 处理了
	if (CommMonitorFlag.bMassFlag)
	{
		MSleep(1500);
		pthread_mutex_lock(CommMonitorFlag.OnlineCountLock);
		for(iQueryCount = 0; iQueryCount < QUERY_COUNT; iQueryCount++)
		//for(iQueryCount = 0; iQueryCount < 5; iQueryCount++)//调试用
		{
			MSleep(1000);
#ifdef _DEBUG_MANY_DEV_	
		printf("1 iRecvLen:%d, iSendLen:%d, iOnlineCount:%d\n", 
				CommMonitorFlag.iRecvLen, CommMonitorFlag.iSendLen, CommMonitorFlag.iOnlineCount);			
#endif
			DEBUGMSG(1,("2 iRecvLen:%d, iSendLen:%d, iOnlineCount:%d\n", 
				CommMonitorFlag.iRecvLen, CommMonitorFlag.iSendLen, CommMonitorFlag.iOnlineCount));
			if (CommMonitorFlag.iRecvLen == CommMonitorFlag.iSendLen
				&& CommMonitorFlag.iRecvLen == CommMonitorFlag.iOnlineCount)
			{
				CommMonitorFlag.iRecvLen = CommMonitorFlag.iSendLen = 0;
				break;
			}
			else
			{
				continue;
			}
		}
		pthread_mutex_unlock(CommMonitorFlag.OnlineCountLock);
	}
	else
	{
		for(iQueryCount = 0; iQueryCount < QUERY_COUNT; iQueryCount++)
		//for(iQueryCount = 0; iQueryCount < 5; iQueryCount++)//调试用
		{
			MSleep(1000);
			DEBUGMSG(1,("iRecvLen:%d, iSendLen:%d, iParListLen:%d\n", 
				CommMonitorFlag.iRecvLen, CommMonitorFlag.iSendLen, pParData->iPartListLen));
			if (CommMonitorFlag.iRecvLen == CommMonitorFlag.iSendLen
				&& CommMonitorFlag.iRecvLen <= pParData->iPartListLen)
			{
				CommMonitorFlag.iRecvLen = CommMonitorFlag.iSendLen = 0;
				break;
			}
			else
			{
				continue;
			}
		}
	}
	//发送后无论是否收到 都全部清零
	CommMonitorFlag.iRecvLen = CommMonitorFlag.iSendLen = CommMonitorFlag.iOnlineCount = 0;

	DEBUGMSG(1,("1111111111111 iRecvLen:%d, iSendLen:%d, iOnlineCount:%d\n", 
		CommMonitorFlag.iRecvLen, CommMonitorFlag.iSendLen, CommMonitorFlag.iOnlineCount));

	return rtn;
}

int ModifyBTCPool(cJSON *pSubArray, stPoolFixedConf *pPoolConf, int iCnt, int iIndex)
{
	cJSON *pSubArr = NULL;
	cJSON *pSubArrToJson = NULL;

	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	pSubArr = cJSON_GetArrayItem(pSubArray, iCnt);
	if(NULL == pSubArr)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArr!\r\n",  __FILE__, __FUNCTION__, __LINE__);		 
		return E_COMMAND_ERROR_JSON;
	}
	
	pSubArrToJson = cJSON_GetObjectItem(pSubArr, szPoolParam[0]);
	if(NULL == pSubArrToJson)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArrToJson, szPoolParam :%s!\r\n",  
			__FILE__, __FUNCTION__, __LINE__, szPoolParam[0]);
		return E_COMMAND_ERROR_JSON;		
	}
	else
	{
		DEBUGMSG(1,("url:%s\n", pSubArrToJson->valuestring));
		strcpy((char *)pPoolConf->stBTCPoolConf[iIndex].szUrl, pSubArrToJson->valuestring);	
	}
	
	pSubArrToJson = cJSON_GetObjectItem(pSubArr, szPoolParam[1]);
	if(NULL == pSubArrToJson)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArrToJson, szPoolParam :%s!\r\n",  
			__FILE__, __FUNCTION__, __LINE__, szPoolParam[1]);
		return E_COMMAND_ERROR_JSON;	
	}
	else
	{
		DEBUGMSG(1,("szUser:%s\n", pSubArrToJson->valuestring));
		strcpy((char *)pPoolConf->stBTCPoolConf[iIndex].szUser, pSubArrToJson->valuestring);
	}
	
	pSubArrToJson = cJSON_GetObjectItem(pSubArr, szPoolParam[2]);
	if(NULL == pSubArrToJson)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArrToJson, szPoolParam :%s!\r\n",  
			__FILE__, __FUNCTION__, __LINE__, szPoolParam[2]);
		return E_COMMAND_ERROR_JSON;		
	}
	else
	{
		DEBUGMSG(1,("szPasswd:%s\n", pSubArrToJson->valuestring));
		strcpy((char *)pPoolConf->stBTCPoolConf[iIndex].szPasswd, pSubArrToJson->valuestring);
	}

	return 0;
}

int ModifyLTCPool(cJSON *pSubArray, stPoolFixedConf *pPoolConf, int iCnt, int iIndex)
{
	cJSON *pSubArr = NULL;
	cJSON *pSubArrToJson = NULL;
	
	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	pSubArr = cJSON_GetArrayItem(pSubArray, iCnt);
	if(NULL == pSubArr)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArr!\r\n",  __FILE__, __FUNCTION__, __LINE__);		 
		return E_COMMAND_ERROR_JSON;
	}
	
	pSubArrToJson = cJSON_GetObjectItem(pSubArr, szPoolParam[0]);
	if(NULL == pSubArrToJson)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArrToJson, szPoolParam :%s!\r\n",  
			__FILE__, __FUNCTION__, __LINE__, szPoolParam[0]);
		return E_COMMAND_ERROR_JSON;		
	}
	else
	{
		DEBUGMSG(1,("url:%s\n", pSubArrToJson->valuestring));
		strcpy((char *)gPoolDefaultConf.stLTCPoolConf[iIndex].szUrl, pSubArrToJson->valuestring);	
	}
	
	pSubArrToJson = cJSON_GetObjectItem(pSubArr, szPoolParam[1]);
	if(NULL == pSubArrToJson)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArrToJson, szPoolParam :%s!\r\n",  
			__FILE__, __FUNCTION__, __LINE__, szPoolParam[1]);
		return E_COMMAND_ERROR_JSON;	
	}
	else
	{
		DEBUGMSG(1,("szUser:%s\n", pSubArrToJson->valuestring));
		strcpy((char *)gPoolDefaultConf.stLTCPoolConf[iIndex].szUser, pSubArrToJson->valuestring);
	}
	
	pSubArrToJson = cJSON_GetObjectItem(pSubArr, szPoolParam[2]);
	if(NULL == pSubArrToJson)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArrToJson, szPoolParam :%s!\r\n",  
			__FILE__, __FUNCTION__, __LINE__, szPoolParam[2]);
		return E_COMMAND_ERROR_JSON;		
	}
	else
	{
		DEBUGMSG(1,("szPasswd:%s\n", pSubArrToJson->valuestring));
		strcpy((char *)gPoolDefaultConf.stLTCPoolConf[iIndex].szPasswd, pSubArrToJson->valuestring);
	}

	return 0;
}

//设置可控制命令
int PackMonitorToComm(int argc, char **argv, void *dest, int destLen, stPartData *pParData)
{
	int rtn = -1;
	int i = 0, j = 0;//, iQueryCount = 0;
	//BYTE timeBuf[BUFFER_LEN_256] = {0};
	devlist_st *pNode, *tmp = NULL;
	int iPos = 0;
	E_SET_MONITOR eSetMass = E_SET_MONITOR_ALL_SUCC;
	int iTotListLen = 0, ioffLineCount = 0;//, iParListLen = 0;//
	int iId = 0;
	//int iPartId[BUFFER_LEN_256] = {0};
	
	DEBUGMSG(1,("%s..., destLen:%d\r\n", __FUNCTION__, destLen));
	//DEBUGMSG(1,("################pSrcBuf:%p\r\n", idControlInput[0].pIdCommandPack[1].pSrcBuf));
		
	rtn = ParseMonitorCommand(argc, argv, 1, pParData);

	DEBUGMSG(1,("ParseMonitorCommand  rtn:%d, CommMonitorFlag.bMassFlag:%d\n", rtn, CommMonitorFlag.bMassFlag));

	if(rtn == 0 || rtn == E_COMMAND_ERROR_ID)
	{
		//iParListLen++;
		//把id保存下来
		/*DEBUGMSG(1,("1 iPartListLen:%d\n", pParData->iPartListLen));
		if (rtn == 0)
			pParData->iPartIdBuf[pParData->iPartListLen++] = strtoul(argv[2], 0, 0);
		else
			pParData->iPartLen--;*/
		
		//DEBUGMSG(1,("2 iPartIdBuf[%d]:%d, argv[2]:%s\n", pParData->iPartListLen - 1, pParData->iPartIdBuf[pParData->iPartListLen - 1], argv[2]));
		//iPartLen = id 个数，如果是all 则iPartLen = 1
		//DEBUGMSG(1,("2 iPartLen:%d, iParListLen:%d\n", pParData->iPartLen, pParData->iPartListLen));
		//if (pParData->iPartLen == pParData->iPartListLen)
	//{
		//delay处理
		SetDelay(pParData);
		
		if (CommMonitorFlag.bMassFlag)
		{
			//因为群发，数据多，最多延迟15s。
			//MSleep(15000);
			CommMonitorFlag.bMassFlag = FALSE;

			pthread_mutex_lock(idControlInput[i].pList->ListLenLock);
			iTotListLen = idControlInput[i].pList->CountUsersStr(*idControlInput[i].pList->StrHUsers); 
			pthread_mutex_unlock(idControlInput[i].pList->ListLenLock);

			pthread_mutex_lock(CommMonitorFlag.UserSeqLock);
			//for (j = 0; j < MAX_CLIENTS; j++)
			//{
			//if (CommMonitorFlag.bRecvSeqFlag[j] == TRUE)
			//{
			//GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
			//snprintf((char *)((stClientSendData*)dest)->bySeCmdBuf, destLen, "%s\t", (char *)timeBuf);
			//iPos = strlen((char *)((stClientSendData*)dest)->bySeCmdBuf);
			
			pthread_mutex_lock(idControlInput[i].pList->MonitorRemLock);
			HASH_ITER(hh, *idControlInput[i].pList->StrHUsers, pNode, tmp)
			{
				pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
				//只对在线的设备进行发送重新登录命令，同时关闭掉了状态查询。
				if (((stCellConn *)pNode->val)->CellData.bCellOnline == TRUE)
				{
					DEBUGMSG(0,("+++++++++++++++iCellId:%s, eCellComm:%d\r\n", ((stCellConn *)pNode->val)->CellData.szCellId, 
						((stCellConn *)pNode->val)->eCellComm));
					if (((stCellConn *)pNode->val)->eCellComm)
					{
						//对于没有收到的结点，序列号也复位。
						((stCellConn *)pNode->val)->uSeqId = 0;
						
						if (!eSetMass)
						{
							eSetMass = E_SET_MONITOR_PART_FAIL;
							snprintf((char *)dest + iPos, destLen, "%s", szRetBuf[1]);
							iPos += strlen((char *)dest + iPos);
						}	
						snprintf((char *)dest + iPos, destLen, " %s", 
							((stCellConn *)pNode->val)->CellData.szCellId);	
						//iPos = iPos + sizeof(((stCellConn *)pNode->val)->CellData.iCellId) + 1;
						iPos = strlen((char *)dest);
					}
				}
				else
				{
					ioffLineCount++;
					if (ioffLineCount == iTotListLen)
					{
						eSetMass = E_SET_MONITOR_ALL_INTER;
						snprintf((char *)dest + iPos, destLen, "%s", szRetBuf[2]);
						iPos += strlen((char *)dest + iPos);
					}					
				}
				pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
			}
			pthread_mutex_unlock(idControlInput[i].pList->MonitorRemLock); 
			pthread_mutex_unlock(CommMonitorFlag.UserSeqLock);

			if (eSetMass == E_SET_MONITOR_ALL_SUCC)
				snprintf((char *)dest+ iPos, destLen, "%s", szRetBuf[0]);

			return 0;

			/*switch(eSetMass)
			{
				case E_SET_MONITOR_ALL_SUCC:
					snprintf((char *)dest + iPos, destLen, " %s\r\n", "null");
					break;

				case E_SET_MONITOR_ALL_INTER:
					snprintf((char *)dest + iPos, destLen, " %s\r\n", "interruption");
					break;

				case E_SET_MONITOR_PART_FAIL:
					//snprintf((char *)((stClientSendData*)dest)->bySeCmdBuf + iPos, destLen, "%s\r\n", "");
					break;

				default:
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, eSetMass is error!\r\n",	__FILE__, __FUNCTION__, __LINE__);
					break;
			}*/

			//CommMonitorFlag.bRecvSeqFlag[j] = FALSE;
			
			//DEBUGMSG(1,("--------------i:%d\r\n", i));
			//}
			//}
			//pthread_mutex_unlock(CommMonitorFlag.UserSeqLock);
		}
		else
		{	
			DEBUGMSG(1,("iPartListLen:%d\n", pParData->iPartListLen));
			for (j = 0; j < pParData->iPartListLen; j++)
			{
				//iId = pParData->iPartIdBuf[j];
				DEBUGMSG(1,("iId:%d, iPartIdBuf[%d]:%s\n", iId, j, pParData->iPartIdBuf[j]));
				pthread_mutex_lock(CommMonitorFlag.UserSeqLock);
				//DEBUGMSG(1,("111,i:%d\n", i));
				pthread_mutex_lock(idControlInput[i].pList->MonitorRemLock);
				//pNode = idControlInput[i].pList->FindUserInt(idControlInput[i].pList->HashUsers, iId);
				pNode = idControlInput[i].pList->FindUserStr(idControlInput[i].pList->StrHUsers, pParData->iPartIdBuf[j]);
				//DEBUGMSG(1,("@pNode:%p\n", pNode));
				if (pNode != NULL)
				{
					pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
					//只对在线的设备进行发送重新登录命令，同时关闭掉了状态查询。
					if (((stCellConn *)pNode->val)->CellData.bCellOnline == TRUE)
					{
						DEBUGMSG(1,("+++++++++++++++szCellId:%s, eCellComm:%d\r\n", ((stCellConn *)pNode->val)->CellData.szCellId, 
							((stCellConn *)pNode->val)->eCellComm));
						if (((stCellConn *)pNode->val)->eCellComm)
						{
							//对于没有收到的结点，序列号也复位。
							((stCellConn *)pNode->val)->uSeqId = 0;
							
							((stCellConn *)pNode->val)->eCellComm = E_COMMAND_TYPE_STAT;
							if (!eSetMass)
							{
								eSetMass = E_SET_MONITOR_PART_FAIL;
								snprintf((char *)dest + iPos, destLen, "%s", szRetBuf[1]);
								iPos += strlen((char *)dest + iPos);
							}
							snprintf((char *)dest + iPos, destLen, " %s", 
								((stCellConn *)pNode->val)->CellData.szCellId);	
							//iPos = iPos + sizeof(((stCellConn *)pNode->val)->CellData.iCellId) + 1;
							iPos += strlen((char *)dest + iPos);
						}
					}
					else
					{
						ioffLineCount++;
						if (ioffLineCount == pParData->iPartListLen)
						{
							if (!eSetMass)
							{
								eSetMass = E_SET_MONITOR_ALL_INTER;
								snprintf((char *)dest + iPos, destLen, "%s", szRetBuf[2]);
								iPos += strlen((char *)dest + iPos);
							}
						}
					}
					pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
				}
				else
				{
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, iPartIdBuf[%d] is error\r\n",	
										__FILE__, __FUNCTION__, __LINE__, iId);
				}
				pthread_mutex_unlock(idControlInput[i].pList->MonitorRemLock); 
				pthread_mutex_unlock(CommMonitorFlag.UserSeqLock);

				if (eSetMass == E_SET_MONITOR_ALL_SUCC)
					snprintf((char *)dest+ iPos, destLen, "%s", szRetBuf[0]);

				DEBUGMSG(1,("000000 szParseDest:%s,  %d\r\n", (char *)dest, destLen));
				//return 0;
				//if (ioffLineCount == iParListLen)
				//	eSetMass = E_SET_MONITOR_ALL_INTER;

				/*DEBUGMSG(1,("eSetMass:%d\n", eSetMass));
				switch(eSetMass)
				{
					case E_SET_MONITOR_ALL_SUCC:
						snprintf((char *)dest+ iPos, destLen, "NULL");
						break;

					case E_SET_MONITOR_ALL_INTER:
						snprintf((char *)dest + iPos, destLen, "Interruption");
						break;

					case E_SET_MONITOR_PART_FAIL:
						snprintf((char *)dest + iPos, destLen, " PartInter");
						//snprintf((char *)((stClientSendData*)dest)->bySeCmdBuf + iPos, destLen, "%s\r\n", "");
						break;

					default:
						GetLocalTimeLog();
						ap_log_debug_log("\t\t%s, %s, %d, eSetMass is error!\r\n",	__FILE__, __FUNCTION__, __LINE__);
						break;
				}*/

				//CommMonitorFlag.bRecvSeqFlag[j] = FALSE;
			}			
			//MSleep(15000);
			return 0;
		}	
	//}
		
						
		DEBUGMSG(1,("++++++++++++++++i++++++++++++++:%d\r\n", i));
		//GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
		//snprintf((char *)dest, destLen, "Failed");		
	}

	/*else if (rtn > 0)
	{
		//GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
		//snprintf((char *)dest, destLen, "%s", (char *)gCommandErr[rtn - 1].cmd);
	}*/
	DEBUGMSG(1,("szParseDest:%s,  %d\r\n", (char *)dest, destLen));
	DEBUGMSG(1,("2 szIdbufRtn:%s\n", (char *)CommMonitorFlag.szIdbufRtn));	
	return rtn;
}

//设置cell重新登录
int SetSpecifyPool(int argc, char **argv, void *dest, int destLen, void *pParData)
{
	int rtn = 0;
	char delim[] = " ";
	int PackArgc = 0;
	char *PackArgv[BUFFER_LEN_256] = {0};
	char Packbuf[BUFFER_LEN_256] = {0};
	int i = 0;
	//int iPartLen = 0;
	//stPartData ParData;
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	/*if(argc != COMMANDLINE_3)
	{
		gUserComdErr[E_SHELL_ERROR_SET_COMM].UserCmdFunc(NULL, (char *)((stClientSendData*)dest)->bySeCmdBuf, destLen);		
		return E_SHELL_ERROR_SET_COMM;
	}*/

	if (argc == 0)
	{
		return 1;
	}
	((stPartData *)pParData)->iPartLen = argc;
	((stPartData *)pParData)->iPartListLen = 0;
	
	for (i = 0; i < argc; i++)
	{
		if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "comm cell %s setpool", argv[i])) < 0)
			return rtn;

		DEBUGMSG(1,("Packbuf:%s\n", Packbuf));
		if (Parse(Packbuf, &PackArgc, PackArgv, delim) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
			return (rtn = 2);
		}
		
		rtn = PackMonitorToComm(PackArgc, PackArgv, dest, destLen, pParData);		
	}

	DEBUGMSG(1,("SetCell Relogin rtn:%d\n", rtn));

	return rtn;
}

//清空整个矿池链表和数据库
int ParseDelAllPool(int argc, char **argv, void *dest, int destLen, void *pParData)
{
	int rtn = 0, iQueryRtn = 0, iQuerySwitchRtn = 0;
	structList *pList = NULL;
	list_node_t *pNode = NULL;
	int iCount = 0;
	char bufDest[BUFFER_LEN_512] = {0};
	int  i = 0, iPos = 0;
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	pList = &stBTCPoolConfList;
	pthread_mutex_lock(pList->ListLenLock);
	iCount = pList->List->len;
	pthread_mutex_unlock(pList->ListLenLock);

	DEBUGMSG(1,("BTC iCount:%d\n", iCount));
	pthread_mutex_lock(pList->RemoveLock);
	for(i = 0;i < iCount;i++)
	{
		//pList->ListRemove(pList->List, pNode);
		//DEBUGMSG(1,("pList->List->len:%d\n", pList->List->len));
		pNode = pList->ListAt(pList->List, 0);
		if (pNode != 0)
		{
			DEBUGMSG(0,("\tid:%d\r\n\t\turl:'%s'\r\n\t\tuser:'%s'\r\n\t\tpwd:'%s'\r\n", 
				((stPoolConfInfo *)pNode->val)->iPoolId,
				((stPoolConfInfo *)pNode->val)->szUrl, 
				((stPoolConfInfo *)pNode->val)->szUser,
				((stPoolConfInfo *)pNode->val)->szPasswd
				));
			pList->ListRemove(pList->List, pNode);
		}
	}
	//if (iCount != 0)
	//	pList->ListDestroy(pList->List);
	pthread_mutex_unlock(pList->RemoveLock);

	pList = &stLTCPoolConfList;
	iCount = 0;
	pthread_mutex_lock(pList->ListLenLock);
	iCount = pList->List->len;
	pthread_mutex_unlock(pList->ListLenLock);

	DEBUGMSG(1,("LTC iCount:%d\n", iCount));
	pthread_mutex_lock(pList->RemoveLock);
	for(i = 0;i < iCount;i++)
	{
		//pList->ListRemove(pList->List, pNode);
		//DEBUGMSG(1,("pList->List->len:%d\n", pList->List->len));	
		pNode = pList->ListAt(pList->List, 0);
		if (pNode != 0)
		{
			DEBUGMSG(0,("\tid:%d\r\n\t\turl:'%s'\r\n\t\tuser:'%s'\r\n\t\tpwd:'%s'\r\n", 
				((stPoolConfInfo *)pNode->val)->iPoolId,
				((stPoolConfInfo *)pNode->val)->szUrl, 
				((stPoolConfInfo *)pNode->val)->szUser,
				((stPoolConfInfo *)pNode->val)->szPasswd
				));		
			pList->ListRemove(pList->List, pNode);
		}
	}
	//if (iCount != 0)
	//	pList->ListDestroy(pList->List);
	pthread_mutex_unlock(pList->RemoveLock);

	//清空数据库(switch_db)中表(switch_base)里面的PoolIdPri
	memset(&gPoolDefaultConf, 0, sizeof(stPoolFixedConf));
	memset(&gPoolSpecifyConf, 0, sizeof(stPoolFixedConf));
	memset((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri, 0, sizeof(gpSwitchDbInfo->DbSwi.szPoolIdPri));
	gCrrMaxPoolId = 0;

	//清空数据库
	DEBUGMSG(1,("bIniPoolConfig:%d\n", bIniPoolConfig));
	if (bIniPoolConfig)
	{
		PoolConfigQuery[E_QUERY_TYPE_DELETE - 1].QueryPackFunc2(
														NULL, 
														&gCfgDataDL.PoolInfo, 
														&gCfgDataDL.PoolConfigColName, 
														bufDest, sizeof(bufDest));
		DEBUGMSG(1,("bufDest:%s\r\n", bufDest));
		iQueryRtn = MysqlFunc.dbDoQuery(&myCellBase, bufDest, E_QUERY_TYPE_UPDATE, NULL);
		memset(bufDest, 0, sizeof(bufDest));


		SwitchBaseQuery[E_QUERY_TYPE_UPDATE - 1].QueryPackFunc1(
															gpSwitchDbInfo, 
															&gCfgDataDL.SwitchBaseInfo, 
															&gCfgDataDL.SwitchBaseColName, 
															bufDest, 
															sizeof(bufDest));
		pthread_mutex_lock(&gSwitchBaseLock);
		iQuerySwitchRtn = MysqlFunc.dbDoQuery(&mySwitchBase, bufDest, E_QUERY_TYPE_UPDATE, NULL);
		pthread_mutex_unlock(&gSwitchBaseLock);
		memset(bufDest, 0, sizeof(bufDest));

		if (!iQueryRtn)
		{
			if (!iQuerySwitchRtn)
				strcpy((char *)dest + iPos, szRetBuf[0]);
			else
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, Delpool, delete poolnodes ok, delete gPoolDefaultConf ok, failed to delete the data in the table!(switch_base)\r\n",	
					__FILE__, __FUNCTION__, __LINE__);
			}
		}
		else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, Delpool, delete poolnodes ok, delete gPoolDefaultConf ok, failed to delete the data in the table!(pool_conf)\r\n",	
				__FILE__, __FUNCTION__, __LINE__);
		}
	}
	else
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d,  bIniPoolConfig error:%d\r\n",	
			__FILE__, __FUNCTION__, __LINE__, bIniPoolConfig);		
	}

	return rtn;
}

//把用户命令cell.reboot打包成Command解析形式的命令
int PackCellRebootMonitorToComm(int argc, char **argv, void *dest, int destLen, void *pParData)
{
	int rtn = 0;
	char delim[] = " ";
	int PackArgc = 0;
	char *PackArgv[BUFFER_LEN_256] = {0};
	char Packbuf[BUFFER_LEN_256] = {0};
	int i = 0;
	//int iPartLen = 0;
	//stPartData ParData;
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	/*if(argc != COMMANDLINE_3)
	{
		gUserComdErr[E_SHELL_ERROR_SET_COMM].UserCmdFunc(NULL, (char *)((stClientSendData*)dest)->bySeCmdBuf, destLen);		
		return E_SHELL_ERROR_SET_COMM;
	}*/

	if (argc == 0)
	{
		return 1;
	}
	((stPartData *)pParData)->iPartLen = argc - 1;
	((stPartData *)pParData)->iPartListLen = 0;
	
	for (i = 1; i < argc; i++)
	{
		if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "comm cell %s reboot %s", argv[i], argv[0])) < 0)
			return rtn;

		DEBUGMSG(1,("Packbuf:%s\n", Packbuf));
		if (Parse(Packbuf, &PackArgc, PackArgv, delim) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
			return (rtn = 2);
		}
		
		rtn = PackMonitorToComm(PackArgc, PackArgv, dest, destLen, pParData);		
	}

	DEBUGMSG(1,("Pack MonitorToComm rtn:%d\n", rtn));

	return rtn;
}

//把用户命令cell.setled打包成Command解析形式的命令
int PackCellSetLedMonitorToComm(int argc, char **argv, void *dest, int destLen, void *pParData)
{
	int rtn = 0;
	char delim[] = " ";
	int PackArgc = 0;
	char *PackArgv[BUFFER_LEN_256] = {0};
	char Packbuf[BUFFER_LEN_256] = {0};
	int i = 0;
	//int iPartLen = 0;
	//stPartData ParData;
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	/*if(argc != COMMANDLINE_3)
	{
		gUserComdErr[E_SHELL_ERROR_SET_COMM].UserCmdFunc(NULL, (char *)((stClientSendData*)dest)->bySeCmdBuf, destLen);		
		return E_SHELL_ERROR_SET_COMM;
	}*/

	if (argc < 1)
	{
		return 1;
	}

	DEBUGMSG(1,("argc:%d, argv[0]:%s\n", argc, argv[0]));

	((stPartData *)pParData)->iPartLen = argc - 1; 
	((stPartData *)pParData)->iPartListLen = 0;

	for (i = 1; i < argc; i++)
	{
		DEBUGMSG(1,("argv[%d]:%s\n", i, argv[i]));
		if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "comm cell %s setled %s", argv[i], argv[0])) < 0)
			return rtn;

		DEBUGMSG(1,("Packbuf:%s\n", Packbuf));
		if (Parse(Packbuf, &PackArgc, PackArgv, delim) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
			return (rtn = 2);
		}
		rtn = PackMonitorToComm(PackArgc, PackArgv, dest, destLen, pParData);
	}

	DEBUGMSG(1,("Pack MonitorToComm rtn:%d\n", rtn));

	return rtn;
}

//把用户命令cell.addpool打包成Command解析形式的命令
int PackCellAddPoolMonitorToComm(int argc, char **argv, void *dest, int destLen, void *pParData)
{
	int rtn = 0, iQueryRtn = 0, iErrIndex = 0;
	char Packbuf[BUFFER_LEN_256] = {0};
	char bufDest[BUFFER_LEN_512] = {0};
	E_ALGO eAlgo;
	//BYTE timeBuf[BUFFER_LEN_256] = {0};
	int iPos = 0;
	int i = 0;
	char delim[] = "\"";
	int PackArgc = 0;
	char *PackArgv[BUFFER_LEN_256] = {0};
	int j = 0;
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	if (argc < 1)
	{
		return 1;
	}

	for (i = 0; i < argc; i++)
	{
		//if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "comm cell %s reboot all", argv[i])) < 0)
		//	return rtn;

		DEBUGMSG(1,("argv[%d]:%s\n", i, argv[i]));
		if (Parse(argv[i], &PackArgc, PackArgv, delim) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
			return (rtn = 2);
		}
		
		DEBUGMSG(1,("PackArgc:%d\n", PackArgc));
		for (j =0; j < PackArgc; j++)
		{
			DEBUGMSG(1,("PackArgv[%d]:%s\n", j, PackArgv[j]));
		}
		
		if ((iErrIndex = ParseAddPool(PackArgc, PackArgv, 0, (void*)&eAlgo)) > 0)
		{
			DEBUGMSG(0,("iErrIndex:%d, E_SHELL_ERROR_ADDPOOL:%d\r\n", iErrIndex, E_SHELL_ERROR_ADDPOOL));
			//去掉算法错误的pool_id
			DEBUGMSG(0,("1 i:%d, argc:%d, gCrrMaxPoolId:%d\n", i, argc, gCrrMaxPoolId));
			gCrrMaxPoolId -= (argc - i);
			DEBUGMSG(0,("2 gCrrMaxPoolId:%d\n", gCrrMaxPoolId));
			if (gUserComdErr[E_SHELL_ERROR_ADDPOOL].UserCmdFunc(gPoolConfErr[iErrIndex - 1].cmd, (char *)dest + iPos, destLen) < 0)
			{
				DEBUGMSG(0,("1 ComdErr rtn:%d\n", rtn));
				return 2;//(rtn = E_SHELL_ERROR_ADDPOOL);
			}		
			else
			{
				DEBUGMSG(0,("2 ComdErr rtn:%d\n", rtn));
				return (rtn = E_COMMAND_ERROR_POOL);
			}				
		}
		
		if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "%s,'%s','%s','%s',%d", 
			PackArgv[0], PackArgv[1], PackArgv[2], PackArgv[3], eAlgo)) < 0)
			return rtn;

		DEBUGMSG(1,("Packbuf:%s\r\n", Packbuf));
		PoolConfigQuery[E_QUERY_TYPE_INSERT - 1].QueryPackFunc1(
															Packbuf, 
															&gCfgDataDL.PoolInfo, 
															&gCfgDataDL.PoolConfigColName, 
															bufDest, 
															sizeof(bufDest));
		pthread_mutex_lock(&gPoolConfigLock);
		iQueryRtn = MysqlFunc.dbDoQuery(&myPoolConfig, bufDest, E_QUERY_TYPE_INSERT, NULL);
		pthread_mutex_unlock(&gPoolConfigLock);


		memset(bufDest, 0, sizeof(bufDest));
		if (!iQueryRtn)
		{
			strcpy((char *)dest + iPos, szRetBuf[0]);
		}
		rtn = iQueryRtn;		
	}

	return rtn;
}

//删除一个池配置信息
int PackCellDelPoolMonitorToComm(int argc, char **argv, void *dest, int destLen, void *pParData)
{
	int rtn = 0, iQueryRtn = 0, iErrIndex = 0, iQuerySwitchRtn = 0;
	char Packbuf[BUFFER_LEN_256] = {0};
	char bufDest[BUFFER_LEN_512] = {0};
	int iPoolId = 0;
	//int szPoolIdErr[BUFFER_LEN_256] = {0};
	//BYTE timeBuf[BUFFER_LEN_256] = {0};
	int iPos = 0, iArgLen = 0;
	int i = 0;
	BOOL bErrFlag = FALSE;
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	if (argc < 1)
	{
		return 1;
	}

	iArgLen = strlen(argv[0]);
	if (iArgLen == 3 && (strncmp(argv[0], "all", iArgLen) == 0))
	{
		rtn = ParseDelAllPool(argc, argv, dest, destLen, pParData);
		return rtn;
	}

	for (i = 0; i < argc; i++)
	{
		DEBUGMSG(1,("DelPool argv[%d]:%s\n", i, argv[i]));
		iErrIndex = ParseDelPool(argc, argv, i, (void*)&iPoolId, 0);

		if (iErrIndex != 0)
		{
			//把错误的pool_id返回
			if (!bErrFlag)
			{
				bErrFlag = TRUE;
				snprintf((char *)dest + iPos, destLen, "%s %s", szRetBuf[3], argv[i]);
				iPos += strlen((char *)dest + iPos);
			}
			else
			{
				snprintf((char *)dest + iPos, destLen, " %s", argv[i]);
				iPos += strlen((char *)dest + iPos);
			}
			
			continue;
		}
		else
		{
			if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "%d", iPoolId)) < 0)
				return rtn;
			DEBUGMSG(1,("Packbuf:%s\r\n", Packbuf));
			
			PoolConfigQuery[E_QUERY_TYPE_DELETE - 1].QueryPackFunc1(
																Packbuf, 
																&gCfgDataDL.PoolInfo, 
																&gCfgDataDL.PoolConfigColName, 
																bufDest, 
																sizeof(bufDest));
			pthread_mutex_lock(&gPoolConfigLock);
			iQueryRtn = MysqlFunc.dbDoQuery(&myPoolConfig, bufDest, E_QUERY_TYPE_DELETE, NULL);
			pthread_mutex_unlock(&gPoolConfigLock);
			memset(bufDest, 0, sizeof(bufDest));
			
			SwitchBaseQuery[E_QUERY_TYPE_UPDATE - 1].QueryPackFunc1(
																gpSwitchDbInfo, 
																&gCfgDataDL.SwitchBaseInfo, 
																&gCfgDataDL.SwitchBaseColName, 
																bufDest, 
																sizeof(bufDest));
			pthread_mutex_lock(&gSwitchBaseLock);
			iQuerySwitchRtn = MysqlFunc.dbDoQuery(&mySwitchBase, bufDest, E_QUERY_TYPE_UPDATE, NULL);
			pthread_mutex_unlock(&gSwitchBaseLock);
			
			memset(bufDest, 0, sizeof(bufDest));
			if (!iQueryRtn)
			{
				if (!iQuerySwitchRtn)
					strcpy((char *)dest + iPos, szRetBuf[0]);
				else
				{
					//strcpy((char *)dest + iPos, 
					//"Delpool, delete poolnodes ok, delete gPoolDefaultConf ok, failed to delete the data in the table!(switch_base)\r\r\n");
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, Delpool, delete poolnodes ok, delete gPoolDefaultConf ok, failed to delete the data in the table!(switch_base)\r\n",  
						__FILE__, __FUNCTION__, __LINE__);
				}
			}
			else
			{
				//strcpy((char *)dest + iPos, 
				//"Delpool, delete poolnodes ok, delete gPoolDefaultConf ok, failed to delete the data in the table!(pool_conf)\r\r\n");
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, Delpool, delete poolnodes ok, delete gPoolDefaultConf ok, failed to delete the data in the table!(pool_conf)\r\n",	
					__FILE__, __FUNCTION__, __LINE__);
			}
		}
	}

	if (bErrFlag)
	{
		DEBUGMSG(1,("1 iErrIndex:%d, E_SHELL_ERROR_ADDPOOL:%d\r\n", iErrIndex, E_SHELL_ERROR_DELPOOL));
		//只把错误的pool_id以notexist 格式返回
		if (iErrIndex == E_POOLCNF_ERROR_POOLID_EXIST)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, [%d]:%s", 
				__FILE__, __FUNCTION__, __LINE__, iErrIndex, dest); 
			return 0;
		}
		else
		{
			if (gUserComdErr[E_SHELL_ERROR_DELPOOL].UserCmdFunc(gPoolConfErr[iErrIndex - 1].cmd, (char *)dest + iPos, destLen) < 0)
				return 2;
			else
				return (rtn = E_COMMAND_ERROR_POOL);
		}
	}

	rtn = iQueryRtn;

	return rtn;
}

//设置一个默认池配置信息，在新的cell登录时获得这个池的信息
int PackSetdefaultPoolMonitorToComm(int argc, char **argv, void *dest, int destLen, void *pParData)
{
	int rtn = 0, iErrIndex = 0, iQueryRtn = 0, rtnPackShell = 0;
	//char Packbuf[BUFFER_LEN_256] = {0};
	char bufDest[BUFFER_LEN_512] = {0};
	//int iPoolId = 0;
	char bufPoolErr[BUFFER_LEN_16] = {0};
	//BYTE timeBuf[BUFFER_LEN_256] = {0};
	int iPos = 0;
	int i = 0;
	char destPackShell[BUFFER_LEN_32] = {0};
	int destLenPackShell = 0;
	int argcShell = 0;
	char *argvShell[BUFFER_LEN_64] = {0};
	char delimShell[] = " ";


	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	
	if(argc > COMMANDLINE_6 || argc < COMMANDLINE_2)
	{
		//gUserComdErr[E_SHELL_ERROR_SET_COMM].UserCmdFunc(NULL, (char *)dest + iPos, destLen);		
		return E_COMMAND_ERROR_IDNUM;
	}

	destLenPackShell = sizeof (destPackShell);
	rtnPackShell = SetdefaultPoolMonitorToShell(
		argc, argv, destPackShell, destLenPackShell, dest, destLen);

	if (rtnPackShell > 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, [%d]:%s",	
			__FILE__, __FUNCTION__, __LINE__, rtnPackShell, dest);		
	
		return (rtn = E_COMMAND_ERROR_POOL);
	}
	
	DEBUGMSG(1,("destPackShell:%s\n", destPackShell));

	if (Parse(destPackShell, &argcShell, argvShell, delimShell) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Shell parse error!\r\n",	__FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

	DEBUGMSG(1,("argcShell:%d\n", argcShell));
	for (i = 0; i < argcShell; i++)
	{
		DEBUGMSG(1,("argvShell[%d]:%s\n", i, argvShell[i]));
	}
	iErrIndex = ParseSetPool(argcShell, argvShell, 0, (void*)bufPoolErr);

	if (iErrIndex > 0)
	{
		DEBUGMSG(1,("2 iErrIndex:%d, E_SHELL_ERROR_ADDPOOL:%d\r\n", iErrIndex, E_SHELL_ERROR_SETPOOL));
		if (gUserComdErr[E_SHELL_ERROR_SETPOOL].UserCmdFunc(gPoolConfErr[iErrIndex - 1].cmd, (char *)dest + iPos, destLen) < 0)
			return (rtn = E_SHELL_ERROR_SETPOOL);
		else
		{
			//把不存在的pool_id返回
			if (iErrIndex == E_POOLCNF_ERROR_POOLID_EXIST)
			{
				iPos += strlen((char *)dest + iPos);
				snprintf((char *)dest + iPos, destLen, "\t\t\t(%s)\r\n", bufPoolErr);
			}
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, [%d]:%s",	
				__FILE__, __FUNCTION__, __LINE__, iErrIndex, dest);	
			return (rtn = E_COMMAND_ERROR_POOL);
		}
	}
	else
	{
	
		DEBUGMSG(1,("222 @%p, szPoolIdPri:%s, szSwiId:%s\r\n", gpSwitchDbInfo, (char *)gpSwitchDbInfo->DbSwi.szPoolIdPri, gpSwitchDbInfo->DbSwi.szSwiId));
		
		SwitchBaseQuery[E_QUERY_TYPE_UPDATE - 1].QueryPackFunc1(
															gpSwitchDbInfo, 
															&gCfgDataDL.SwitchBaseInfo, 
															&gCfgDataDL.SwitchBaseColName, 
															bufDest, 
															sizeof(bufDest));
		pthread_mutex_lock(&gSwitchBaseLock);
		iQueryRtn = MysqlFunc.dbDoQuery(&mySwitchBase, bufDest, E_QUERY_TYPE_UPDATE, NULL);
		pthread_mutex_unlock(&gSwitchBaseLock);
		//memset(bufDest, 0, sizeof(bufDest));
		DEBUGMSG(1,("bufDest:%s\r\n", bufDest));

		if (!iQueryRtn)
		{
			//把正确信息返回，否则把错误的pool_id直接返回
			if (rtnPackShell == 0)
				strcpy((char *)dest + iPos, szRetBuf[0]);
		}
		else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, Setdefaultpool update databases is error!\r\n",  
				__FILE__, __FUNCTION__, __LINE__);		
		}
	}

	return rtn;
}

//给指定的cell设置一个矿池
int PackSetPoolMonitorToComm(int argc, char **argv, void *dest, int destLen, void *pParData)
{
	int rtn = 0, rtnPackShell = 0;//,rtnRelogin = 0;
	int argcSecond = 0;
	char *argvSecond[BUFFER_LEN_20K] = {0};
	char delimSecond[] = ",";
	char destPackShell[BUFFER_LEN_256K] = {0};
	int destLenPackShell = 0, iArgLen = 0, iParaLen = 0, iReloLen = 0;
	char szParseDest[BUFFER_LEN_32K] = {0};
	int  i = 0, j = 0;
	//int iIndex = 0;
	
	//int argcShell = 0;
	//char *argvShell[BUFFER_LEN_64] = {0};
	//char delimShell[] = " ";
	int iPos = 0;

	if(argc != COMMANDLINE_2)
	{
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, ParseSetPoolCommand is error, argc:%d, argv[0]:%s!\r\n",  
			__FILE__, __FUNCTION__, __LINE__, argc, argv[0]);
	
		return E_COMMAND_ERROR_IDNUM;
	}
	for (i = 0; i < argc; i++)
	{
		DEBUGMSG(0,("argv[%d]:%s\n", i, argv[i]));
		if (Parse(argv[i], &argcSecond, argvSecond, delimSecond) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, Shell parse error!\r\n",	__FILE__, __FUNCTION__, __LINE__);
			return -1;
		}

		if(argcSecond < COMMANDLINE_2 && i == 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, Parse Second param error, argc:%d, argv[%d]:%s!\r\n",  
			__FILE__, __FUNCTION__, __LINE__, argc, i, argv[i]);

			return E_COMMAND_ERROR_IDNUM;
		}

		DEBUGMSG(1,("argcSecond:%d\n", argcSecond));
		//printf("argcSecond:%d\n", argcSecond);

		//pool_id,cell_id
		iArgLen = strlen(argvSecond[0]);
		iParaLen = strlen((char *)ParamMoreTypes[i].cmd);
		if (iArgLen == iParaLen && (strncmp(argvSecond[0], (char *)ParamMoreTypes[i].cmd, iArgLen) == 0))
		{
			//调试用
			//if (i == 1)
			//	continue;
			
			destLenPackShell = sizeof (destPackShell);
			//SetSpecifyPoolFindCellId
			//rtnPackShell = SetSpecifyPoolMonitor(
			rtnPackShell += ParamMoreTypes[i].SetCommFunc(
				argcSecond, argvSecond, destPackShell, destLenPackShell, dest, destLen);


			DEBUGMSG(1,("ParamMoreTypes[%d].cmd:%s\n", i, (char *)ParamMoreTypes[i].cmd));
			DEBUGMSG(1,("destLenPackShell:%d, destPackShell:%s\n", destLenPackShell, destPackShell));
			DEBUGMSG(1,("destLen:%d, dest:%s\n", destLen, (char *)dest));
			DEBUGMSG(1,("rtnPackShell:%d\n", rtnPackShell));
			if (rtnPackShell > 0)
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, [%d]:%s", 
					__FILE__, __FUNCTION__, __LINE__, rtnPackShell, dest);		
			
				return (rtn = E_COMMAND_ERROR_POOL);
			}
		}

		for (j = 0; j < argcSecond; j++)
		{
			memset(argvSecond+j, 0, sizeof(*argvSecond));
			argcSecond = 0;
		}
		//iIndex = i;
	}
	

	//command setpool
	if (rtnPackShell == 0)
	{
		if (Parse(destPackShell, &argcSecond, argvSecond, delimSecond) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, Shell parse error!\r\n",	__FILE__, __FUNCTION__, __LINE__);
			return -1;
		}
		
		if(argcSecond <= 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, Parse Second param error, argc:%d, destPackShell:%s!\r\n",	
			__FILE__, __FUNCTION__, __LINE__, argc, destPackShell);
		
			return E_COMMAND_ERROR_IDNUM;
		}
		
		SetSpecifyPool(argcSecond, argvSecond, szParseDest, sizeof(szParseDest), pParData);
		iReloLen = strlen(szParseDest);
		if (iReloLen != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, SetPool, iReloLen:%d, cell_id:[%s]!\r\n",	
				__FILE__, __FUNCTION__, __LINE__, iReloLen, szParseDest);
		
		}
		//成功
		strcpy((char *)dest + iPos, szRetBuf[0]);
	}

	return rtn;
}

int PackCellSearchToMulticast
	(void *pSrc, void *pLoginPool, void *dest, int destLen, int *pSendLen)
{
	int rtn = 0;
	char *pBuf = NULL;
    cJSON * pJsonRoot = NULL;
	cJSON * pJsonSub = NULL;

	DEBUGMSG(1,("%s...\n", __FUNCTION__));
	
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        return -1;
    }

    //cJSON_AddStringToObject(pJsonRoot, "id", "111");
	cJSON_AddNumberToObject(pJsonRoot, "id", *((stMonitorSrc*)pSrc)->pIdSeq);
	cJSON_AddStringToObject(pJsonRoot, "cellid", "0");
    cJSON_AddStringToObject(pJsonRoot, "method", "search");

	//cJSON_AddNullToObject(pJsonRoot, "param");


	pJsonSub = cJSON_CreateObject();
    if (pJsonSub == NULL )
    {
        return -1;
    }
	//inet_ntoa(((stApiNetConnPool *)pLoginPool)->listener.addr4.sin_addr)
	//cJSON_AddStringToObject(pJsonSub, "ip", "192.168.1.106");
	cJSON_AddStringToObject(pJsonSub, "ip", (char *)gCfgDataDL.SwitchInfo.szServerIp);
	//cJSON_AddNumberToObject(pJsonSub, "port",  ntohs(((stApiNetConnPool *)pLoginPool)->listener.addr4.sin_port));
	cJSON_AddNumberToObject(pJsonSub, "port",  gCfgDataDL.SwitchInfo.iServerPort);
	cJSON_AddItemToObject(pJsonRoot, "param", pJsonSub);

	pBuf = cJSON_Print(pJsonRoot);
    if(NULL == pBuf)
    {
        // create object faild, exit
        cJSON_Delete(pJsonRoot);
        return -1;
    }
	*pSendLen = strlen(pBuf);
	strncpy(dest, pBuf, destLen);
	DEBUGMSG(0,("pBuf:%s\n", pBuf));
	DEBUGMSG(1,("*pSendLen:%d\n", *pSendLen));
	DEBUGMSG(1,("err @:%p, dest:%s\n", dest, (char *)dest));

	//释放
	cJSON_Delete(pJsonRoot);
	free(pBuf);

	return rtn;
}

int PackSwitchvalidator(void *pSrc, void *dest, int destLen, int *pSendLen)
{
	int rtn = 0;
    cJSON *pJsonRoot = NULL;
	char *pBuf = NULL;
	//long lTimeStamp = 0;
	BYTE szTimeBuf[BUFFER_LEN_16] = {0};
	int i;
	long lTimestamp = 0;
	unsigned char encrypt[BUFFER_LEN_64]; 	             //d191bb9dc3a6de544dc9422f946e5d98
	unsigned char decrypt[BUFFER_LEN_16] = {0};    
	MD5_CTX md5;
	char bufMd5[BUFFER_LEN_64] = {0};
	int iPos = 0;
	
	DEBUGMSG(1,("%s..., @MoniSrc:%p, @pSrc:%p\r\n", __FUNCTION__, &MoniSrc, pSrc));

    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        return -1;
    }

	lTimestamp = GetTime();
	snprintf((char *)encrypt, sizeof(encrypt), "%ld%s", lTimestamp, (char *)gCfgDataDL.MonitorInfo.szToken);
	MD5Init(&md5);         		
	MD5Update(&md5,encrypt,strlen((char *)encrypt));
	MD5Final(&md5,decrypt);  
	
	DEBUGMSG(0,("before:%s\nafter:", encrypt));
	DEBUGMSG(0,("1 @decrypt:%p\n", decrypt));
	
	for(i=0;i<16;i++)
	{
		//DEBUGMSG(1,("%02x",decrypt[i]));
		snprintf(bufMd5+iPos, sizeof(bufMd5), "%02x", decrypt[i]);
		iPos += strlen(bufMd5+iPos);
	} 
	//DEBUGMSG(1,("\n"));

	//DEBUGMSG(1,("2 @decrypt:%p\n", decrypt));

	//strncpy(bufMd5, (char *)decrypt, sizeof(bufMd5));
	//DEBUGMSG(1,("bufMd5:%s\n", bufMd5));
	//DEBUGMSG(1,("222 cmd:%p\n", ((stMonitorSrc*)pSrc)->pMonitorComm[0].cmd));
    cJSON_AddStringToObject(pJsonRoot, "method", (char *)((stMonitorSrc*)pSrc)->pMonitorComm[0].cmd);
	//DEBUGMSG(1,("pIdSeq:%d\n", *((stMonitorSrc*)pSrc)->pIdSeq));
    cJSON_AddNumberToObject(pJsonRoot, "id", *((stMonitorSrc*)pSrc)->pIdSeq);
	//cJSON_AddStringToObject(pJsonRoot, "sign", "837106328fb8326da9663ed01b129272");

	//DEBUGMSG(1,("3 @decrypt:%p\n", decrypt));
	cJSON_AddStringToObject(pJsonRoot, "sign", bufMd5);
	//DEBUGMSG(1,("4 @bufMd5:%p, bufMd5:%s\n", bufMd5, bufMd5));
	
	//DEBUGMSG(1,("iSwiId:%d\n", ((stMonitorSrc*)pSrc)->pSwiDbInfo->DbSwi.iSwiId));
	cJSON_AddStringToObject(pJsonRoot, "switchid", (char *)((stMonitorSrc*)pSrc)->pSwiDbInfo->DbSwi.szSwiId);
	//cJSON_AddNumberToObject(pJsonRoot, "switchid", 1);

	//lTimestamp = 1428494203;
	snprintf((char *)szTimeBuf, sizeof(szTimeBuf), "%ld", lTimestamp);
	cJSON_AddStringToObject(pJsonRoot, "timestamp", (char *)szTimeBuf);
	
    cJSON * pSubJson = NULL;
    pSubJson = cJSON_CreateObject();
    if(NULL == pSubJson)
    {
        // create object faild, exit
        cJSON_Delete(pJsonRoot);
        return -1;
    }
	
    pBuf = cJSON_Print(pJsonRoot);

	DEBUGMSG(0,("pBuf:%s\n", pBuf));

    if(NULL == pBuf)
    {
        //convert json list to string faild, exit
        //because sub json pSubJson han been add to pJsonRoot, so just delete pJsonRoot, if you also delete pSubJson, it will coredump, and error is : double free
        cJSON_Delete(pJsonRoot);
        return -1;
    }
	//*pSendLen = strlen(pBuf);
	snprintf((char *)((stClientSendData*)dest)->bySeCmdBuf, destLen, "%s\r\n", pBuf);
	//strncpy((char *)dest, pBuf, destLen);

	*pSendLen = strlen((char *)((stClientSendData*)dest)->bySeCmdBuf);

	//释放
    cJSON_Delete(pJsonRoot);
	free(pBuf);

	DEBUGMSG(1,("*pSendLen:%d, destLen:%d\ndest:%s\n", *pSendLen, destLen, (char *)dest));

	/*for (i = 0; i < strlen((char *)dest);i++)
		DEBUGMSG(1,("%02x", *((char *)dest+i)));
	DEBUGMSG(1,("\n"));*/

    return rtn;
}

int NoPackMonitor(int argc, char **argv, void *dest, int destLen, void *pParData)
{
	int rtn = 0;

	return rtn;
}

//打成1包
int PackCellNewInfo(void *pSrc, void *dest, int destLen)
//(void *pSrc, void *dest, int destLen, stCellConn *val)
{
	int rtn = -1;
	structUthash *pList = NULL;
	int iTotListLen = 0;
	devlist_st *pNode = NULL, *tmp = NULL;
	char *pBuf = NULL;
	int CellOnline = 0;

	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));
	//printf("%s...\r\n", __FUNCTION__);

	pthread_mutex_lock(&gLoginMonitorLock);
	
	pList = ((stMonitorSrc*)pSrc)->pList;
	pthread_mutex_lock(pList->ListLenLock);
	iTotListLen = pList->CountUsersStr(*pList->StrHUsers); 
	pthread_mutex_unlock(pList->ListLenLock);

#ifdef _DEBUG_MANY_DEV_
	printf("gNewCellSendLen:%d, iTotListLen:%d\n", gNewCellSendLen, iTotListLen);
#endif
	if (iTotListLen == 0 || gNewCellSendLen >= iTotListLen)
	{
		gNewCellSendLen = iTotListLen;
		pthread_mutex_unlock(&gLoginMonitorLock);
		return -1;
	}

	cJSON* pItem = cJSON_CreateObject();
	if (pItem == NULL)
	{
		pthread_mutex_unlock(&gLoginMonitorLock);
		return -1;
	}	

	cJSON* pArray = cJSON_CreateArray();	
	if (pArray == NULL)
	{
		pthread_mutex_unlock(&gLoginMonitorLock);
		return -1;
	}	

	//DEBUGMSG(0,("iTotListLen:%d\n", iTotListLen));
	pthread_mutex_lock(pList->MonitorRemLock);
	//pthread_mutex_lock(pList->LoginRemLock);
	int i = 0;
	HASH_ITER(hh, *pList->StrHUsers, pNode, tmp)
	{		
		//((stCellConn *)pNode->val)
		if(((stCellConn *)pNode->val)->eCellDataSer == E_DEV_DATAUPDATE_NEWCELL)
		{
			if (i++ >= MONITOR_CELL_LIST_PACK_NUM)
			{
				printf("i:%d, szCellId:%s\t", i, (char *)((stCellConn *)tmp->val)->CellData.szCellId);
				((stClientSendData*)dest)->pNextNode = pNode;
				break;
			}
			else
			{
				gNewCellSendLen++;
			}


			pItem = cJSON_CreateObject();
			cJSON_AddStringToObject(pItem, "cell_id", (char *)((stCellConn *)pNode->val)->CellData.szCellId);

			//1:normal,-1:offline,-2:fault	  -2暂时不处理
			if (((stCellConn *)pNode->val)->CellData.bCellOnline == 0)
				CellOnline = -1;
			else
				CellOnline = 1;
			cJSON_AddNumberToObject(pItem, "cell_status", CellOnline);

			cJSON_AddNumberToObject(pItem, "reg_time", ((stCellConn *)pNode->val)->CellData.iRegtime);
			cJSON_AddStringToObject(pItem, "cell_ip", inet_ntoa(((stCellConn *)pNode->val)->remote.addr4.sin_addr));
			//暂时这么处理
			cJSON_AddStringToObject(pItem, "cell_type", "sfd01");
			cJSON_AddNumberToObject(pItem, "entry_temp", ((stCellConn *)pNode->val)->CellStat.iEntryTemp);
			cJSON_AddNumberToObject(pItem, "exit_temp", ((stCellConn *)pNode->val)->CellStat.iExitTemp);
			
			cJSON_AddNumberToObject(pItem, "btc_rated_khash", ((stCellConn *)pNode->val)->CellData.lBTCRatedKHash);
			//btc_real_khash in 30 minutes
			cJSON_AddNumberToObject(pItem, "btc_real_khash", ((stCellConn *)pNode->val)->CellStat.llBTCRealKHash);
			cJSON_AddNumberToObject(pItem, "btc_reject", ((stCellConn *)pNode->val)->CellStat.ullBTCReject);
			cJSON_AddNumberToObject(pItem, "btc_accept", ((stCellConn *)pNode->val)->CellStat.ullBTCAccept);
			
			cJSON_AddNumberToObject(pItem, "ltc_rated_khash", ((stCellConn *)pNode->val)->CellData.lLTCRatedKHash);
			//ltc_real_khash in 30 minutes
			cJSON_AddNumberToObject(pItem, "ltc_real_khash", ((stCellConn *)pNode->val)->CellStat.llLTCRealKHash);
			cJSON_AddNumberToObject(pItem, "ltc_reject", ((stCellConn *)pNode->val)->CellStat.ullLTCReject);
			cJSON_AddNumberToObject(pItem, "ltc_accept", ((stCellConn *)pNode->val)->CellStat.ullLTCAccept);

			cJSON_AddItemToArray(pArray, pItem);

			//复位
			((stCellConn *)pNode->val)->eCellDataSer = 0;

			if (rtn == -1)
				rtn = 0;
		}
	}
	//pthread_mutex_unlock(pList->LoginRemLock);
	pthread_mutex_unlock(pList->MonitorRemLock); 
	pthread_mutex_unlock(&gLoginMonitorLock);
	


#ifdef _DEBUG_MANY_DEV_
		if (pNode != NULL)
		{
			printf("@tmp:%p, @StrHUsers:%p\t", pNode, *pList->StrHUsers);
			printf("11111 szCellId:%s\n", (char *)((stCellConn *)pNode->val)->CellData.szCellId);
		}
#endif

	if (rtn == 0)
	{
		cJSON* pJsonRoot = cJSON_CreateObject();
		if (pJsonRoot == NULL )
		{
			return -1;
		}
		
		cJSON_AddStringToObject(pJsonRoot, "method", (char *)((stMonitorSrc*)pSrc)->pMonitorComm[1].cmd);
		cJSON_AddNumberToObject(pJsonRoot, "id", *((stMonitorSrc*)pSrc)->pIdSeq);
		cJSON_AddStringToObject(pJsonRoot, "switchid", (char *)((stMonitorSrc*)pSrc)->pSwiDbInfo->DbSwi.szSwiId);
		cJSON_AddItemToObject(pJsonRoot, "param", pArray);		
		
		pBuf = cJSON_Print(pJsonRoot);
		if(NULL == pBuf)
		{
			// create object faild, exit
			cJSON_Delete(pJsonRoot);
			return -1;
		}
		destLen = strlen(pBuf);
		strncpy(dest, pBuf, destLen);
		
		if (destLen > 0)
			DEBUGMSG(1,("destLen:%d\ndest:%s\n", destLen, (char *)dest));
		
		//释放
		cJSON_Delete(pJsonRoot);
		free(pBuf);
	}
	else
	{
		if (pItem != NULL)
			cJSON_Delete(pItem);
		if (pArray != NULL)
			cJSON_Delete(pArray);
	}

	return rtn;
}

//每包20k，
/*int PackCellNewInfo(void *pSrc, void *dest, int destLen, int *pSendLen)
{
	int rtn = -1, i = 0, iListLen = 0;//, iErrIndex = 0, j = 0;//iTotListLen = 0, 
	structUthash *pList = NULL;
	//int iTotListLen = 0;
	devlist_st *pNode = NULL, *tmp = NULL;
	char *pBuf = NULL;
	int CellOnline = 0;
	int iNumPack = 0, iRemain = 0, iLoop = 0;//, iIndex = 0;
	BOOL bDataIn = FALSE;

	//stClientSendData *pCliSendData = NULL;
	//pCliSendData = &gCliSendDaTa;

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

    //cJSON* pJsonRoot = NULL, pArray;
    //pJsonRoot = cJSON_CreateObject();
	//pArray = cJSON_CreateArray();

	pList = ((stMonitorSrc*)pSrc)->pList;

	//printf("\nbSubcontFlag:%d\t", ((stClientSendData*)dest)->bSubcontFlag);
	if (!((stClientSendData*)dest)->bSubcontFlag)
	{
		//iListLen = ((structUthash *)pList)->SelCrrNode->SelLen;
		pthread_mutex_lock(pList->ListLenLock);
		iListLen = pList->CountUsersStr(*pList->StrHUsers); 
		pthread_mutex_unlock(pList->ListLenLock);

		if (iListLen == 0)
			return -1;
		
		//DEBUGMSG(0,("iTotListLen:%d\n", iTotListLen));		
		iRemain = iListLen % MONITOR_CELL_LIST_PACK_NUM;
		iNumPack = iRemain > 0 ? iListLen / MONITOR_CELL_LIST_PACK_NUM+1 : iListLen / MONITOR_CELL_LIST_PACK_NUM;
		
		if (iNumPack > 1)	//分包
		{	
			((stClientSendData*)dest)->bSubcontFlag = TRUE;
			((stClientSendData*)dest)->iNumPack = iNumPack;
			((stClientSendData*)dest)->iRemain = iRemain;
		}

		DEBUGMSG(1,("iRemain:%d, iSubindex:%d\t",((stClientSendData*)dest)->iRemain, ((stClientSendData*)dest)->iSubindex));

#ifdef _DEBUG_MANY_DEV_
	printf("iRemain:%d, iNumPack:%d\t",((stClientSendData*)dest)->iRemain, ((stClientSendData*)dest)->iNumPack);
#endif
	}


	//循环打包
	printf("iSubindex:%d, iNumPack:%d\t", ((stClientSendData*)dest)->iSubindex, ((stClientSendData*)dest)->iNumPack);
	printf("@pNextNode:%p\n", ((stClientSendData*)dest)->pNextNode);
	if (((stClientSendData*)dest)->iSubindex <= ((stClientSendData*)dest)->iNumPack - 1)
	{
		if (((stClientSendData*)dest)->iSubindex == (((stClientSendData*)dest)->iNumPack - 1) && ((stClientSendData*)dest)->iRemain)
		{
			iLoop = ((stClientSendData*)dest)->iRemain;
			tmp = ((stClientSendData*)dest)->pNextNode;
		}
		else
		{
			iLoop = MONITOR_CELL_LIST_PACK_NUM;
			if (((stClientSendData*)dest)->pNextNode != NULL)
				tmp = ((stClientSendData*)dest)->pNextNode;
			else
				tmp = *pList->StrHUsers;
		}
	}
	else
	{
		iLoop = iListLen;
		tmp = *pList->StrHUsers;
	}

	//DEBUGMSG(0,("bSubcontFlag:%d, iLoop:%d, @tmp:%p\n", 
	//	((stClientSendData*)dest)->bSubcontFlag, iLoop, tmp));
	//DEBUGMSG(0,("11111 szCellId:%s\n", (char *)((stCellConn *)tmp->val)->CellData.szCellId));
#ifdef _DEBUG_MANY_DEV_
	if (tmp != NULL)
	{
		printf("iLoop:%d, @tmp:%p, @StrHUsers:%p\t", iLoop, tmp, *pList->StrHUsers);
		printf("11111 szCellId:%s\n", (char *)((stCellConn *)tmp->val)->CellData.szCellId);
	}
#endif

	cJSON* pArray = cJSON_CreateArray();	
    if (pArray == NULL)
    {
        return -1;
    }	
	cJSON* pItem = cJSON_CreateObject();
    if (pItem == NULL)
    {
        return -1;
    }

	pthread_mutex_lock(pList->MonitorRemLock);
	for(pNode = tmp; pNode != NULL; pNode = (devlist_st*)(pNode->hh.next))
	{
		if (i++ >= iLoop)// || i >= E_DEV_DATAUPDATE_NEWCELL)
		{
			printf("i:%d, szCellId:%s\t", i, (char *)((stCellConn *)tmp->val)->CellData.szCellId);
			((stClientSendData*)dest)->pNextNode = pNode;
			break;
		}

		if(((stCellConn *)pNode->val)->eCellDataSer == E_DEV_DATAUPDATE_NEWCELL)
		{
			pItem = cJSON_CreateObject();

			cJSON_AddStringToObject(pItem, "cell_id", (char *)((stCellConn *)pNode->val)->CellData.szCellId);

			//1:normal,-1:offline,-2:fault	  -2暂时不处理
			if (((stCellConn *)pNode->val)->CellData.bCellOnline == 0)
				CellOnline = -1;
			else
				CellOnline = 1;
			cJSON_AddNumberToObject(pItem, "cell_status", CellOnline);

			cJSON_AddNumberToObject(pItem, "reg_time", ((stCellConn *)pNode->val)->CellData.iRegtime);
			cJSON_AddStringToObject(pItem, "cell_ip", inet_ntoa(((stCellConn *)pNode->val)->remote.addr4.sin_addr));
			//暂时这么处理
			cJSON_AddStringToObject(pItem, "cell_type", "sfd01");
			cJSON_AddNumberToObject(pItem, "entry_temp", ((stCellConn *)pNode->val)->CellStat.iEntryTemp);
			cJSON_AddNumberToObject(pItem, "exit_temp", ((stCellConn *)pNode->val)->CellStat.iExitTemp);
			
			cJSON_AddNumberToObject(pItem, "btc_rated_khash", ((stCellConn *)pNode->val)->CellData.lBTCRatedKHash);
			//btc_real_khash in 30 minutes
			cJSON_AddNumberToObject(pItem, "btc_real_khash", ((stCellConn *)pNode->val)->CellStat.llBTCRealKHash);
			cJSON_AddNumberToObject(pItem, "btc_reject", ((stCellConn *)pNode->val)->CellStat.ullBTCReject);
			cJSON_AddNumberToObject(pItem, "btc_accept", ((stCellConn *)pNode->val)->CellStat.ullBTCAccept);
			
			cJSON_AddNumberToObject(pItem, "ltc_rated_khash", ((stCellConn *)pNode->val)->CellData.lLTCRatedKHash);
			//ltc_real_khash in 30 minutes
			cJSON_AddNumberToObject(pItem, "ltc_real_khash", ((stCellConn *)pNode->val)->CellStat.llLTCRealKHash);
			cJSON_AddNumberToObject(pItem, "ltc_reject", ((stCellConn *)pNode->val)->CellStat.ullLTCReject);
			cJSON_AddNumberToObject(pItem, "ltc_accept", ((stCellConn *)pNode->val)->CellStat.ullLTCAccept);

			cJSON_AddItemToArray(pArray, pItem);

			//复位
			((stCellConn *)pNode->val)->eCellDataSer = 0;

			if (rtn == -1)
				rtn = 0;
		}

		
	}
	pthread_mutex_unlock(pList->MonitorRemLock); 

	
#ifdef _DEBUG_MANY_DEV_
	printf("bDataIn:%d\t", bDataIn);
#endif
	if (rtn == 0)
	{
		cJSON* pJsonRoot = cJSON_CreateObject();
		if (pJsonRoot == NULL )
		{
			return -1;
		}
		
		cJSON_AddStringToObject(pJsonRoot, "method", (char *)((stMonitorSrc*)pSrc)->pMonitorComm[1].cmd);
		cJSON_AddNumberToObject(pJsonRoot, "id", *((stMonitorSrc*)pSrc)->pIdSeq);
		cJSON_AddStringToObject(pJsonRoot, "switchid", (char *)((stMonitorSrc*)pSrc)->pSwiDbInfo->DbSwi.szSwiId);
		cJSON_AddItemToObject(pJsonRoot, "param", pArray);		
	
	
		pBuf = cJSON_Print(pJsonRoot);
		if(NULL == pBuf)
		{
			// create object faild, exit
			cJSON_Delete(pJsonRoot);
			return -1;
		}
		*pSendLen = strlen(pBuf);
		//strncpy(dest, pBuf, destLen);
		//strncpy((char *)((stClientSendData*)dest)->bySeCmdBuf, pBuf, destLen);
		snprintf((char *)((stClientSendData*)dest)->bySeCmdBuf, destLen, "%s", pBuf);
	
#ifdef _DEBUG_MANY_DEV_
		printf("destLen:%d, *pSendLen:%d\n", destLen, *pSendLen);
#endif
		DEBUGMSG(1,("destLen:%d, *pSendLen:%d\n", destLen, *pSendLen));
		DEBUGMSG(0,("*************Pack All Cell dest:%s\n", (char *)dest));
		
		//释放
		free(pBuf);
		cJSON_Delete(pJsonRoot);
	}
	else
	{
		if (pItem != NULL)
			cJSON_Delete(pItem);
		if (pArray != NULL)
			cJSON_Delete(pArray);		
		*pSendLen = 0;
	}

    return rtn;
}*/


//每包20k，
int PackCellAllInfo(void *pSrc, void *dest, int destLen, int *pSendLen)
{
	int rtn = -1, i = 0, iListLen = 0;//, iErrIndex = 0, j = 0;//iTotListLen = 0, 
	structUthash *pList = NULL;
	//int iTotListLen = 0;
	devlist_st *pNode = NULL, *tmp = NULL;
	char *pBuf = NULL;
	int CellOnline = 0;
	int iNumPack = 0, iRemain = 0, iLoop = 0;//, iIndex = 0;
	//BOOL bDataIn = FALSE;

	//stClientSendData *pCliSendData = NULL;
	//pCliSendData = &gCliSendDaTa;

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));


    //cJSON* pJsonRoot = NULL, pArray;
    //pJsonRoot = cJSON_CreateObject();
	//pArray = cJSON_CreateArray();
		
	pList = ((stMonitorSrc*)pSrc)->pList;

#ifdef _DEBUG_MANY_DEV_	
	printf("\nbSubcontFlag:%d\t", ((stClientSendData*)dest)->bSubcontFlag);
#endif
	if (!((stClientSendData*)dest)->bSubcontFlag)
	{
		//iListLen = ((structUthash *)pList)->SelCrrNode->SelLen;
		pthread_mutex_lock(pList->ListLenLock);
		iListLen = pList->CountUsersStr(*pList->StrHUsers); 
		pthread_mutex_unlock(pList->ListLenLock);

		if (iListLen == 0)
			return -1;

		//DEBUGMSG(0,("iTotListLen:%d\n", iTotListLen));		
		iRemain = iListLen % MONITOR_CELL_LIST_PACK_NUM;
		iNumPack = iRemain > 0 ? iListLen / MONITOR_CELL_LIST_PACK_NUM+1 : iListLen / MONITOR_CELL_LIST_PACK_NUM;
		
		if (iNumPack > 1)	//分包
		{	
			((stClientSendData*)dest)->bSubcontFlag = TRUE;
			((stClientSendData*)dest)->iNumPack = iNumPack;
			((stClientSendData*)dest)->iRemain = iRemain;
		}

		DEBUGMSG(1,("iRemain:%d, iSubindex:%d\t",((stClientSendData*)dest)->iRemain, ((stClientSendData*)dest)->iSubindex));

#ifdef _DEBUG_MANY_DEV_
	printf("iRemain:%d, iNumPack:%d\t",((stClientSendData*)dest)->iRemain, ((stClientSendData*)dest)->iNumPack);
#endif
	}


	//循环打包
	printf("iSubindex:%d, iNumPack:%d\t", ((stClientSendData*)dest)->iSubindex, ((stClientSendData*)dest)->iNumPack);
	printf("@pNextNode:%p\n", ((stClientSendData*)dest)->pNextNode);
	if (((stClientSendData*)dest)->iSubindex <= ((stClientSendData*)dest)->iNumPack - 1)
	{
		if (((stClientSendData*)dest)->iSubindex == (((stClientSendData*)dest)->iNumPack - 1) && ((stClientSendData*)dest)->iRemain)
		{
			iLoop = ((stClientSendData*)dest)->iRemain;
			tmp = ((stClientSendData*)dest)->pNextNode;
		}
		else
		{
			iLoop = MONITOR_CELL_LIST_PACK_NUM;
			if (((stClientSendData*)dest)->pNextNode != NULL)
				tmp = ((stClientSendData*)dest)->pNextNode;
			else
				tmp = *pList->StrHUsers;
		}
	}
	else
	{
		iLoop = iListLen;
		tmp = *pList->StrHUsers;
	}

	//DEBUGMSG(0,("bSubcontFlag:%d, iLoop:%d, @tmp:%p\n", 
	//	((stClientSendData*)dest)->bSubcontFlag, iLoop, tmp));
	//DEBUGMSG(0,("11111 szCellId:%s\n", (char *)((stCellConn *)tmp->val)->CellData.szCellId));
#ifdef _DEBUG_MANY_DEV_
	printf("iLoop:%d, @tmp:%p, @StrHUsers:%p\t", iLoop, tmp, *pList->StrHUsers);
	printf("11111 szCellId:%s\n", (char *)((stCellConn *)tmp->val)->CellData.szCellId);
#endif

	cJSON* pArray = cJSON_CreateArray();	
    if (pArray == NULL)
    {
        return -1;
    }	
	
	cJSON* pItem = cJSON_CreateObject();
    if (pItem == NULL)
    {
        return -1;
    }

	pthread_mutex_lock(pList->MonitorRemLock);
	for(pNode = tmp; pNode != NULL; pNode = (devlist_st*)(pNode->hh.next))
	{
		if (i++ >= iLoop)// || i >= MONITOR_CELL_LIST_PACK_NUM)
		{
			printf("i:%d, szCellId:%s\t", i, (char *)((stCellConn *)tmp->val)->CellData.szCellId);
			((stClientSendData*)dest)->pNextNode = pNode;
			break;
		}

		if(((stCellConn *)pNode->val)->eCellDataSer == E_DEV_DATAUPDATE_CELLNEWDATA)
		{
			//if (!bDataIn)
			//	bDataIn = TRUE;
			pItem = cJSON_CreateObject();
//#ifdef _DEBUG_MANY_DEV_			
			//printf("222222 szCellId:%s\n", (char *)((stCellConn *)pNode->val)->CellData.szCellId);
//#endif
			cJSON_AddStringToObject(pItem, "cell_id", (char *)((stCellConn *)pNode->val)->CellData.szCellId);

			//1:normal,-1:offline,-2:fault	  -2暂时不处理
			if (((stCellConn *)pNode->val)->CellData.bCellOnline == 0)
				CellOnline = -1;
			else
				CellOnline = 1;
			cJSON_AddNumberToObject(pItem, "cell_status", CellOnline);

			cJSON_AddNumberToObject(pItem, "reg_time", ((stCellConn *)pNode->val)->CellData.iRegtime);
			cJSON_AddStringToObject(pItem, "cell_ip", inet_ntoa(((stCellConn *)pNode->val)->remote.addr4.sin_addr));
			//暂时这么处理
			cJSON_AddStringToObject(pItem, "cell_type", "sfd01");
			cJSON_AddNumberToObject(pItem, "entry_temp", ((stCellConn *)pNode->val)->CellStat.iEntryTemp);
			cJSON_AddNumberToObject(pItem, "exit_temp", ((stCellConn *)pNode->val)->CellStat.iExitTemp);
			
			cJSON_AddNumberToObject(pItem, "btc_rated_khash", ((stCellConn *)pNode->val)->CellData.lBTCRatedKHash);
			//btc_real_khash in 30 minutes
			cJSON_AddNumberToObject(pItem, "btc_real_khash", ((stCellConn *)pNode->val)->CellStat.llBTCRealKHash);
			cJSON_AddNumberToObject(pItem, "btc_reject", ((stCellConn *)pNode->val)->CellStat.ullBTCReject);
			cJSON_AddNumberToObject(pItem, "btc_accept", ((stCellConn *)pNode->val)->CellStat.ullBTCAccept);
			
			cJSON_AddNumberToObject(pItem, "ltc_rated_khash", ((stCellConn *)pNode->val)->CellData.lLTCRatedKHash);
			//ltc_real_khash in 30 minutes
			cJSON_AddNumberToObject(pItem, "ltc_real_khash", ((stCellConn *)pNode->val)->CellStat.llLTCRealKHash);
			cJSON_AddNumberToObject(pItem, "ltc_reject", ((stCellConn *)pNode->val)->CellStat.ullLTCReject);
			cJSON_AddNumberToObject(pItem, "ltc_accept", ((stCellConn *)pNode->val)->CellStat.ullLTCAccept);

			cJSON_AddItemToArray(pArray, pItem);

			//复位
			((stCellConn *)pNode->val)->eCellDataSer = 0;

			if (rtn == -1)
				rtn = 0;
		}
	}
	pthread_mutex_unlock(pList->MonitorRemLock); 

	
/*	pthread_mutex_lock(pList->MonitorRemLock);
	HASH_ITER(hh, *pList->StrHUsers, pNode, tmp)
	{
		if(((stCellConn *)pNode->val)->eCellDataSer == E_DEV_DATAUPDATE_CELLNEWDATA)
		{
		
			pItem = cJSON_CreateObject();
#ifdef _DEBUG_MANY_DEV_			
			printf("11111 szCellId:%s\n", (char *)((stCellConn *)pNode->val)->CellData.szCellId);
#endif
			cJSON_AddStringToObject(pItem, "cell_id", (char *)((stCellConn *)pNode->val)->CellData.szCellId);

			//1:normal,-1:offline,-2:fault    -2暂时不处理
			if (((stCellConn *)pNode->val)->CellData.bCellOnline == 0)
				CellOnline = -1;
			else
				CellOnline = 1;
			cJSON_AddNumberToObject(pItem, "cell_status", CellOnline);

			cJSON_AddNumberToObject(pItem, "reg_time", ((stCellConn *)pNode->val)->CellData.iRegtime);
			cJSON_AddStringToObject(pItem, "cell_ip", inet_ntoa(((stCellConn *)pNode->val)->remote.addr4.sin_addr));
			//暂时这么处理
			cJSON_AddStringToObject(pItem, "cell_type", "sfd01");
			cJSON_AddNumberToObject(pItem, "entry_temp", ((stCellConn *)pNode->val)->CellStat.iEntryTemp);
			cJSON_AddNumberToObject(pItem, "exit_temp", ((stCellConn *)pNode->val)->CellStat.iExitTemp);
			
			cJSON_AddNumberToObject(pItem, "btc_rated_khash", ((stCellConn *)pNode->val)->CellData.lBTCRatedKHash);
			//btc_real_khash in 30 minutes
			cJSON_AddNumberToObject(pItem, "btc_real_khash", ((stCellConn *)pNode->val)->CellStat.llBTCRealKHash);
			cJSON_AddNumberToObject(pItem, "btc_reject", ((stCellConn *)pNode->val)->CellStat.ullBTCReject);
			cJSON_AddNumberToObject(pItem, "btc_accept", ((stCellConn *)pNode->val)->CellStat.ullBTCAccept);
			
			cJSON_AddNumberToObject(pItem, "ltc_rated_khash", ((stCellConn *)pNode->val)->CellData.lLTCRatedKHash);
			//ltc_real_khash in 30 minutes
			cJSON_AddNumberToObject(pItem, "ltc_real_khash", ((stCellConn *)pNode->val)->CellStat.llLTCRealKHash);
			cJSON_AddNumberToObject(pItem, "ltc_reject", ((stCellConn *)pNode->val)->CellStat.ullLTCReject);
			cJSON_AddNumberToObject(pItem, "ltc_accept", ((stCellConn *)pNode->val)->CellStat.ullLTCAccept);

			cJSON_AddItemToArray(pArray, pItem);

			//复位
			((stCellConn *)pNode->val)->eCellDataSer = 0;
		}
	}
	pthread_mutex_unlock(pList->MonitorRemLock); */
/*#ifdef _DEBUG_MANY_DEV_
	printf("bDataIn:%d\t", bDataIn);
#endif*/
	if (rtn == 0)
	{
		cJSON* pJsonRoot = cJSON_CreateObject();
		if (pJsonRoot == NULL )
		{
			return -1;
		}
		
		cJSON_AddStringToObject(pJsonRoot, "method", (char *)((stMonitorSrc*)pSrc)->pMonitorComm[1].cmd);
		cJSON_AddNumberToObject(pJsonRoot, "id", *((stMonitorSrc*)pSrc)->pIdSeq);
		cJSON_AddStringToObject(pJsonRoot, "switchid", (char *)((stMonitorSrc*)pSrc)->pSwiDbInfo->DbSwi.szSwiId);
		cJSON_AddItemToObject(pJsonRoot, "param", pArray);	

	
		pBuf = cJSON_Print(pJsonRoot);
		if(NULL == pBuf)
		{
			// create object faild, exit
			cJSON_Delete(pJsonRoot);
			return -1;
		}
		*pSendLen = strlen(pBuf);
		//strncpy(dest, pBuf, destLen);
		//strncpy((char *)((stClientSendData*)dest)->bySeCmdBuf, pBuf, destLen);
		snprintf((char *)((stClientSendData*)dest)->bySeCmdBuf, destLen, "%s", pBuf);
	
#ifdef _DEBUG_MANY_DEV_
		printf("destLen:%d, *pSendLen:%d\n", destLen, *pSendLen);
#endif
		DEBUGMSG(1,("destLen:%d, *pSendLen:%d\n", destLen, *pSendLen));
		DEBUGMSG(0,("*************Pack All Cell dest:%s\n", (char *)dest));
		
		//释放
		free(pBuf);
		cJSON_Delete(pJsonRoot);
	}
	else
	{
		if (pItem != NULL)
			cJSON_Delete(pItem);
		if (pArray != NULL)
			cJSON_Delete(pArray);
		*pSendLen = 0;
	}

    return rtn;
}

/*int PackReplayErrorOffline(void *pJsonRoot, int PackArgc, char **PackArgv)
{
	int rtn = 0;
	cJSON *pJsonError = NULL;
	int i = 0, j = 0;
	int iId = 0;
	char delim[] = " ";
	int ArgLen = 0, bufLen = 0;

	pJsonError = cJSON_CreateObject();
	if(NULL == pJsonError)
	{
		//cJSON_Delete((cJSON *)pJsonRoot);
		return -1;
	}
	cJSON* pArray = cJSON_CreateArray();	
	if (pArray == NULL)
	{
		//cJSON_Delete(pJsonRoot);
		return -1;
	}
	//offline
	for (j = 1; j < PackArgc; j++)
	{
		iId = strtoul(PackArgv[j], 0, 0);
		cJSON_AddNumberToObject(pArray, "", iId);
	}
	cJSON_AddItemToObject(pJsonError, "offline", pArray);
	

	return rtn;
}
*/

int PackReplay(void *pSrc, void *dest, int destLen, int iIndex)
{
	int rtn = 0;
	char *pBuf = NULL;
    cJSON * pJsonRoot = NULL, *pJsonError = NULL;
	char delim[] = " ";
	int PackArgc = 0;
	char *PackArgv[BUFFER_LEN_2048] = {0};
	int ArgLen = 0, bufLen = 0;
	int i = 0, j = 0, iRetIndex = 0;
	//int iId = 0;
	BOOL bParseDest = FALSE, bIdbufRtn = FALSE;
	//E_SET_MONITOR eSetMass = E_SET_MONITOR_ALL_SUCC;
	//devlist_ist *pNode = NULL;//*tmp = NULL;
	//structUthash *pList = NULL;
	//int iTotListLen = 0, ioffLineCount = 0;
	//int iId = 0;

	DEBUGMSG(1,("%s...\n", __FUNCTION__));
	
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        return -1;
    }

    cJSON_AddNumberToObject(pJsonRoot, "id", *((stMonitorSrc*)pSrc)->pIdSeq);
	cJSON_AddStringToObject(pJsonRoot, "switchid", (char *)((stMonitorSrc*)pSrc)->pSwiDbInfo->DbSwi.szSwiId);
    cJSON_AddStringToObject(pJsonRoot, "method", (char *)((stMonitorSrc*)pSrc)->pMonitorComm[iIndex].cmd);

	DEBUGMSG(1,("3 szIdbufRtn:%s\n", (char *)CommMonitorFlag.szIdbufRtn));	
	DEBUGMSG(1,("3 iBufPos:%d\n", CommMonitorFlag.iBufPos));
	//notexist
	if (CommMonitorFlag.iBufPos > 0)
	{
		DEBUGMSG(0,("szIdbufRtn:%s\n", CommMonitorFlag.szIdbufRtn));
		DEBUGMSG(0,("sizeof(*PackArgv):%d\n", sizeof(*PackArgv)));
		
		/*for (i = 0; i < PackArgc; i++)
			memset(PackArgv+i, 0, sizeof(*PackArgv));
		PackArgc = 0;*/
		if (Parse((char *)CommMonitorFlag.szIdbufRtn, &PackArgc, PackArgv, delim) != 0)
		{
			cJSON_Delete(pJsonRoot);
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
			return (rtn = 2);
		}	
		
		/*if (PackArgc < 1)
		{
			cJSON_Delete(pJsonRoot);
			return (rtn = 3);
		}*/

		if (PackArgc > 0)
		{
			DEBUGMSG(1,("PackArgv[0]:%s\n", PackArgv[0]));
			ArgLen = strlen(PackArgv[0]);
			bufLen = strlen(szRetBuf[4]); 
			if (ArgLen == bufLen && strncasecmp(PackArgv[0], szRetBuf[4], ArgLen) == 0)
			{
				pJsonError = cJSON_CreateObject();
				if(NULL == pJsonError)
				{
					cJSON_Delete((cJSON *)pJsonRoot);
					return -1;
				}			
				/*if (NULL == pJsonError)
				{
					pJsonError = cJSON_CreateObject();
					if(NULL == pJsonError)
					{
						cJSON_Delete((cJSON *)pJsonRoot);
						return -1;
					}
				}*/

				cJSON* pArrayNotExist = cJSON_CreateArray();
				if (pArrayNotExist == NULL)
				{
					cJSON_Delete(pJsonRoot);
					return -1;
				}
				for (j = 1; j < PackArgc; j++)
				{
					//iId = strtoul(PackArgv[j], 0, 0);
					cJSON_AddStringToObject(pArrayNotExist, "", PackArgv[j]);
				}
				cJSON_AddItemToObject(pJsonError, szRetBuf[4], pArrayNotExist);
				bIdbufRtn = TRUE;
			}
		}
		cJSON_AddFalseToObject(pJsonRoot, "result");
	}
	else
	{
		if (NULL == pJsonError)
		{
			pJsonError = cJSON_CreateObject();
			if(NULL == pJsonError)
			{
				cJSON_Delete((cJSON *)pJsonRoot);
				return -1;
			}
		}		
		cJSON_AddTrueToObject(pJsonRoot, "result");
		cJSON_AddNullToObject(pJsonError, szRetBuf[4]);
	}

	//offline 
	DEBUGMSG(1,("2 szParseDest:%s\n", ((stMonitorSrc*)pSrc)->szParseDest));
	for (i = 0; i < PackArgc; i++)
		memset(PackArgv+i, 0, sizeof(*PackArgv));
	PackArgc = 0;
	if (Parse(((stMonitorSrc*)pSrc)->szParseDest, &PackArgc, PackArgv, delim) != 0)
	{
		cJSON_Delete(pJsonRoot);
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return (rtn = 1);
	}	

	/*if (PackArgc < 1)
	{
		cJSON_Delete(pJsonRoot);
		return (rtn = 2);
	}*/
		
	DEBUGMSG(0,("PackArgc:%d\n", PackArgc));
	if (PackArgc > 0)
	{
		DEBUGMSG(0,("PackArgv[0]:%s\n", PackArgv[0]));
		ArgLen = strlen(PackArgv[0]);
		DEBUGMSG(0,("ArgLen:%d\n", ArgLen));	
		DEBUGMSG(0,("dim(szRetBuf):%d\n",  dim(szRetBuf)));
		for (iRetIndex = 0; iRetIndex < dim(szRetBuf) - 1; iRetIndex++)
		{
			bufLen = strlen(szRetBuf[iRetIndex]);
			DEBUGMSG(0,("PackArgv[0]:%s, szRetBuf[%d]:%s\n", PackArgv[0], iRetIndex, szRetBuf[iRetIndex]));
			DEBUGMSG(0,("ArgLen:%d, bufLen:%d\n", ArgLen, bufLen));
			if (ArgLen == bufLen && strncasecmp(PackArgv[0], szRetBuf[iRetIndex], ArgLen) == 0)
			{
				DEBUGMSG(1,("1 iRetIndex:%d, bIdbufRtn:%d\n", iRetIndex, bIdbufRtn));
				switch(iRetIndex)
				{
				case 0:
					if (!bIdbufRtn)
						cJSON_AddNullToObject(pJsonRoot, "error");
					break;
					
				case 1:
				case 3:	
					//offline 
					//if (PackReplayError(pJsonRoot, PackArgc, PackArgv) != 0)
					//	 cJSON_Delete(pJsonRoot);
					if (NULL == pJsonError)
					{
						pJsonError = cJSON_CreateObject();
						if(NULL == pJsonError)
						{
							cJSON_Delete((cJSON *)pJsonRoot);
							return -1;
						}
					}		
					/*pJsonError = cJSON_CreateObject();
					if(NULL == pJsonError)
					{
						cJSON_Delete((cJSON *)pJsonRoot);
						return -1;
					}*/
					cJSON* pArray = cJSON_CreateArray();	
					if (pArray == NULL)
					{
						cJSON_Delete(pJsonRoot);
						return -1;
					}
					//offline
					for (j = 1; j < PackArgc; j++)
					{
						//iId = strtoul(PackArgv[j], 0, 0);
						//DEBUGMSG(1,("iId:%d\n", iId));
						//cJSON_AddNumberToObject(pArray, "", iId);
						DEBUGMSG(1,("PackArgv[%d]:%s\n", j, PackArgv[j]));
						cJSON_AddStringToObject(pArray, "", PackArgv[j]);
					}
					cJSON_AddItemToObject(pJsonError, PackArgv[0], pArray);
					//cJSON_AddItemToObject((cJSON *)pJsonRoot, "error", pJsonError);
					bParseDest = TRUE;
					DEBUGMSG(1,("2 iRetIndex:%d\n", iRetIndex));
					break;

				case 2:
					if (NULL == pJsonError)
					{
						pJsonError = cJSON_CreateObject();
						if(NULL == pJsonError)
						{
							cJSON_Delete((cJSON *)pJsonRoot);
							return -1;
						}
					}						
					/*pJsonError = cJSON_CreateObject();
					if(NULL == pJsonError)
					{
						cJSON_Delete((cJSON *)pJsonRoot);
						return -1;
					}*/
					cJSON_AddStringToObject(pJsonError, "offline", szRetBuf[iRetIndex]);
					bParseDest = TRUE;
					break;

				/*case 3:
					if (NULL == pJsonError)
					{
						pJsonError = cJSON_CreateObject();
						if(NULL == pJsonError)
						{
							cJSON_Delete((cJSON *)pJsonRoot);
							return -1;
						}
					}						

					cJSON_AddStringToObject(pJsonError, "notexist_pool", szRetBuf[iRetIndex]);
					bParseDest = TRUE;
					break;*/

				default:
					break;
				}
			}
		}
	}
	else
	{
		if (NULL == pJsonError)
		{
			pJsonError = cJSON_CreateObject();
			if(NULL == pJsonError)
			{
				cJSON_Delete((cJSON *)pJsonRoot);
				return -1;
			}
		}		
		cJSON_AddNullToObject(pJsonError, szRetBuf[1]);
	}

	//offline or notexist	
	//DEBUGMSG(1,("iRetIndex:%d, CommMonitorFlag.iBufPos:%d\n", iRetIndex, CommMonitorFlag.iBufPos));
	DEBUGMSG(1,("bParseDest:%d, bIdbufRtn:%d\n", bParseDest, bIdbufRtn));
	if (bParseDest || bIdbufRtn)
		cJSON_AddItemToObject((cJSON *)pJsonRoot, "error", pJsonError);

	pBuf = cJSON_Print(pJsonRoot);
    if(NULL == pBuf)
    {
        // create object faild, exit
        cJSON_Delete(pJsonRoot);
        return -1;
    }
	destLen = strlen(pBuf);
	strncpy(dest, pBuf, destLen);
	DEBUGMSG(1,("pBuf:%s\n", pBuf));
	DEBUGMSG(1,("destLen:%d\n", destLen));
	DEBUGMSG(1,("@:%p, dest:%s\n", dest, (char *)dest));

	//释放
	//cJSON_Delete(pJsonError);
	//DEBUGMSG(1,("1 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<,\n"));
	cJSON_Delete(pJsonRoot);
	//DEBUGMSG(1,("2 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<,\n"));
	//cJSON_Delete(pArray);
	//DEBUGMSG(1,("3 <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<,\n"));
	free(pBuf);

    return rtn;
}

int PackOK(void *pSrc, void *dest, int destLen, int iIndex, int rtnRecv)
{
	int rtn = 0;
	char *pBuf = NULL;
    cJSON * pJsonRoot = NULL;
	//char szErrBuf[BUFFER_LEN_64] = {0};
	/*char delim[] = " ";
	int PackArgc = 0;
	char *PackArgv[BUFFER_LEN_256] = {0};
	int ArgLen = 0, bufLen = 0;
	int i = 0, j = 0, iRetIndex = 0;
	int iId = 0;
	BOOL bParseDest = FALSE, bIdbufRtn = FALSE;*/

	DEBUGMSG(1,("%s...\n", __FUNCTION__));
	
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        return -1;
    }

    cJSON_AddNumberToObject(pJsonRoot, "id", *((stMonitorSrc*)pSrc)->pIdSeq);
	cJSON_AddStringToObject(pJsonRoot, "switchid", (char *)((stMonitorSrc*)pSrc)->pSwiDbInfo->DbSwi.szSwiId);
    cJSON_AddStringToObject(pJsonRoot, "method", (char *)((stMonitorSrc*)pSrc)->pMonitorComm[iIndex].cmd);
	cJSON_AddTrueToObject(pJsonRoot, "result");
	cJSON_AddNullToObject(pJsonRoot, "error");

	pBuf = cJSON_Print(pJsonRoot);
    if(NULL == pBuf)
    {
        // create object faild, exit
        cJSON_Delete(pJsonRoot);
        return -1;
    }
	destLen = strlen(pBuf);
	strncpy(dest, pBuf, destLen);
	DEBUGMSG(0,("pBuf:%s\n", pBuf));
	DEBUGMSG(0,("destLen:%d\n", destLen));
	DEBUGMSG(1,("err @:%p, dest:%s\n", dest, (char *)dest));

	//释放
	cJSON_Delete(pJsonRoot);
	free(pBuf);

	return rtn;
}


int PackError(void *pSrc, void *dest, int destLen, int iIndex, int rtnRecv)
{
	int rtn = 0;
	char *pBuf = NULL;
    cJSON * pJsonRoot = NULL;
	char szErrBuf[BUFFER_LEN_64] = {0};
	/*char delim[] = " ";
	int PackArgc = 0;
	char *PackArgv[BUFFER_LEN_256] = {0};
	int ArgLen = 0, bufLen = 0;
	int i = 0, j = 0, iRetIndex = 0;
	int iId = 0;
	BOOL bParseDest = FALSE, bIdbufRtn = FALSE;*/

	DEBUGMSG(1,("%s...\n", __FUNCTION__));
	
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        return -1;
    }

    cJSON_AddNumberToObject(pJsonRoot, "id", *((stMonitorSrc*)pSrc)->pIdSeq);
	cJSON_AddStringToObject(pJsonRoot, "switchid", (char *)((stMonitorSrc*)pSrc)->pSwiDbInfo->DbSwi.szSwiId);
    cJSON_AddStringToObject(pJsonRoot, "method", (char *)((stMonitorSrc*)pSrc)->pMonitorComm[iIndex].cmd);
	cJSON_AddFalseToObject(pJsonRoot, "result");

	/*cJSON* pArrayNotExist = cJSON_CreateArray();
	if (pArrayNotExist == NULL)
	{
		cJSON_Delete(pJsonRoot);
		return -1;
	}
	
	for (j = 1; j < PackArgc; j++)
	{
		iId = strtoul(PackArgv[j], 0, 0);
		cJSON_AddNumberToObject(pArrayNotExist, "", iId);
	}
	//cJSON_AddItemToObject(pJsonError, "notexist", pArrayNotExist);*/
	
	//cJSON_AddStringToObject(pJsonError, "", (char *)gCommandErr[rtn - 1].cmd);
	//cJSON_AddStringToObject(pJsonRoot, "error", (char *)gCommandErr[rtn - 1].cmd);

	if (rtnRecv == E_COMMAND_ERROR_POOL)
	{
		strncpy(szErrBuf, ((stMonitorSrc*)pSrc)->szParseDest, sizeof(szErrBuf));
	}
	else
	{
		DEBUGMSG(0,("cmd:%s\n", (char *)gCommandErr[rtnRecv - 1].cmd));
		strncpy(szErrBuf, (char *)gCommandErr[rtnRecv - 1].cmd, sizeof(szErrBuf));
		DEBUGMSG(0,("szErrBuf:%s\n", szErrBuf));
	}
	
	cJSON_AddStringToObject(pJsonRoot, "error", szErrBuf);

	pBuf = cJSON_Print(pJsonRoot);
    if(NULL == pBuf)
    {
        // create object faild, exit
        cJSON_Delete(pJsonRoot);
        return -1;
    }
	destLen = strlen(pBuf);
	strncpy(dest, pBuf, destLen);
	DEBUGMSG(0,("pBuf:%s\n", pBuf));
	DEBUGMSG(0,("destLen:%d\n", destLen));
	DEBUGMSG(1,("err @:%p, dest:%s\n", dest, (char *)dest));

	//释放
	cJSON_Delete(pJsonRoot);
	free(pBuf);

	return rtn;
}

int PackConfirm(void *pSrc, int iIndex, struct ap_net_connection_t *pConn)
{
	int rtn = 0;//, iSendRtn = 0;
	char *pBuf = NULL;
    cJSON * pJsonRoot = NULL;//, *pJsonError = NULL;
	void *dest = NULL;
	int destLen = 0;

	DEBUGMSG(1,("%s...\n", __FUNCTION__));
	
    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        return -1;
    }

    cJSON_AddNumberToObject(pJsonRoot, "id", *((stMonitorSrc*)pSrc)->pIdSeq);
	cJSON_AddStringToObject(pJsonRoot, "switchid", (char *)((stMonitorSrc*)pSrc)->pSwiDbInfo->DbSwi.szSwiId);
    cJSON_AddStringToObject(pJsonRoot, "method", (char *)((stMonitorSrc*)pSrc)->pMonitorComm[iIndex].cmd);
	cJSON_AddTrueToObject(pJsonRoot, "result");

	cJSON_AddNullToObject(pJsonRoot, "error");
	
	pBuf = cJSON_Print(pJsonRoot);
    if(NULL == pBuf)
    {
        // create object faild, exit
        cJSON_Delete(pJsonRoot);
        return -1;
    }
	destLen = strlen(pBuf);
	//init
	dest = pConn->sendBuf; 
	//sizeof(gCliSendDaTa.bySeCmdBuf);
	strncpy((char *)dest, pBuf, destLen);
	DEBUGMSG(1,("111 pBuf:%s\n", pBuf));
	DEBUGMSG(1,("111 destLen:%d\n", destLen));
	DEBUGMSG(1,("111 @:%p, dest:%s\n", dest, (char *)dest));

	//释放
	cJSON_Delete(pJsonRoot);
	free(pBuf);

	DEBUGMSG(0,("222 dest:%s\n", (char *)dest));
	if (rtn == 0)
	{
		//iSendRtn = 
		stApiNetMonitor.ConnectionPoolSend(
				pConn->parent, pConn->idx, 
				dest, 
				strlen(dest)); 					
			//DEBUGMSG(1,("3 Error iSendRtn:%d\n", iSendRtn));
		//清空发送区
		memset((char *)dest, 0, destLen);
		DEBUGMSG(0,("333 dest:%s\n", (char *)dest));
	}

	return rtn;
}

#if 0
int ParseRebootCommand(void *pJson, void *dest, int destLen, int *pSize)
{
	int rtn = 0;
	//int iParamLen = 0;
	int iCnt = 0;//iSize = 0, 
	//int i = 0;
	int iValue = 0;
	int iPos = 0;
	
	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	cJSON * pSub = cJSON_GetObjectItem((cJSON *)pJson, "param");
	
	if(NULL == pSub)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;		
	}
	/*else
	{
		iParamLen = strlen(pSub->valuestring);
		if (4 != iParamLen || strncasecmp(pSub->valuestring, "null", 4) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, command is error, error:[%s]!\r\n",  
				__FILE__, __FUNCTION__, __LINE__, pSub->valuestring);
		}
	}*/
	
    /*if((root = cJSON_Parse(pJson)) == NULL)
    {
        return -1;
    }*/
    *pSize = cJSON_GetArraySize(pSub);
	DEBUGMSG(1,("*pSize:%d\n", *pSize));
    for(iCnt = 0; iCnt < *pSize; iCnt++)
    {
        cJSON *pSubArr = cJSON_GetArrayItem(pSub, iCnt);
        if(NULL == pSubArr)
        {
            continue;
        }
        iValue = pSubArr->valueint;
		if (iCnt == *pSize - 1)
		{
			snprintf((char *)dest+iPos, destLen, "%d", iValue);
			iPos += strlen((char *)dest+iPos);
		}
		else
		{
			snprintf((char *)dest+iPos, destLen, "%d ", iValue);
			iPos += strlen((char *)dest+iPos);
		}

        DEBUGMSG(0,("iValue:[%d]\n", iValue));
    }   

	//"param":[ ] 则是对所有设备下发命令
	if (*pSize == 0)
	{
		iPos = 0;
		snprintf((char *)dest+iPos, destLen, "all");
		//iPos += strlen((char *)dest+iPos);
	}
	DEBUGMSG(0,("iPos:%d\n", iPos));
	DEBUGMSG(0,("rtn:%d, dest:%s\n", rtn, (char *)dest));

	//rtn = iSize;
	
	return rtn;
}
#endif

int ParseRebootCommand(void *pJson, void *dest, int destLen, int *pSize)
{
	int rtn = 0;
	//int iParamLen = 0;
	int iArrCnt = 0, iCnt = 0;//iSize = 0, 
	int i = 0;
	//int iValue = 0;
	int iPos = 0;
	cJSON *pSubArray = NULL;
	//int iCommLen = 0, iValuesLen = 0;
	int iSize = 0;
		
	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	cJSON *pJsonParam = cJSON_GetObjectItem((cJSON *)pJson, "param");
	
	if(NULL == pJsonParam)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;		
	}

	iSize = cJSON_GetArraySize(pJsonParam);

    for(iArrCnt = 0; iArrCnt < iSize; iArrCnt++)
    {
        pSubArray = cJSON_GetArrayItem(pJsonParam, iArrCnt);
        if(NULL == pSubArray)
        {
            continue;
        }
		DEBUGMSG(0,("1 type:%d, pSubArr->string:%s, valuestring:%s\n", 
			pSubArray->type, pSubArray->string, pSubArray->valuestring));

		DEBUGMSG(1,("1 type:%d, pSubArr->string:%s, valueint:%d\n", 
			pSubArray->type, pSubArray->string, pSubArray->valueint));		

		//第一个参数
		if (iArrCnt == 0)
		{
			if (pSubArray->string != NULL)
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, \"param\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
				return E_COMMAND_ERROR_JSON;
			}

			for (i = 0; i < dim(gReboot); i++)
			{
				if (pSubArray->valueint == i)
				{
					snprintf((char *)dest+iPos, destLen, "%s", (char *)gReboot[i].byReboot);
					iPos += strlen((char *)dest+iPos);
					break;
				}	
				if (i == dim(gReboot) - 1)
				{
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, \"led control parameters on/off/flash/normal\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
					return E_COMMAND_ERROR_LED;
				}		
			}
			
			/*for (i = 0; i < dim(gSetLed); i++)
			{
				//DEBUGMSG(1,("i:%d, bySetLed:%s\n", i, (char *)gSetLed[i].bySetLed));
				//if (strcmp((char *)gSetLed[i].bySetLed, "off") == 0)
				//	DEBUGMSG(1,("off..............\n"));
				iCommLen = strlen((char *)gSetLed[i].bySetLed);
				iValuesLen = strlen(pSubArray->valuestring);
				DEBUGMSG(1,("iCommLen:%d, iValuesLen:%d\n", iCommLen, iValuesLen));
				if (pSubArray->valuestring != NULL 
					&& iCommLen == iValuesLen && strncmp(pSubArray->valuestring, (char *)gSetLed[i].bySetLed, iCommLen) == 0)
				{
					snprintf((char *)dest+iPos, destLen, "%s", pSubArray->valuestring);
					iPos += strlen((char *)dest+iPos);
					break;
				}
				if (i == dim(gSetLed) - 1)
				{
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, \"led control parameters on/off/flash/normal\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
					return E_COMMAND_ERROR_LED;
				}
			}*/
			continue;
		}

	    *pSize = cJSON_GetArraySize(pSubArray);
		DEBUGMSG(1,("pSize:%d\n", *pSize));
	    for(iCnt = 0; iCnt < *pSize; iCnt++)
	    {
	        cJSON *pSubArrParma = cJSON_GetArrayItem(pSubArray, iCnt);
	        if(NULL == pSubArrParma)
	        {
	            continue;
	        }
	        //iValue = pSubArrParma->valueint;

			if (pSubArrParma->valuestring != NULL)
			{
				snprintf((char *)dest+iPos, destLen, " %s", pSubArrParma->valuestring);
				iPos += strlen((char *)dest+iPos);
				DEBUGMSG(1,("valuestring:[%s]\n", pSubArrParma->valuestring));
			}
	    }   
	}
	//DEBUGMSG(1,("pSubArray->string: %s\n", pSubArray->string));
	DEBUGMSG(1,("1 dest:%s\n", (char *)dest));

	//"param":[ ] 则是对所有设备下发命令
	if (*pSize == 0)
	{
		snprintf((char *)dest+iPos, destLen, " all");
		//iPos += strlen((char *)dest+iPos);
	}
	DEBUGMSG(1,("iPos:%d\n", iPos));
	DEBUGMSG(1,("rtn:%d, dest:%s\n", rtn, (char *)dest));

	//rtn = iSize;
	
	return rtn;
}

#if 0
int ParseSetLedCommand(void *pJson, void *dest, int destLen, int *pSize)
{
	int rtn = 0;
	//int iParamLen = 0;
	int iCnt = 0;//iSize = 0, 
	int i = 0;
	int iValue = 0;
	int iPos = 0;
	cJSON *pSubArray = NULL;
		
	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	cJSON *pJsonParam = cJSON_GetObjectItem((cJSON *)pJson, "param");
	
	if(NULL == pJsonParam)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;		
	}

	/*cJSON * pJsonError = cJSON_GetObjectItem(pJsonParam, "error");
	if(NULL == pJsonError)
	{
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"error\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}																											
	printf("error : %s\n", pJsonError->string);*/


	for (i = 0; i < dim(gSetLed); i++)
	{
		//DEBUGMSG(1,("i:%d, bySetLed:%s\n", i, (char *)gSetLed[i].bySetLed));
		//if (strcmp((char *)gSetLed[i].bySetLed, "off") == 0)
		//	DEBUGMSG(1,("off..............\n"));
		pSubArray = cJSON_GetObjectItem(pJsonParam, (char *)gSetLed[i].bySetLed);
		if (pSubArray != NULL)
		{
			break;
		}
	}

	//pSubArray = cJSON_GetObjectItem(pJsonParam, "on");
	if(NULL == pSubArray)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"led control parameters on/off/flash/normal\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_LED;
		//snprintf((char *)dest + iPos, destLen, "%s", szRetBuf[3]);
		//iPos += strlen((char *)dest + iPos);	
		//return -1;
	}
	else
	{
		snprintf((char *)dest+iPos, destLen, "%s", pSubArray->string);
		iPos += strlen((char *)dest+iPos);
	}
	DEBUGMSG(1,("pSubArray->string: %s\n", pSubArray->string));
	DEBUGMSG(1,("1 dest:%s\n", (char *)dest));
	
    *pSize = cJSON_GetArraySize(pSubArray);
	DEBUGMSG(1,("pSize:%d\n", *pSize));
    for(iCnt = 0; iCnt < *pSize; iCnt++)
    {
        cJSON *pSubArrParma = cJSON_GetArrayItem(pSubArray, iCnt);
        if(NULL == pSubArrParma)
        {
            continue;
        }
        iValue = pSubArrParma->valueint;
		/*if (iCnt == 0)
		{
			snprintf((char *)dest+iPos, destLen, "%d", iValue);
			iPos += strlen((char *)dest+iPos);
		}
		else
		{
			snprintf((char *)dest+iPos, destLen, " %d", iValue);
			iPos += strlen((char *)dest+iPos);
		}*/
		snprintf((char *)dest+iPos, destLen, " %d", iValue);
		iPos += strlen((char *)dest+iPos);

        DEBUGMSG(1,("iValue:[%d]\n", iValue));
    }   

	//"param":[ ] 则是对所有设备下发命令
	if (*pSize == 0)
	{
		snprintf((char *)dest+iPos, destLen, " all");
		//iPos += strlen((char *)dest+iPos);
	}
	DEBUGMSG(1,("iPos:%d\n", iPos));
	DEBUGMSG(1,("rtn:%d, dest:%s\n", rtn, (char *)dest));

	//rtn = iSize;
	
	return rtn;
}
#endif

int NoParseMonitor(void *pJson, void *dest, int destLen, int *pSize)
{
	int rtn = 0;


	//GetLocalTimeLog();
	//ap_log_debug_log("\t\t%s, %s, %d, \"error\" param is error!(%s)\r\n",	
	//	__FILE__, __FUNCTION__, __LINE__, pJsonParam->valuestring);

	return rtn;
}

int ParseRefuse(void *pJson, void *dest, int destLen, int *pSize)
{
	int rtn = 0;

	cJSON *pJsonParam = cJSON_GetObjectItem((cJSON *)pJson, "error");
	
	if(NULL == pJsonParam)
	{
		// get "error" from json faild
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"error\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;		
	}
	
	if (pJsonParam->valuestring != NULL)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"error\" param is error!(%s)\r\n",	
			__FILE__, __FUNCTION__, __LINE__, pJsonParam->valuestring);
	}

	return rtn;
}


int ParseSetLedCommand(void *pJson, void *dest, int destLen, int *pSize)
{
	int rtn = 0;
	//int iParamLen = 0;
	int iArrCnt = 0, iCnt = 0;//iSize = 0, 
	int i = 0;
	//int iValue = 0;
	int iPos = 0;
	cJSON *pSubArray = NULL;
	//int iCommLen = 0, iValuesLen = 0;
	int iSize = 0;
		
	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	cJSON *pJsonParam = cJSON_GetObjectItem((cJSON *)pJson, "param");
	
	if(NULL == pJsonParam)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;		
	}

	iSize = cJSON_GetArraySize(pJsonParam);

    for(iArrCnt = 0; iArrCnt < iSize; iArrCnt++)
    {
        pSubArray = cJSON_GetArrayItem(pJsonParam, iArrCnt);
        if(NULL == pSubArray)
        {
            continue;
        }
		DEBUGMSG(0,("1 type:%d, pSubArr->string:%s, valuestring:%s\n", 
			pSubArray->type, pSubArray->string, pSubArray->valuestring));

		DEBUGMSG(1,("1 type:%d, pSubArr->string:%s, valueint:%d\n", 
			pSubArray->type, pSubArray->string, pSubArray->valueint));		

		//第一个参数
		if (iArrCnt == 0)
		{
			if (pSubArray->string != NULL)
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, \"param\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
				return E_COMMAND_ERROR_JSON;
			}

			for (i = 0; i < dim(gSetLed); i++)
			{
				if (pSubArray->valueint == i)
				{
					snprintf((char *)dest+iPos, destLen, "%s", (char *)gSetLed[i].bySetLed);
					iPos += strlen((char *)dest+iPos);
					break;
				}	
				if (i == dim(gSetLed) - 1)
				{
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, \"led control parameters on/off/flash/normal\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
					return E_COMMAND_ERROR_LED;
				}		
			}
			
			/*for (i = 0; i < dim(gSetLed); i++)
			{
				//DEBUGMSG(1,("i:%d, bySetLed:%s\n", i, (char *)gSetLed[i].bySetLed));
				//if (strcmp((char *)gSetLed[i].bySetLed, "off") == 0)
				//	DEBUGMSG(1,("off..............\n"));
				iCommLen = strlen((char *)gSetLed[i].bySetLed);
				iValuesLen = strlen(pSubArray->valuestring);
				DEBUGMSG(1,("iCommLen:%d, iValuesLen:%d\n", iCommLen, iValuesLen));
				if (pSubArray->valuestring != NULL 
					&& iCommLen == iValuesLen && strncmp(pSubArray->valuestring, (char *)gSetLed[i].bySetLed, iCommLen) == 0)
				{
					snprintf((char *)dest+iPos, destLen, "%s", pSubArray->valuestring);
					iPos += strlen((char *)dest+iPos);
					break;
				}
				if (i == dim(gSetLed) - 1)
				{
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, \"led control parameters on/off/flash/normal\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
					return E_COMMAND_ERROR_LED;
				}
			}*/
			continue;
		}

	    *pSize = cJSON_GetArraySize(pSubArray);
		DEBUGMSG(1,("pSize:%d\n", *pSize));
	    for(iCnt = 0; iCnt < *pSize; iCnt++)
	    {
	        cJSON *pSubArrParma = cJSON_GetArrayItem(pSubArray, iCnt);
	        if(NULL == pSubArrParma)
	        {
	            continue;
	        }
	        //iValue = pSubArrParma->valueint;

			if (pSubArrParma->valuestring != NULL)
			{
				snprintf((char *)dest+iPos, destLen, " %s", pSubArrParma->valuestring);
				iPos += strlen((char *)dest+iPos);
				DEBUGMSG(1,("valuestring:[%s]\n", pSubArrParma->valuestring));
			}
	    }   
	}
	//DEBUGMSG(1,("pSubArray->string: %s\n", pSubArray->string));
	DEBUGMSG(1,("1 dest:%s\n", (char *)dest));

	//"param":[ ] 则是对所有设备下发命令
	if (*pSize == 0)
	{
		snprintf((char *)dest+iPos, destLen, " all");
		//iPos += strlen((char *)dest+iPos);
	}
	DEBUGMSG(1,("iPos:%d\n", iPos));
	DEBUGMSG(1,("rtn:%d, dest:%s\n", rtn, (char *)dest));

	//rtn = iSize;
	
	return rtn;
}
int ParseAddPoolCommand(void *pJson, void *dest, int destLen, int *pSize)
{
	int rtn = 0;
	//int iParamLen = 0;
	int iCnt = 0;//iSize = 0, 
	int i = 0;
	//int iValue = 0;
	int iPos = 0;
	cJSON * pSubArr = NULL, *pSubArrToJson = NULL;
	
	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	cJSON *pSubArray = cJSON_GetObjectItem((cJSON *)pJson, "param");
	if(NULL == pSubArray)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;		
	}

    *pSize = cJSON_GetArraySize(pSubArray);
	printf("*pSize:%d\n", *pSize);
    for(iCnt = 0; iCnt < *pSize; iCnt++)
    {
       	pSubArr = cJSON_GetArrayItem(pSubArray, iCnt);
        if(NULL == pSubArr)
        {
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArr!\r\n",  __FILE__, __FUNCTION__, __LINE__);        
            continue;
        }

		DEBUGMSG(1,("1 gCrrMaxPoolId:%d\n", gCrrMaxPoolId));
		snprintf((char *)dest+iPos, destLen, "%d\"", ++gCrrMaxPoolId);
		iPos += strlen((char *)dest+iPos);

		for (i = 0; i < dim(szPoolParam) - 1; i++)
		{
			pSubArrToJson = cJSON_GetObjectItem(pSubArr, szPoolParam[i]);
			if(NULL != pSubArrToJson)
			{
				DEBUGMSG(1,("url:%s\n", pSubArrToJson->valuestring));
				snprintf((char *)dest+iPos, destLen, "%s\"", pSubArrToJson->valuestring);
				iPos += strlen((char *)dest+iPos);
			}
			else
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArrToJson, szPoolParam :%s!\r\n",  
					__FILE__, __FUNCTION__, __LINE__, szPoolParam[i]);
				break;
			}
		}
		if(NULL == pSubArrToJson)
		{
			continue;
		}

		/*pSubArrToJson = cJSON_GetObjectItem(pSubArr, "user");
		DEBUGMSG(1,("user:%s\n", pSubArrToJson->valuestring));
		snprintf((char *)dest+iPos, destLen, "%s\"", pSubArrToJson->valuestring);
		iPos += strlen((char *)dest+iPos);

		pSubArrToJson = cJSON_GetObjectItem(pSubArr, "passwd");
		DEBUGMSG(1,("passwd:%s\n", pSubArrToJson->valuestring));
		snprintf((char *)dest+iPos, destLen, "%s\"", pSubArrToJson->valuestring);
		iPos += strlen((char *)dest+iPos);*/

		pSubArrToJson = cJSON_GetObjectItem(pSubArr, szPoolParam[3]);
		DEBUGMSG(1,("algo:%s\n", pSubArrToJson->valuestring));
		if(NULL == pSubArrToJson)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArrToJson, szPoolParam :%s!\r\n",  
				__FILE__, __FUNCTION__, __LINE__, szPoolParam[3]);	
			continue;
		}
		else
		{
			if (iCnt == *pSize - 1)
			{
				snprintf((char *)dest+iPos, destLen, "%s", pSubArrToJson->valuestring);
			}
			else
			{
				snprintf((char *)dest+iPos, destLen, "%s ", pSubArrToJson->valuestring);
			}
			iPos += strlen((char *)dest+iPos);
		}
	} 

	DEBUGMSG(0,("iPos:%d\n", iPos));
	DEBUGMSG(0,("rtn:%d, dest:%s\n", rtn, (char *)dest));
	
	return rtn;
}

int ParseDelPoolCommand(void *pJson, void *dest, int destLen, int *pSize)
{
	int rtn = 0;
	//int iParamLen = 0;
	int iCnt = 0;//iSize = 0, 
	//int i = 0;
	int iValue = 0;
	int iPos = 0;
	
	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	cJSON * pSub = cJSON_GetObjectItem((cJSON *)pJson, "param");
	
	if(NULL == pSub)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;		
	}

    *pSize = cJSON_GetArraySize(pSub);
	DEBUGMSG(1,("*pSize:%d\n", *pSize));
    for(iCnt = 0; iCnt < *pSize; iCnt++)
    {
        cJSON *pSubArr = cJSON_GetArrayItem(pSub, iCnt);
        if(NULL == pSubArr)
        {
            continue;
        }
        iValue = pSubArr->valueint;
		if (iCnt == *pSize - 1)
		{
			snprintf((char *)dest+iPos, destLen, "%d", iValue);
			iPos += strlen((char *)dest+iPos);
		}
		else
		{
			snprintf((char *)dest+iPos, destLen, "%d ", iValue);
			iPos += strlen((char *)dest+iPos);
		}

        DEBUGMSG(0,("iValue:[%d]\n", iValue));
    }   

	//"param":[ ] 则是对所有设备下发命令
	if (*pSize == 0)
	{
		iPos = 0;
		snprintf((char *)dest+iPos, destLen, "all");
		//iPos += strlen((char *)dest+iPos);
	}
	DEBUGMSG(0,("iPos:%d\n", iPos));
	DEBUGMSG(1,("rtn:%d, dest:%s\n", rtn, (char *)dest));
	
	return rtn;
}

/*int ParseSetdefaultPoolCommand(void *pJson, void *dest, int destLen, int *pSize)
{
	int rtn = 0;
	//int iParamLen = 0;
	int iCnt = 0;//iSize = 0, 
	//int i = 0;
	int iValue = 0;
	int iPos = 0;
	
	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	cJSON * pSub = cJSON_GetObjectItem((cJSON *)pJson, "param");
	
	if(NULL == pSub)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;		
	}

    *pSize = cJSON_GetArraySize(pSub);
	DEBUGMSG(1,("*pSize:%d\n", *pSize));
    for(iCnt = 0; iCnt < *pSize; iCnt++)
    {
        cJSON *pSubArr = cJSON_GetArrayItem(pSub, iCnt);
        if(NULL == pSubArr)
        {
            continue;
        }
        iValue = pSubArr->valueint;
		if (iCnt == *pSize - 1)
		{
			snprintf((char *)dest+iPos, destLen, "%d", iValue);
			iPos += strlen((char *)dest+iPos);
		}
		else
		{
			snprintf((char *)dest+iPos, destLen, "%d ", iValue);
			iPos += strlen((char *)dest+iPos);
		}

        DEBUGMSG(0,("iValue:[%d]\n", iValue));
    }   

	//"param":[ ] 则是对所有设备下发命令
	if (*pSize == 0)
	{
		iPos = 0;
		snprintf((char *)dest+iPos, destLen, "all");
		//iPos += strlen((char *)dest+iPos);
	}
	DEBUGMSG(0,("iPos:%d\n", iPos));
	DEBUGMSG(1,("rtn:%d, dest:%s\n", rtn, (char *)dest));
	
	return rtn;
}*/

int ParseSetdefaultPoolCommand(void *pJson, void *dest, int destLen, int *pSize)
{
	int rtn = 0, rtnModify = 0, iQueryRtn = 0;
	//int iParamLen = 0;
	int iCnt = 0;//iSize = 0, 
	//int i = 0;
	//int iValue = 0;
	//int iPos = 0;
	cJSON * pSubArr = NULL, *pSubArrToJson = NULL;
	//int iBTCAlgoLen = 0, iLTCAlgoLen = 0;
	int iIndex = 0;
	char Packbuf[BUFFER_LEN_1024] = {0};
	char bufDest[BUFFER_LEN_512] = {0};
	
	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	cJSON *pSubArray = cJSON_GetObjectItem((cJSON *)pJson, "param");
	if(NULL == pSubArray)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;		
	}

    *pSize = cJSON_GetArraySize(pSubArray);
	DEBUGMSG(1,("*pSize:%d\n", *pSize));
	//清空BUF和数据库
	memset(&gPoolDefaultConf, 0, sizeof(stPoolFixedConf));
	PoolConfigQuery[E_QUERY_TYPE_DELETE - 1].QueryPackFunc2(
														Packbuf, 
														&gCfgDataDL.PoolInfo, 
														&gCfgDataDL.PoolConfigColName, 
														bufDest, 
														sizeof(bufDest));
	pthread_mutex_lock(&gPoolConfigLock);
	iQueryRtn = MysqlFunc.dbDoQuery(&myPoolConfig, bufDest, E_QUERY_TYPE_DELETE, NULL);
	pthread_mutex_unlock(&gPoolConfigLock);
	memset(bufDest, 0, sizeof(bufDest));

	
	//当参数超过6个，就赋值为6
	if (*pSize > COMMANDLINE_6)
		*pSize = COMMANDLINE_6;
    for(iCnt = 0; iCnt < *pSize; iCnt++)
    {
       	pSubArr = cJSON_GetArrayItem(pSubArray, iCnt);
        if(NULL == pSubArr)
        {
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArr!\r\n",  
				__FILE__, __FUNCTION__, __LINE__);        
            continue;
        }
	
		pSubArrToJson = cJSON_GetObjectItem(pSubArr, szPoolParam[3]);
		
		if(NULL == pSubArrToJson)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pSubArrToJson, %s:%d!\r\n",  
				__FILE__, __FUNCTION__, __LINE__, szPoolParam[3], pSubArrToJson->valueint);	
			continue;
		}
		else
		{
			DEBUGMSG(1,("algo:%d\n", pSubArrToJson->valueint));		
			switch(pSubArrToJson->valueint - 1)
			{
				case E_ALGO_BTC:
					iIndex = gPoolDefaultConf.iBTCPoolCount;

					gPoolDefaultConf.stBTCPoolConf[iIndex].iPoolId = iCnt + 1;
					gPoolDefaultConf.stBTCPoolConf[iIndex].eAlgo = pSubArrToJson->valueint - 1; 

					rtnModify = ModifyBTCPool(pSubArray, &gPoolDefaultConf, iCnt, iIndex);
					if (rtnModify == 0)
						if (++gPoolDefaultConf.iBTCPoolCount > MAX_POOL_COUNT)
							gPoolDefaultConf.iBTCPoolCount = MAX_POOL_COUNT;
					DEBUGMSG(1,("2 iBTCPoolCount:%d\n", gPoolDefaultConf.iBTCPoolCount));			
					
					if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "%d,'%s','%s','%s',%d", 
						gPoolDefaultConf.stBTCPoolConf[iIndex].iPoolId, 
						(char *)gPoolDefaultConf.stBTCPoolConf[iIndex].szUrl, 
						(char *)gPoolDefaultConf.stBTCPoolConf[iIndex].szUser, 
						(char *)gPoolDefaultConf.stBTCPoolConf[iIndex].szPasswd, 
						gPoolDefaultConf.stBTCPoolConf[iIndex].eAlgo)) < 0)
						return rtn;
					
					break;

				case E_ALGO_LTC:					
					iIndex = gPoolDefaultConf.iLTCPoolCount;

					gPoolDefaultConf.stLTCPoolConf[iIndex].iPoolId = iCnt + 1;
					gPoolDefaultConf.stLTCPoolConf[iIndex].eAlgo = pSubArrToJson->valueint - 1;
					
					rtnModify = ModifyLTCPool(pSubArray, &gPoolDefaultConf, iCnt, iIndex);
					if (rtnModify == 0)
						if (++gPoolDefaultConf.iLTCPoolCount > MAX_POOL_COUNT)
							gPoolDefaultConf.iLTCPoolCount = MAX_POOL_COUNT;
					DEBUGMSG(1,("2 iLTCPoolCount:%d\n", gPoolDefaultConf.iLTCPoolCount));

					if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "%d,'%s','%s','%s',%d", 
						gPoolDefaultConf.stLTCPoolConf[iIndex].iPoolId, 
						(char *)gPoolDefaultConf.stLTCPoolConf[iIndex].szUrl, 
						(char *)gPoolDefaultConf.stLTCPoolConf[iIndex].szUser, 
						(char *)gPoolDefaultConf.stLTCPoolConf[iIndex].szPasswd, 
						gPoolDefaultConf.stLTCPoolConf[iIndex].eAlgo)) < 0)
						return rtn;	
					break;
					
				default:
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, Algorithm[%d] does not exist!\r\n", 
						__FILE__, __FUNCTION__, __LINE__, pSubArrToJson->valueint - 1);
					break;
			}

			DEBUGMSG(1,("1 iBTCPoolCount:%d\n", gPoolDefaultConf.iBTCPoolCount));
			DEBUGMSG(1,("1 iLTCPoolCount:%d\n", gPoolDefaultConf.iLTCPoolCount));
			/*if (gPoolDefaultConf.iBTCPoolCount > MAX_POOL_COUNT)
			{
				gPoolDefaultConf.iBTCPoolCount = MAX_POOL_COUNT;
				continue;
			}
			DEBUGMSG(1,("1 iLTCPoolCount:%d\n", gPoolDefaultConf.iLTCPoolCount));
			if (gPoolDefaultConf.iLTCPoolCount > MAX_POOL_COUNT)
			{
				gPoolDefaultConf.iLTCPoolCount = MAX_POOL_COUNT;
				continue;
			}*/		


			DEBUGMSG(1,("Packbuf:%s\r\n", Packbuf));
			PoolConfigQuery[E_QUERY_TYPE_INSERT - 1].QueryPackFunc1(
																Packbuf, 
																&gCfgDataDL.PoolInfo, 
																&gCfgDataDL.PoolConfigColName, 
																bufDest, 
																sizeof(bufDest));
			pthread_mutex_lock(&gPoolConfigLock);
			iQueryRtn = MysqlFunc.dbDoQuery(&myPoolConfig, bufDest, E_QUERY_TYPE_INSERT, NULL);
			pthread_mutex_unlock(&gPoolConfigLock);

			
			if (iQueryRtn != 0)
			{
				//strcpy((char *)dest + iPos, szRetBuf[0]);
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, PoolConfigQuery is error iQueryRtn:%d\r\n",  
					__FILE__, __FUNCTION__, __LINE__, iQueryRtn);				
			}
			rtn = iQueryRtn;	

			memset(bufDest, 0, sizeof(bufDest));
			
			if (rtnModify < 0)
				continue;			
		}

	} 

	//DEBUGMSG(1,("iPos:%d\n", iPos));
	//DEBUGMSG(1,("rtn:%d, dest:%s\n", rtn, (char *)dest));
	
	return rtn;
}

/*
int ParseSetPoolCommand(void *pJson, void *dest, int destLen, int *pSize)
{
	int rtn = 0;
	int iCnt = 0, iSubCnt = 0;
	//int i = 0;
	int iSubSize = 0, iValue = 0;//iSize = 0, 
	int iPos = 0;
	cJSON *pSubArr = NULL, *pSubArrSubA = NULL;
	
	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	cJSON * pSub = cJSON_GetObjectItem((cJSON *)pJson, "param");
	
	if(NULL == pSub)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;		
	}

    *pSize = cJSON_GetArraySize(pSub);
	DEBUGMSG(1,("*pSize:%d\n", *pSize));

	if (*pSize != COMMANDLINE_2)
	{
		return E_COMMAND_ERROR_IDNUM;
	}

	for(iCnt = 0; iCnt < *pSize; iCnt++)	
    {
        pSubArr = cJSON_GetArrayItem(pSub, iCnt);
        if(NULL == pSubArr)
        {
            continue;
        }

		iSubSize = cJSON_GetArraySize(pSubArr);
		if (iCnt == 0)
		{
			//pool_id不能为空
			if (iSubSize == 0)
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, pool_id can not be empty!\r\n",	
					__FILE__, __FUNCTION__, __LINE__);				
				return E_COMMAND_ERROR_IDNUM;
			}
		}
		else if (iCnt == 1)
		{	
			//if list is empty then mean all cell_ids
			if (iSubSize == 0)
			{
				snprintf((char *)dest+iPos, destLen, " all");
				iPos += strlen((char *)dest+iPos);
				return rtn;
			}
		}			
		
		DEBUGMSG(1,("iSubSize:%d\n", iSubSize));
	    for(iSubCnt = 0; iSubCnt < iSubSize; iSubCnt++)
	    {
			pSubArrSubA = cJSON_GetArrayItem(pSubArr, iSubCnt);
			iValue = pSubArrSubA->valueint;
			if (iSubCnt == 0)
			{
				if (iCnt == 0)
					snprintf((char *)dest+iPos, destLen, "pool_id,%d", iValue);
				else
					snprintf((char *)dest+iPos, destLen, " cell_id,%d", iValue);
				iPos += strlen((char *)dest+iPos);
			}
			else
			{
				snprintf((char *)dest+iPos, destLen, ",%d", iValue);
				iPos += strlen((char *)dest+iPos);
			}
			DEBUGMSG(1,("iValue:[%d]\n", iValue));
		}
    } 

	//"param":[ ] 则是对所有设备下发命令
	//if (*pSize == 0)
	//{
	//	iPos = 0;
	//	snprintf((char *)dest+iPos, destLen, "all");
	//	//iPos += strlen((char *)dest+iPos);
	//}
	DEBUGMSG(0,("iPos:%d\n", iPos));
	DEBUGMSG(1,("rtn:%d, dest:%s\n", rtn, (char *)dest));
	
	return rtn;
}
*/

int ParseSetPoolCommand(void *pJson, void *dest, int destLen, int *pSize)
{
	int rtn = 0;
	int iCnt = 0, iSubCnt = 0;
	//int i = 0;
	int iSubSize = 0;//, iValue = 0;//iSize = 0, 
	int iPos = 0;
	cJSON *pSubArr = NULL, *pSubArrSubA = NULL;
	//stPoolConfInfo *poolValue = NULL;

	char destPoolBuf[BUFFER_LEN_1024] = {0};
	int destLenPoolBuf = sizeof(destPoolBuf);
	char szCellIdStr[BUFFER_LEN_32] = {0};
	
	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	cJSON * pSub = cJSON_GetObjectItem((cJSON *)pJson, "param");
	
	if(NULL == pSub)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;		
	}

    *pSize = cJSON_GetArraySize(pSub);
	DEBUGMSG(1,("*pSize:%d\n", *pSize));

	if (*pSize != COMMANDLINE_2)
	{
		return E_COMMAND_ERROR_IDNUM;
	}

	//建立链表
	//poolValue = malloc(sizeof(stPoolConfInfo));
	//memset(poolValue, 0, sizeof(stPoolConfInfo));
	//pList = &stPoolConfList;

	for(iCnt = 0; iCnt < *pSize; iCnt++)	
    {
    	DEBUGMSG(1,("111 dest:%s\n", (char *)dest));
        pSubArr = cJSON_GetArrayItem(pSub, iCnt);
        if(NULL == pSubArr)
        {
            continue;
        }

		iSubSize = cJSON_GetArraySize(pSubArr);
		DEBUGMSG(1,("iCnt:%d, iSubSize:%d\n", iCnt, iSubSize));
		if (iCnt == 0)
		{
			//pool_id不能为空
			if (iSubSize == 0)
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, pool_id can not be empty!\r\n",	
					__FILE__, __FUNCTION__, __LINE__);				
				return E_COMMAND_ERROR_IDNUM;
			}
		}
		else if (iCnt == 1)
		{	
			//if list is empty then mean all cell_ids
			if (iSubSize == 0)
			{
				snprintf((char *)dest+iPos, destLen, " cell_id,all");
				iPos += strlen((char *)dest+iPos);
				return rtn;
			}
		}			
		DEBUGMSG(1,("222 dest:%s\n", (char *)dest));
			
		DEBUGMSG(1,("iSubSize:%d\n", iSubSize));
		
	    for(iSubCnt = 0; iSubCnt < iSubSize; iSubCnt++)
	    {
			pSubArrSubA = cJSON_GetArrayItem(pSubArr, iSubCnt);
			//if (iCnt == 0)
			//	iValue = pSubArrSubA->valueint;
			if (iCnt == 1 && pSubArrSubA->valuestring != NULL)
				strncpy(szCellIdStr, pSubArrSubA->valuestring, sizeof(szCellIdStr));
			if (iSubCnt == 0)
			{
				if (iCnt == 0)
				{
					//每次重新设置起始pool_id
					gCrrMaxPoolId = 0;
					if (SetParseAddPoolToString(pSubArrSubA, destPoolBuf, destLenPoolBuf, &iSubSize, iSubCnt) != 0)
						continue;
					
					//清空缓冲  这里可清除，也可不清除，2015-04-23
					SetDelAllPoolList();
					if (SetAddPoolToList(destPoolBuf, destLenPoolBuf) != 0)
						continue;
								
					snprintf((char *)dest+iPos, destLen, "pool_id,%d", iSubCnt+1);
				}				
				else
					snprintf((char *)dest+iPos, destLen, " cell_id,%s", szCellIdStr);
				iPos += strlen((char *)dest+iPos);

				DEBUGMSG(1,("333 dest:%s\n", (char *)dest));
			}
			else
			{
				if (iCnt == 0)
				{
					if (SetParseAddPoolToString(pSubArrSubA, destPoolBuf, destLenPoolBuf, &iSubSize, iSubCnt) != 0)
						continue;
					if (SetAddPoolToList(destPoolBuf, destLenPoolBuf) != 0)
						continue;
												
					snprintf((char *)dest+iPos, destLen, ",%d", iSubCnt+1);
				}
				else
				{
					snprintf((char *)dest+iPos, destLen, ",%s", szCellIdStr);
				}
				
				iPos += strlen((char *)dest+iPos);
				DEBUGMSG(1,("444 dest:%s\n", (char *)dest));
			}
			//DEBUGMSG(1,("iValue:[%d]\n", iValue));
		}
    } 

	//"param":[ ] 则是对所有设备下发命令
	/*if (*pSize == 0)
	{
		iPos = 0;
		snprintf((char *)dest+iPos, destLen, "all");
		//iPos += strlen((char *)dest+iPos);
	}*/
	DEBUGMSG(0,("iPos:%d\n", iPos));
	DEBUGMSG(1,("rtn:%d, 1 Spool dest:%s\n", rtn, (char *)dest));

	DEBUGMSG(1,("555 dest:%s\n", (char *)dest));
	
	return rtn;
}

int ParseCellSearch(void * pJson, void *dest, int destLen, int *pSize)
{
	int rtn = 0;
	//int iCnt = 0;
	//int i = 0;
	//int iValue = 0;
	int iPos = 0;
	
	DEBUGMSG(1,("%s...\n", __FUNCTION__));
	
    // get param from json
	cJSON * pSub = cJSON_GetObjectItem((cJSON *)pJson, "param");
	
	if(NULL == pSub)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;		
	}
	//"param":[ ] 则是对所有设备下发命令
	if (*pSize == 0)
	{
		iPos = 0;
		snprintf((char *)dest+iPos, destLen, "all");
		//iPos += strlen((char *)dest+iPos);
	}
	DEBUGMSG(0,("iPos:%d\n", iPos));
	DEBUGMSG(0,("rtn:%d, dest:%s\n", rtn, (char *)dest));


	char destBuf[BUFFER_LEN_2048] = {0};
	int destBufLen = sizeof(destBuf), iSendLen = 0;

	if (PackCellSearchToMulticast((void *)&MoniSrc, gpLoginPool, destBuf, destBufLen, &iSendLen) != 0)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Pack CellSearch is error!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;		
	}

	DEBUGMSG(1,("-------------------------------iSendLen:%d\n", iSendLen));
	DEBUGMSG(1,("destBuf:%s\n", destBuf));

	if (stApiNetWork.ApiNetSendToClient(
				gpLoginPool->listener.sock, 
				gpLoginPool->MulticastAddr4, 
				destBuf, iSendLen, gpLoginPool) < 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, ApiNetSendToClient error!\r\n",  __FILE__, __FUNCTION__, __LINE__);	
	}
	else
	{
		usleep(1);				  //避免出现EAGAIN
	}

	return rtn;
}


int ParseReplay(void * pJson, void *dest, int destLen, int *pSize)
{
	int rtn = 0;
	//int iParamLen = 0;//, iMethodLen = 0;
	//cJSON * pJson = cJSON_Parse(pMsg);
	//char valBuf[BUFFER_LEN_16] = {0};
	
	DEBUGMSG(1,("%s...\n", __FUNCTION__));
	
	/*if (NULL == pJson)                                                                                         
	{
		// parse faild, return
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, pJson is NULL!\r\n",  __FILE__, __FUNCTION__, __LINE__);	      
		return -1;
	}

    // get "method" from json
    cJSON * pSub = cJSON_GetObjectItem(pJson, "method");
    if(NULL == pSub)
    {
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"method\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);   
		return -1;
    }
	else
	{
		iMethodLen = strlen((char *)((stMonitorSrc*)pSrc)->pMonitorComm->cmd);
		iParamLen = strlen(pSub->valuestring);
		if (iMethodLen != iParamLen || 
			strncasecmp((char *)((stMonitorSrc*)pSrc)->pMonitorComm->cmd, pSub->valuestring, iMethodLen) != 0)
		{		
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, command is error, valuestring:%s!\r\n",  
				__FILE__, __FUNCTION__, __LINE__, pSub->valuestring);  
			return -1;
		}
	}

    // get "id" from json
    pSub = cJSON_GetObjectItem(pJson, "id");
    if(NULL == pSub)
    {
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"id\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return -1;
    }
	else
	{
		if (pSub->valueint != *((stMonitorSrc*)pSrc)->pIdSeq)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, command is error, valueint:%d != %d!\r\n",  
				__FILE__, __FUNCTION__, __LINE__, pSub->valueint, *((stMonitorSrc*)pSrc)->pIdSeq);
		}
	}*/

    // get bool from json
    cJSON * pSub = cJSON_GetObjectItem((cJSON *)pJson, "result");
    if(NULL == pSub)
    {
        // get "result" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"result\" is error!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);
		return -1;
    }
	else
	{
		DEBUGMSG(0,("1 -------------\n"));
		DEBUGMSG(0,("type :%d, result : %d\n", pSub->type, pSub->valueint));
		if (FALSE == pSub->valueint)
		{
			rtn = 1;
		}
	}
	DEBUGMSG(0,("2 -------------\n"));
	pSub = cJSON_GetObjectItem((cJSON *)pJson, "error");

	if(NULL == pSub)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"error\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return -1;		
	}
	else
	{
		DEBUGMSG(0,("2 -------------, type:%d, pSub->string:%s\n", pSub->type, pSub->string));

		if (pSub->type != 2)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, command is error, type:[%d]!\r\n",  
				__FILE__, __FUNCTION__, __LINE__, pSub->type);
		}
		
		//strncpy(valBuf, pSub->valuestring, sizeof(valBuf));
		//iParamLen = strlen(valBuf);

		//DEBUGMSG(1,("iParamLen : %d\n", iParamLen));
		//DEBUGMSG(1,("valBuf:%s, valuestring : %s\n", valBuf, pSub->valuestring));
		/*if (4 != iParamLen || strncasecmp(valBuf, "null", 4) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, command is error, error:[%s]!\r\n",  
				__FILE__, __FUNCTION__, __LINE__, pSub->valuestring);
		}*/
	}

    //cJSON_Delete((cJSON *)pJson);

	return rtn;
}

int ParseConn(char *pMsg, void *pSrc, int *pIndex)
{
	int rtn = -1;
    cJSON * pJson = NULL, * pSub = NULL;

	DEBUGMSG(1,("%s...\n", __FUNCTION__));
	
	pJson = cJSON_Parse(pMsg);
	if (NULL == pJson)                                                                                         
	{
		// parse faild, return
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, pJson is NULL!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);	      
		//return E_COMMAND_ERROR_JSON;
	}
	else
	{
		pSub = cJSON_GetObjectItem(pJson, "conn");
		if(NULL == pSub)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, \"conn\" is error!\r\n",  
				__FILE__, __FUNCTION__, __LINE__);   
			//return E_COMMAND_ERROR_JSON;
			rtn = 1;
		}
		else
		{
		
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, {\"conn\":%d}\r\n",  
				__FILE__, __FUNCTION__, __LINE__, pSub->valueint);
			rtn = 0;
		}
	}

	return rtn;
}
	
/****************************************************************
** Function name:		ParseRecv
** Descriptions:	

** input parameters:			无
** output parameters:		无
** Returned value:			1:id错误
							2:switchid错误


****************************************************************/
int ParseRecv
(char *pMsg, void *pSrc, int *pIndex, struct ap_net_connection_t *pConn)
{
	int rtn = 0;
	int iParamLen = 0, iMethodLen = 0;
	//int iSize = 0, iCnt = 0;
	int i = 0, iSize = -1;
	//int iValue = 0;
	char dest[BUFFER_LEN_4096] = {0};
	char delim[] = " ";
	int PackArgc = 0;
	char *PackArgv[BUFFER_LEN_256] = {0};
	//int iSwiLen = 0, iPubLen = 0;

	DEBUGMSG(1,("pMsg:%s\n", pMsg));
	
	cJSON * pJson = cJSON_Parse(pMsg);
    //cJSON * root = NULL;

	if (NULL == pJson)                                                                                         
	{
		// parse faild, return
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, pJson is NULL!\r\n",  __FILE__, __FUNCTION__, __LINE__);	      
		return E_COMMAND_ERROR_JSON;
	}

	cJSON * pSub = NULL;
    // get "id" from json
    /*cJSON * pSub = cJSON_GetObjectItem(pJson, "id");
    if(NULL == pSub)
    {
    	// get "id" from json faild
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"id\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;
    }
	else
	{
		if (pSub->valueint != *((stMonitorSrc*)pSrc)->pIdSeq)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, command is error, id:%d != %d!\r\n",  
				__FILE__, __FUNCTION__, __LINE__, pSub->valueint, *((stMonitorSrc*)pSrc)->pIdSeq);
			return 1;
		}
	}*/

    /*pSub = cJSON_GetObjectItem(pJson, "switchid");
    if(NULL == pSub)
    {
        // get "switchid" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"switchid\" is error!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);
		return E_COMMAND_ERROR_JSON;
    }
	else
	{
		//printf("switchid : %d\n", pSub->valueint);
		//if (((stMonitorSrc*)pSrc)->pSwiDbInfo->DbSwi.iSwiId != pSub->valueint)
		iSwiLen = strlen((char *)((stMonitorSrc*)pSrc)->pSwiDbInfo->DbSwi.szSwiId);
		iPubLen = strlen(pSub->valuestring);
		if (iSwiLen == iPubLen &&
			strncmp((char *)((stMonitorSrc*)pSrc)->pSwiDbInfo->DbSwi.szSwiId, pSub->valuestring, iSwiLen))
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, command is error, switchid:%d != %d!\r\n",  
				__FILE__, __FUNCTION__, __LINE__, pSub->valueint, (char *)((stMonitorSrc*)pSrc)->pSwiDbInfo->DbSwi.szSwiId);			
			return E_COMMAND_ERROR_JSON;
		}
	}*/

    // get "method" from json
    pSub = cJSON_GetObjectItem(pJson, "method");
    if(NULL == pSub)
    {
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"method\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);   
		return E_COMMAND_ERROR_JSON;
    }
	else
	{
		iParamLen = strlen(pSub->valuestring);

		for (i = 0; i < ((stMonitorSrc*)pSrc)->iCommLen; i++)
		{
			iMethodLen = strlen((char *)((stMonitorSrc*)pSrc)->pMonitorComm[i].cmd);
			if (iMethodLen == iParamLen && 
				strncasecmp((char *)((stMonitorSrc*)pSrc)->pMonitorComm[i].cmd, pSub->valuestring, iMethodLen) == 0)
			{	
				*pIndex = i;
				break;
			}
		}
		DEBUGMSG(1,("valuestring:%s\n", pSub->valuestring));
		//没有对应的命令
		DEBUGMSG(0,("i:%d, iCommLen:%d\n", i, ((stMonitorSrc*)pSrc)->iCommLen));
		if (i == ((stMonitorSrc*)pSrc)->iCommLen)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, \"method\" param is error!(%s)\r\n",	
				__FILE__, __FUNCTION__, __LINE__,pMsg);

			return E_COMMAND_ERROR_JSON;
		}
		//return 0;
	}

	//DEBUGMSG(0,("*pIndex:%d < iCommLen:%d\n", *pIndex, ((stMonitorSrc*)pSrc)->iCommLen));
	//解析出dest为[1,2,...]或者all

	rtn = ((stMonitorSrc*)pSrc)->pMonitorComm[*pIndex].UserCmdFunc(pJson, dest, sizeof(dest), &iSize);
	DEBUGMSG(1,("rtn:%d, iSize:%d\n", rtn, iSize));
	//去掉cell.setdefaultpool，cell.search，refused  这些命令都是NoPackMonitor
	if (*pIndex < MONITOR_REPLY_COMM) 
		//&& iSize >= 0)
	{
		DEBUGMSG(1,("1 *pIndex:%d, dest:%s\n", *pIndex, dest));
		if (iSize >= 0)
		{
			if (Parse(dest, &PackArgc, PackArgv, delim) != 0)
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
				return (rtn = 3);
			}
			else
			{
				//向command传输数据
				//PackCellRebootMonitorToComm
				DEBUGMSG(1,("2 *pIndex:%d, PackArgc:%d\n", *pIndex, PackArgc));
				//cell.reboot/cell.setled/cell.setpool
				if (*pIndex > 1 && *pIndex < 5)
				{		
					PackConfirm(pSrc, *pIndex, pConn);

					//只有下发command才开启
					//临时处理
					if (!gCommandType)
						gCommandType = E_COMM_TYPE_MONITOR;
					
					//开启标志位
					CommMonitorFlag.bServerCommFlag = TRUE;
					
				
					//清除command接受返回过来的错误id
					//memset(CommMonitorFlag.szIdbufRtn, 0, sizeof(CommMonitorFlag.szIdbufRtn));
					//CommMonitorFlag.iBufPos = 0;
					rtn = ((stMonitorSrc*)pSrc)->pMonitorComm[*pIndex].CommPackFunc(
							PackArgc, PackArgv, 
							(void *)((stMonitorSrc*)pSrc)->szParseDest, 
							sizeof(((stMonitorSrc*)pSrc)->szParseDest),
							&ParData);
					//rtn = PackCellSetLedMonitorToComm(PackArgc, PackArgv, (void *)((stMonitorSrc*)pSrc)->szParseDest, sizeof(((stMonitorSrc*)pSrc)->szParseDest));

					//把错误id返回,而不是打印输出到LOG
					DEBUGMSG(1,("PACK rtn:%d\n", rtn));
					if (rtn == E_COMMAND_ERROR_ID || rtn == E_COMMAND_ERROR_PARTID)
						rtn = 0;
					//0:正常、3:id错误;其他错误打印到switch_debug.log文件里面.
					if (rtn != 0)
					{
						GetLocalTimeLog();
						ap_log_debug_log("\t\t%s, %s, %d, [%d]:%s\r\n", 
							__FILE__, __FUNCTION__, __LINE__, rtn - 1, (char *)gCommandErr[rtn - 1].cmd);
					}
				}
				DEBUGMSG(1,("dest:%s\n", dest));
				
			}
		}
	}
	else
	{
		DEBUGMSG(0,("2 *pIndex:%d\n", *pIndex));
	}

    cJSON_Delete(pJson);

	return rtn;
}

char * makeJson()
{
    cJSON * pJsonRoot = NULL;

    pJsonRoot = cJSON_CreateObject();
    if(NULL == pJsonRoot)
    {
        //error happend here
        return NULL;
    }
    /*cJSON_AddStringToObject(pJsonRoot, "hello", "hello world");
    cJSON_AddNumberToObject(pJsonRoot, "number", 10010);
    cJSON_AddBoolToObject(pJsonRoot, "bool", 1);*/

    cJSON_AddStringToObject(pJsonRoot, "method", "switch.validator");
    cJSON_AddNumberToObject(pJsonRoot, "id", 1);
	cJSON_AddStringToObject(pJsonRoot, "sign", "f561aaf6ef0bf14d4208bb46a4ccb3ad");
	cJSON_AddStringToObject(pJsonRoot, "switchid", "2");
	cJSON_AddStringToObject(pJsonRoot, "timestamp", "1427344719");



	
    cJSON * pSubJson = NULL;
    pSubJson = cJSON_CreateObject();
    if(NULL == pSubJson)
    {
        // create object faild, exit
        cJSON_Delete(pJsonRoot);
        return NULL;
    }
    //cJSON_AddStringToObject(pSubJson, "subjsonobj", "a sub json string");
    //cJSON_AddItemToObject(pJsonRoot, "subobj", pSubJson);

	//printf("string:%s\n", pJsonRoot->string);
	//printf("valuestring:%s\n", pJsonRoot->valuestring);

    char * p = cJSON_Print(pJsonRoot);

	//printf("%s\n", p);
  // else use : 
    // char * p = cJSON_PrintUnformatted(pJsonRoot);
    if(NULL == p)
    {
        //convert json list to string faild, exit
        //because sub json pSubJson han been add to pJsonRoot, so just delete pJsonRoot, if you also delete pSubJson, it will coredump, and error is : double free
        cJSON_Delete(pJsonRoot);
        return NULL;
    }
    //free(p);
    
    cJSON_Delete(pJsonRoot);

    return p;
}

//解析CJSON数组
void ParseArray(char * pJson)
{
	int iCnt = 0;

    if(NULL == pJson)
    {                                                                                                
        return ;
    }
    cJSON * root = NULL;
    if((root = cJSON_Parse(pJson)) == NULL)
    {
        return ;
    }
    int iSize = cJSON_GetArraySize(root);
    for(iCnt = 0; iCnt < iSize; iCnt++)
    {
        cJSON * pSub = cJSON_GetArrayItem(root, iCnt);
        if(NULL == pSub)
        {
            continue;
        }
        int iValue = pSub->valueint;
        printf("value[%2d] : [%d]\n", iCnt, iValue);
    }   
    cJSON_Delete(root);
    return;
}


void parseJson(char * pMsg)
{
	int Len = 0, i = 0;

    if(NULL == pMsg)
    {
        return;
    }
	//printf("pMsg:%s\n", pMsg);

	Len = strlen(pMsg);
	
	for (i = 0; i < Len; i++)
	{
		//printf("%02x ", pMsg[i]);
	
	}
	printf("\n");	
	
	
    cJSON * pJson = cJSON_Parse(pMsg);
    if(NULL == pJson)                                                                                         
    {
        // parse faild, return
      return ;
    }

    // get string from json
    cJSON * pSub = cJSON_GetObjectItem(pJson, "method");
    if(NULL == pSub)
    {
        //get object named "hello" faild
    }
    printf("obj_1 : %s\n", pSub->valuestring);

    // get number from json
    pSub = cJSON_GetObjectItem(pJson, "id");
    if(NULL == pSub)
    {
        //get number from json faild
    }
    printf("obj_2 : %d\n", pSub->valueint);

    // get bool from json
    pSub = cJSON_GetObjectItem(pJson, "sign");
    if(NULL == pSub)
    {
        // get bool from json faild
    }                                                                                                         
    printf("obj_3 : %s\n", pSub->valuestring);

	pSub = cJSON_GetObjectItem(pJson, "switchid");
	if(NULL == pSub)
	{
		// 
	}																										   
	printf("obj_4 : %s\n", pSub->valuestring);

	pSub = cJSON_GetObjectItem(pJson, "timestamp");
	if(NULL == pSub)
	{
		// 
	}																											
	printf("obj_5 : %s\n", pSub->valuestring);


 // get sub object
    /*pSub = cJSON_GetObjectItem(pJson, "subobj");
    if(NULL == pSub)
    {
        // get sub object faild
    }
    cJSON * pSubSub = cJSON_GetObjectItem(pSub, "subjsonobj");
    if(NULL == pSubSub)
    {
        // get object from subject object faild
    }
    printf("sub_obj_1 : %s\n", pSubSub->valuestring);*/

    cJSON_Delete(pJson);
}

/*int main()
{
    char * p = makeJson();
    if(NULL == p)
    {
        return 0;
    }
    //printf("%s\n", p);
    parseJson(p);                                                                                             

    return 0;
}*/

void TCPClientDestroy()
{
	stApiNetShell.ConnectionPoolDestroy(tcp_client,1);
}

int ClientCreated(void *conn)
{
	int rtn = 0;

	DEBUGMSG(1,("%s...\n", __FUNCTION__));
	
	if(((struct ap_net_connection_t *)conn)->user_data == NULL)
	{
	    ((struct ap_net_connection_t *)conn)->user_data = &gCliUserData;
	
		if (((struct ap_net_connection_t *)conn)->user_data == NULL)
	    {
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, * !create user_data error!\r\n", __FILE__, __FUNCTION__, __LINE__);
	        rtn = 1;
	    }
	}

	if (((struct ap_net_connection_t *)conn)->sendBuf == NULL)
	{
		((struct ap_net_connection_t *)conn)->sendBuf = &gCliSendDaTa;	
		
		if (((struct ap_net_connection_t *)conn)->sendBuf == NULL)
	    {
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, * !create sendBuf error!\r\n", __FILE__, __FUNCTION__, __LINE__);			
	        rtn = 2;
	    }
	}

	return rtn;
}

int ClientDataIn(struct ap_net_connection_t *conn)
{
	int rtnRecv = 0;
	int iIndex = 0;

	//conn 只接受一次
	if (!bConnOK)
	{
		//if (ParseConn((char *)conn->recebuf, (void *)&MoniSrc, &iIndex) == 0);
		//	bConnOK = TRUE;
		ParseConn((char *)conn->recebuf, (void *)&MoniSrc, &iIndex);
		bConnOK = TRUE;
	}
	else
	{
		/*strncpy(receBufTemp, (char *)conn->recebuf, sizeof(receBufTemp));
		//发送确认信息
		rtnConf = PackConfirm(
			receBufTemp, 
			(char *)((struct ap_net_connection_t *)conn)->sendBuf, 
			strlen((char *)((struct ap_net_connection_t *)conn)->sendBuf), 
			iSendRtn);
			((void *)&MoniSrc, ((struct ap_net_connection_t *)conn)->sendBuf, sizeof(gCliSendDaTa.bySeCmdBuf),	iIndex)
				
		*/
		//目前 由于cell设备重启比较慢，最多延迟15秒
		DEBUGMSG(1,("--------------------recebuf:%s, iIndex:%d\n", (char *)conn->recebuf, iIndex));
		//清空Monitor解析后数据缓冲
		memset(MoniSrc.szParseDest, 0, sizeof(MoniSrc.szParseDest));
		//清空command层返回错误数据缓冲
		memset((char *)CommMonitorFlag.szIdbufRtn, 0, sizeof(CommMonitorFlag.szIdbufRtn));
		CommMonitorFlag.iBufPos = 0;
		//清空发送缓冲
		memset((char *)((struct ap_net_connection_t *)conn)->sendBuf, 0, sizeof(gCliSendDaTa.bySeCmdBuf));
		//清空Part id
		memset(&ParData, 0, sizeof(stPartData));
	
		rtnRecv = ParseRecv((char *)conn->recebuf, (void *)&MoniSrc, &iIndex, (struct ap_net_connection_t *)conn);
		DEBUGMSG(1,("rtnRecv:%d\n", rtnRecv));
	
		if (rtnRecv == 0)
		{
			DEBUGMSG(1,("sendBuf:%s\n", (char *)((struct ap_net_connection_t *)conn)->sendBuf));	
			switch(iIndex)
			{
			case E_MONITOR_COMMAND_TYPE_VALIDATOR:
				//recv switch.validator 转到cell.info
				*MoniSrc.pSendInterval = SEND_CELLINFO_INTERVAL_TIME;
				break;
		
			case E_MONITOR_COMMAND_TYPE_CELLINFO:
				//recv cell.info转到switch.validator
				*MoniSrc.pSendInterval = SEND_SWITCHDATA_INTERVAL_TIME;
				break;
	
			case E_MONITOR_COMMAND_TYPE_REBOOT:
			case E_MONITOR_COMMAND_TYPE_SETLED:
			case E_MONITOR_COMMAND_TYPE_SET_POOL:
				if (PackReplay((void *)&MoniSrc, ((struct ap_net_connection_t *)conn)->sendBuf, sizeof(gCliSendDaTa.bySeCmdBuf),  iIndex)!= 0)
				{
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, ERROR: PackReplay\r\n", 
						__FILE__, __FUNCTION__, __LINE__);
				}
				else
				{
					//iSendRtn = 
					stApiNetMonitor.ConnectionPoolSend(
						conn->parent, conn->idx, 
						(char *)((struct ap_net_connection_t *)conn)->sendBuf, 
						strlen((char *)((struct ap_net_connection_t *)conn)->sendBuf)); 					
					//DEBUGMSG(1,("1 iSendRtn:%d\n", iSendRtn));
				}
				break;
				
			case E_MONITOR_COMMAND_TYPE_SET_DEFAULTPOOL:
			//case E_MONITOR_COMMAND_TYPE_SET_POOL:
			case E_MONITOR_COMMAND_TYPE_CELL_SEARCH:
				if (PackOK((void *)&MoniSrc, ((struct ap_net_connection_t *)conn)->sendBuf, 
					sizeof(gCliSendDaTa.bySeCmdBuf),  iIndex, rtnRecv)!= 0)
				{
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, ERROR: PackOK\r\n", 
						__FILE__, __FUNCTION__, __LINE__);
				}
				else
				{
					//iSendRtn = 
					stApiNetMonitor.ConnectionPoolSend(
						conn->parent, conn->idx, 
						(char *)((struct ap_net_connection_t *)conn)->sendBuf, 
						strlen((char *)((struct ap_net_connection_t *)conn)->sendBuf)); 					
					//DEBUGMSG(0,("3 Error iSendRtn:%d\n", iSendRtn));
				}	
				break;
	
			default:
				break;
			}
		}
		else
		{
			if (PackError((void *)&MoniSrc, ((struct ap_net_connection_t *)conn)->sendBuf, 
				sizeof(gCliSendDaTa.bySeCmdBuf),  iIndex, rtnRecv)!= 0)
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, ERROR: PackError\r\n", 
					__FILE__, __FUNCTION__, __LINE__);
			}
			else
			{
				//iSendRtn = 
				stApiNetMonitor.ConnectionPoolSend(
					conn->parent, conn->idx, 
					(char *)((struct ap_net_connection_t *)conn)->sendBuf, 
					strlen((char *)((struct ap_net_connection_t *)conn)->sendBuf)); 					
				//DEBUGMSG(0,("2 Error iSendRtn:%d\n", iSendRtn));
			}				
		}
	}
	//清空接受缓冲
	memset((char *)conn->recebuf, 0, sizeof(conn->recebuf));
	
	if (gCommandType == E_COMM_TYPE_MONITOR)
		gCommandType = 0;	

	return 0;
}

int ClientClose(void *conn)
{
	int rtn = 0;

	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	memset(((struct ap_net_connection_t *)conn)->recebuf, 0,  ((struct ap_net_connection_t *)conn)->bufsize);
	
	return rtn;
}


int client_callback(struct ap_net_connection_t *conn, int signal_type)
{
	//int rtnRecv = 0;
	//int iIndex = 0;
	//int iLen = 0;// iCommLen = 0;
	//int iSendRtn = 0;
	//int rtnConf = 0;
	//iLen = sizeof(gCliSendDaTa.bySeCmdBuf);

	DEBUGMSG(1,("%s..., signal_type:%d\n", __FUNCTION__, signal_type));
    switch (signal_type)
    {
        case AP_NET_SIGNAL_CONN_CREATED:
			ClientCreated(conn);
            break;
			
		case AP_NET_SIGNAL_CONN_CLOSING:
			ClientClose(conn);
			break;	

        case AP_NET_SIGNAL_CONN_TIMED_OUT:
			//conn->state = AP_NET_ST_RECONNECTION;
            break;
			
        case AP_NET_SIGNAL_CONN_DESTROYING:
            free(conn->user_data);
            break;

        case AP_NET_SIGNAL_CONN_CONNECTED:
            //ud->state = STATE_CONNECTED;
            break;

        case AP_NET_SIGNAL_CONN_ACCEPTED:
            break;

        case AP_NET_SIGNAL_CONN_MOVED_FROM:
        case AP_NET_SIGNAL_CONN_MOVED_TO:
            break;

        case AP_NET_SIGNAL_CONN_DATA_IN:
        case AP_NET_SIGNAL_CONN_DATA_LEFT:
			ClientDataIn(conn);			
			break;

        case AP_NET_SIGNAL_CONN_CAN_SEND:

            break;

        default:
            break;
    }

    return 1;
}

int SwitchPro(struct ap_net_connection_t *conn)
{
	int iSendLen = 0;

	if (PackSwitchvalidator((void *)&MoniSrc, ((struct ap_net_connection_t *)conn)->sendBuf, sizeof(gCliSendDaTa.bySeCmdBuf), &iSendLen) != 0)
	{
		return -1;
	}
	else
	{
		if (*MoniSrc.pSendInterval == SEND_INIT_INTERVAL_TIME)
		{
			*MoniSrc.pSendInterval = SEND_SWITCHDATA_INTERVAL_TIME;
		}
	}
	stApiNetMonitor.ConnectionPoolSend(
		conn->parent, conn->idx, 
		(char *)((struct ap_net_connection_t *)conn)->sendBuf, 
		iSendLen);
	
	memset((char *)((stClientSendData *)conn->sendBuf)->bySeCmdBuf, 0, iSendLen);
	
	return 0;
}

void CellInfoPro(struct ap_net_connection_t *conn)
{
	int rtnCmd = 0;
	int iSendLen = 0;
	BOOL bSendOne = TRUE;

	do{
		DEBUGMSG(0,("2 %s...\r\n", __FUNCTION__));
		
		rtnCmd = PackCellAllInfo((void *)&MoniSrc, ((struct ap_net_connection_t *)conn)->sendBuf, sizeof(gCliSendDaTa.bySeCmdBuf), &iSendLen);
		if (rtnCmd == 0 && iSendLen != 0)
		{
			stApiNetMonitor.ConnectionPoolSend(
				conn->parent, conn->idx, 
				(char *)((struct ap_net_connection_t *)conn)->sendBuf, 
				iSendLen);
			if (bSendOne)
			{
				DEBUGMSG(1,("%s\n", (char *)((struct ap_net_connection_t *)conn)->sendBuf));
				bSendOne = FALSE;
			}			
		}
		
		memset((char *)((stClientSendData *)conn->sendBuf)->bySeCmdBuf, 0, iSendLen);
		iSendLen = 0;
	}while(++((stClientSendData *)conn->sendBuf)->iSubindex < ((stClientSendData *)conn->sendBuf)->iNumPack);

	memset((char *)(stClientSendData *)conn->sendBuf, 0, sizeof(stClientSendData));
}

void *MonitorProcess(void *arg)
{
    struct ap_net_connection_t *conn = NULL;
    long lSendStartTime = 0, lSendEndTime = 0, lReConnStartTime = 0, lReConnEndTime = 0;
	long lSendNewCellStartTime = 0;
	//int iSendRtn = 0;
	//BOOL bOneTime = TRUE;
	//int iLen = 0;//, iCommLen = 0;
	//int iIndex = 0;
	int recvRtn = 0;
	//BOOL bFirConn = FALSE;
	//int rtnCmd = 0;
	//int iSendLen = 0;
	
	DEBUGMSG(1,("8 %s...\r\n", __FUNCTION__));

	MSleep(2000);

	CreateTCPClient();

	conn = tcp_client->conns;
	
	//iLen = sizeof(gCliSendDaTa.bySeCmdBuf);
	//iCommLen = dim(MonitorComm);

	/*else
	{
		//bFirConn = TRUE;
	}*/
	/*else
	{
		memset((char *)((struct ap_net_connection_t *)conn)->sendBuf, 0, iLen);
		if (PackSwitchvalidator((void *)&MoniSrc, ((struct ap_net_connection_t *)conn)->sendBuf, iLen) == 0)
		{
			iSendRtn = stApiNetMonitor.ConnectionPoolSend(conn->parent, conn->idx, (char *)((struct ap_net_connection_t *)conn)->sendBuf, iLen);
		}
	}*/

	lReConnEndTime = lReConnStartTime = lSendEndTime = lSendStartTime = lSendNewCellStartTime = GetTime();

	DEBUGMSG(1,("3 tcp_client->conns->fd:%d, state:%d, conns->state:%d\n", tcp_client->conns->fd, tcp_client->state, tcp_client->conns->state));

	while(1)
    {
		//MSleep(2000);
		MSleep(500);
		//creating connection for debug output 
		if (lReConnEndTime - lReConnStartTime >= RECONN_INTERVAL_TIME )
		{
			DEBUGMSG(0,("4 tcp_client->conns->fd:%d, state:%d, conns->state:%d\n", conn->fd, tcp_client->state, conn->state));
			//AP_NET_ST_DISCONNECTION + AP_NET_ST_CONNECTED
			if (conn->state == AP_NET_ST_DISCONNECTION + AP_NET_ST_CONNECTED)			
				stApiNetShell.ConnectionPoolClose(conn->parent, conn->idx);

			if (conn->state == 0)
			{
				//连接
				if (NULL == stApiNetMonitor.ConnectionPoolConnect(tcp_client, 0, (char *)gCfgDataDL.MonitorInfo.szMoniIp, AF_INET, gCfgDataDL.MonitorInfo.iMoniPort, 0))
				{
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, ERROR: Client debug dataserver connect.: %s\r\n",	
						__FILE__, __FUNCTION__, __LINE__, ap_error_get_string());
					//exit(1);
				}
				lReConnStartTime = lReConnEndTime;
			}
		}
		else
		{
			lReConnEndTime = GetTime();
			DEBUGMSG(0,("2 lReConnEndTime:%ld\n", lReConnEndTime));
		}
		DEBUGMSG(0,("lReConnEndTime:%ld, lReConnStartTime:%ld, ReConn interval:%ld\n", 
			lReConnEndTime, lReConnStartTime, lReConnEndTime - lReConnStartTime));

		DEBUGMSG(0,("5 tcp_client->conns->fd:%d, state:%d, conns->state:%d\n", conn->fd, tcp_client->state, conn->state));
		DEBUGMSG(0,("used_slots:%d, pSendInterval:%d\n", tcp_client->used_slots, *MoniSrc.pSendInterval));

		if (conn->state == AP_NET_ST_CONNECTED)
		{
			if (lSendEndTime - lSendStartTime >= *MoniSrc.pSendInterval)
			//if (lSendEndTime - lSendStartTime >= 10)	//调试用
			{
				DEBUGMSG(1,("&&&&&&&&&&&&&&, parent->state:%d, *MoniSrc.pSendInterval:%d\n", conn->parent->state, *MoniSrc.pSendInterval));
				//if (conn->state == AP_NET_ST_CONNECTED)
				//{	
				//发送
				memset((char *)((struct ap_net_connection_t *)conn)->sendBuf, 0, sizeof(gCliSendDaTa.bySeCmdBuf));
				if (*MoniSrc.pSendInterval == SEND_SWITCHDATA_INTERVAL_TIME || *MoniSrc.pSendInterval == SEND_INIT_INTERVAL_TIME)
				{
					if (SwitchPro(conn) != 0)
						continue;
				}
				else if (*MoniSrc.pSendInterval == SEND_CELLINFO_INTERVAL_TIME)	// || *MoniSrc.pSendInterval == SEND_INIT_INTERVAL_TIME
				{
					CellInfoPro(conn);

					/*if (rtnCmd == 0)
					{
						if (*MoniSrc.pSendInterval == SEND_INIT_INTERVAL_TIME)
						{
							*MoniSrc.pSendInterval = SEND_CELLINFO_INTERVAL_TIME;
						}
					}*/
				}
				/*else if (*MoniSrc.pSendInterval == SEND_INIT_INTERVAL_TIME)
				{

				}*/
				//iSendRtn = 
				//DEBUGMSG(0,("iSendRtn:%d\n", iSendRtn));
				DEBUGMSG(0,("<<<<<<state:%d\n", conn->state));
				
				lSendStartTime = lSendEndTime;
				MSleep(100);
				//}
			}
			else
			{
				if (*MoniSrc.pSendInterval == SEND_CELLINFO_INTERVAL_TIME)
				{
					//MSleep(10000);
					//printf("lSendEndTime - lSendNewCellStartTime:%ld\n", lSendEndTime - lSendNewCellStartTime);
					if (lSendEndTime - lSendNewCellStartTime  >= SEND_NEWCELLINFO_INTERVAL_TIME)// && !bLoginQueValue)
					{
						while(1)
						{
							if (PackCellNewInfo((void *)&MoniSrc, ((struct ap_net_connection_t *)conn)->sendBuf, sizeof(gCliSendDaTa.bySeCmdBuf)) == 0)
							{
								stApiNetMonitor.ConnectionPoolSend(
									conn->parent, conn->idx, 
									(char *)((struct ap_net_connection_t *)conn)->sendBuf, 
									strlen((char *)((struct ap_net_connection_t *)conn)->sendBuf));
							}
							else
							{
								break;
							}
							memset((char *)((stClientSendData *)conn->sendBuf)->bySeCmdBuf, 0, sizeof(gCliSendDaTa.bySeCmdBuf));
						}
					
						lSendNewCellStartTime = GetTime();
					}
				}
				lSendEndTime = GetTime();
			}

			//接受
			recvRtn = stApiNetMonitor.ConnectionPoolPoll(tcp_client);
			DEBUGMSG(0,("                recvRtn:%d\n", recvRtn));
			if (recvRtn == 0)
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, ERROR: Client tcp_pool:%s\r\n",	
					__FILE__, __FUNCTION__, __LINE__, ap_error_get_string());			
				//exit(1);
			}
		}
		//MSleep(4000);
	}
}


int dbgdumpGetPooSpecifyConf()
{
	int rtn = 0;
	int iPos = 0;//, iCount = 0;
	int i = 0;
	char dest[BUFFER_LEN_2048] = {0};
	int destLen = sizeof(dest);

	if (gPoolSpecifyConf.iBTCPoolCount == 0 && gPoolSpecifyConf.iLTCPoolCount == 0)
	{
		snprintf(dest + iPos, destLen, "\r\n######################################\r\nPoolPri:NULL\r\n");
	}
	else
	{
		snprintf(dest + iPos, destLen, 
			"\r\n######################################\r\nPoolPri:%s\r\n-------------------PoolSpecifyConfStart---------------------\r\nBTCFixedPoolCount:%d\r\n", 
			(char *)gpSwitchDbInfo->DbSwi.szPoolIdPri,
			gPoolSpecifyConf.iBTCPoolCount);
		iPos += strlen(dest+iPos);

		
		
		for (i = 0; i < gPoolSpecifyConf.iBTCPoolCount && gPoolSpecifyConf.iBTCPoolCount <=3; i++)
		{
			DEBUGMSG(0,("222 stBTCPoolConf@%p, iPoolId[%d]:%d\r\n", gPoolSpecifyConf.stBTCPoolConf, i, gPoolSpecifyConf.stBTCPoolConf[i].iPoolId));
			snprintf(dest + iPos, destLen, 
				"\tid:%d\r\n\t\turl:'%s'\r\n\t\tuser:'%s'\r\n\t\tpwd:'%s'\r\n", 
				gPoolSpecifyConf.stBTCPoolConf[i].iPoolId,
				gPoolSpecifyConf.stBTCPoolConf[i].szUrl, 
				gPoolSpecifyConf.stBTCPoolConf[i].szUser,
				gPoolSpecifyConf.stBTCPoolConf[i].szPasswd
				);
			iPos += strlen(dest+iPos);
		}

		snprintf(dest + iPos, destLen, "LTCFixedPoolCount:%d\r\n", 
			gPoolSpecifyConf.iLTCPoolCount);
		iPos += strlen(dest+iPos);

		for (i = 0; i < gPoolSpecifyConf.iLTCPoolCount && gPoolSpecifyConf.iLTCPoolCount <=3; i++)
		{
			DEBUGMSG(0,("222 stLTCPoolConf@%p, iPoolId[%d]:%d\r\n", gPoolSpecifyConf.stLTCPoolConf, i, gPoolSpecifyConf.stLTCPoolConf[i].iPoolId));
			snprintf(dest + iPos, destLen, 
				"\tid:%d\r\n\t\turl:'%s'\r\n\t\tuser:'%s'\r\n\t\tpwd:'%s'\r\n", 
				gPoolSpecifyConf.stLTCPoolConf[i].iPoolId,
				gPoolSpecifyConf.stLTCPoolConf[i].szUrl, 
				gPoolSpecifyConf.stLTCPoolConf[i].szUser,
				gPoolSpecifyConf.stLTCPoolConf[i].szPasswd
				);
			iPos += strlen(dest+iPos);
		}
		snprintf(dest + iPos, destLen, 
			"-------------------PoolSpecifyConfEnd-----------------------\r\n");
		
	}
	//iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);	

	DEBUGMSG(1,("dest:%s\n", dest));

	return rtn;
}

