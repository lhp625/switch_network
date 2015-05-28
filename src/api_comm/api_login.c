/***********************************************************************
* $Id$
* Project:	 switch_network  
* File:api_comm/api_commt.c		 
* Description: brief Part of login api. 
*
* @version:		V1.0.0
* @date:		20th August 2014
*
* Copyright (C) 2014 SFARDS Co., Ltd. All rights reserved.
*-----------------------------------------------------------------
*
* Revise:  
* 		 date		 version 		author 		description
*  		2014/08/20	 1.0.0		zhengchao	create MinerLogin and CellLogin
*
*
*
**********************************************************************/
#include "api_login.h"
#include "../app/debug.h"
#include "../ap_log.h"
#include "../download/download.h"
//#include "api_assign_resource.h"
#include "../db_op/db_mysql.h"
#include "def_devstr.h"
#include "../cJSON/cJSON.h"
//#include "./api_monitor.h"

stApiNetConnPool *gpLoginPool = NULL;
int gLoginPort = 30000;
stQueue *gpLoginQue = NULL;
long gLoginStartTime = 0;
BOOL gbLoginsendOK = FALSE, bStaTime = TRUE;
stApiNetPoll *gpPoller = NULL;

//pthread_mutex_t gLoginQueLock = PTHREAD_MUTEX_INITIALIZER;
/*pthread_mutex_t gMinerOnliLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCellOnliLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gMinerAlarmTimerLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCellAlarmTimerLock = PTHREAD_MUTEX_INITIALIZER;*/

pthread_mutex_t gLoginMonitorLock = PTHREAD_MUTEX_INITIALIZER;


stAlgo Algo[] = 
{
	{"BTC"},
	{"LTC"},
	{"DUAL"}
};

stLoginType loginTypeInput[] =
{
	//{"miner_login", ParseMinerLogin},
	{"cell_login", ParseCellLogin}
};

stLogin LoginComm[] = 
{
	{"login", ParseJsonCellLogin, PackJsonCellLogin},
};

const stLoginOutputHead LoginOutputHead[] =
{
	{"miner_login"},
	{"cell_login"}
};

const stLoginOutputError LoginOutputError[] =
{
	{"More than the number of the maximum  MINER"},
	{"More than the number of the maximum CELL"},
	{"More than the number of the maximum MinerAlgo"},
	{"More than the number of the maximum CellAlgo"},
	{"error ip_conflict"},
	{"error mac_conflict"},
	{"More than the number of the maximum BTC"},
	{"More than the number of the maximum LTC"},
	{"Get a error Json or command!"}
};

char szCellLoginParam[][BUFFER_LEN_16] = 
{
	{"sw_ver"},
	{"hw_ver"},
	{"algo"},
	{"chip_cnt"},
	{"btc_hash"},
	{"ltc_hash"},	
	{"btc_freq"},
	{"ltc_freq"},
	{"test_result"}
};

static int UserEqual(PTR pa, PTR pb)
{
  return 0 == strcmp(pa, pb);
}

#if 0
int PoolToFixediBuffer()
{
	int rtn = 0;
	structList *pList = NULL;
	list_node_t *pNode = NULL, *NodeBTCTemp = NULL, *NodeLTCTemp = NULL;
	int i = 0;//BTCIndex = 0, LTCIndex = 0;iCount = 0, 
	int iPoolIdTemp = 0;

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	pList = &stBTCPoolConfList;
	/*pthread_mutex_lock(pList->ListLenLock);
	iCount = pList->List->len;
	pthread_mutex_unlock(pList->ListLenLock);

	DEBUGMSG(1,("iCount:%d\r\n", iCount));*/

	DEBUGMSG(1,("iBTCPoolCount:%d\r\n", gPoolDefaultConf.iBTCPoolCount));
	pthread_mutex_lock(pList->RemoveLock);
	for (i = 0; i < gPoolDefaultConf.iBTCPoolCount; i++)
	{
		//查找BTC pool_id	
		iPoolIdTemp = gPoolDefaultConf.iBTCPoolIdPri[i];
		NodeBTCTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);

		if (NodeBTCTemp != NULL)
		{
			gPoolDefaultConf.stBTCPoolConf[i].iPoolId = ((stPoolConfInfo *)pNode->val)->iPoolId;
			strcpy((char *)gPoolDefaultConf.stBTCPoolConf[i].szUrl, (char *)((stPoolConfInfo *)pNode->val)->szUrl);
			strcpy((char *)gPoolDefaultConf.stBTCPoolConf[i].szUser, (char *)((stPoolConfInfo *)pNode->val)->szUser);
			strcpy((char *)gPoolDefaultConf.stBTCPoolConf[i].szPasswd, (char *)((stPoolConfInfo *)pNode->val)->szPasswd);
			gPoolDefaultConf.stBTCPoolConf[i].eAlgo = ((stPoolConfInfo *)pNode->val)->eAlgo;
		}
		else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, Cann't find NodeBTCTemp!(pool_id:%d)\r\n", 
				__FILE__, __FUNCTION__, __LINE__, iPoolIdTemp);			
		}
	}
	pthread_mutex_unlock(pList->RemoveLock);

	DEBUGMSG(1,("iLTCPoolCount:%d\r\n", gPoolDefaultConf.iLTCPoolCount));
	pList = &stLTCPoolConfList;
	pthread_mutex_lock(pList->RemoveLock);
	for (i = 0; i < gPoolDefaultConf.iLTCPoolCount; i++)
	{
		//查找LTC pool_id	
		iPoolIdTemp = gPoolDefaultConf.iLTCPoolIdPri[i];
		NodeLTCTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);

		if (NodeLTCTemp != NULL)
		{
			gPoolDefaultConf.stLTCPoolConf[i].iPoolId = ((stPoolConfInfo *)pNode->val)->iPoolId;
			strcpy((char *)gPoolDefaultConf.stLTCPoolConf[i].szUrl, (char *)((stPoolConfInfo *)pNode->val)->szUrl);
			strcpy((char *)gPoolDefaultConf.stLTCPoolConf[i].szUser, (char *)((stPoolConfInfo *)pNode->val)->szUser);
			strcpy((char *)gPoolDefaultConf.stLTCPoolConf[i].szPasswd, (char *)((stPoolConfInfo *)pNode->val)->szPasswd);
			gPoolDefaultConf.stLTCPoolConf[i].eAlgo = ((stPoolConfInfo *)pNode->val)->eAlgo;
		}
		else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, Cann't find NodeLTCTemp!(pool_id:%d)\r\n", 
				__FILE__, __FUNCTION__, __LINE__, iPoolIdTemp);			
		}
	}
	pthread_mutex_unlock(pList->RemoveLock);

	/*pthread_mutex_lock(pList->RemoveLock);
	for(i = 0;i < iCount;i++)
	{
		pNode = pList->ListAt(pList->List, i);
		DEBUGMSG(1,("eAlgo:%d\r\n", ((stPoolConfInfo *)pNode->val)->eAlgo));
		//if (((stPoolConfInfo *)pNode->val)->eAlgo == E_ALGO_BTC)
		{
			if (++BTCIndex > MAX_POOL_COUNT)
			{
				continue;
			}
			gBTCPoolConf[BTCIndex].iPoolId = ((stPoolConfInfo *)pNode->val)->iPoolId;
			strcpy((char *)gPoolDefaultConf.stBTCPoolConf[BTCIndex].szUrl, (char *)((stPoolConfInfo *)pNode->val)->szUrl);
			strcpy((char *)gPoolDefaultConf.stBTCPoolConf[BTCIndex].szUser, (char *)((stPoolConfInfo *)pNode->val)->szUser);
			strcpy((char *)gPoolDefaultConf.stBTCPoolConf[BTCIndex].szPasswd, (char *)((stPoolConfInfo *)pNode->val)->szPasswd);
			gPoolDefaultConf.stBTCPoolConf[BTCIndex][BTCIndex].eAlgo = ((stPoolConfInfo *)pNode->val)->eAlgo;
			gBTCPoolCount = BTCIndex;					
		}
		//else if (((stPoolConfInfo *)pNode->val)->eAlgo == E_ALGO_LTC)
		{
			if (++LTCIndex > MAX_POOL_COUNT)
			{
				continue;
			}
			gLTCPoolConf[LTCIndex].iPoolId = ((stPoolConfInfo *)pNode->val)->iPoolId;
			strcpy((char *)gLTCPoolConf[LTCIndex].szUrl, (char *)((stPoolConfInfo *)pNode->val)->szUrl);
			strcpy((char *)gLTCPoolConf[LTCIndex].szUser, (char *)((stPoolConfInfo *)pNode->val)->szUser);
			strcpy((char *)gLTCPoolConf[LTCIndex].szPasswd, (char *)((stPoolConfInfo *)pNode->val)->szPasswd);
			gLTCPoolConf[LTCIndex].eAlgo = ((stPoolConfInfo *)pNode->val)->eAlgo;
			gLTCPoolCount = LTCIndex;
		}
		else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, Algo error!\r\n",	__FILE__, __FUNCTION__, __LINE__);			
		}
	}
	pthread_mutex_unlock(pList->RemoveLock);*/

	return rtn;
}
#endif

stLoginSrc LoginSrc = {0}; 
UINT gLoginIdSeq = 1;                                 //全局变量，每发送一包序列号加1
CHAR LoginBuf[BUFFER_LEN_512];

void InitLogin()
{
	DEBUGMSG(1, ("%s...\r\n", __FUNCTION__));

	//初始化被删除的CellId结点
	stDelCellIdList.List = stDelCellIdList.ListNew();
	stDelCellIdList.List->match = UserEqual;

	gpLoginPool = stApiNetWork.ApiNetConnPoolCreate(API_NET_POOL_FLAGS_ASYNC, MAX_PER_CLIENT);
	if (gpLoginPool == NULL
		|| ! stApiNetWork.ApiNetSetIp4Addr(&gpLoginPool->listener.addr4, INADDR_ANY, gLoginPort)
		|| -1 == stApiNetWork.ApiNetConnPoolListenerCreate(gpLoginPool, 1, 1))
	{
		printf("* !ERROR: udp_pool: %s\r\n", ap_error_get_string());
		exit(1);
	}

	gpPoller = stApiNetWork.ApiNetPollCreate(MAX_PER_CLIENT);
	if (gpPoller == NULL)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, InitPoll error!\r\n", __FILE__, __FUNCTION__, __LINE__);	
		exit(1);
	}
	else
	{		
		stApiNetWork.ApiNetPollControl(gpLoginPool->listener.sock, gpPoller);
	}

	//gpLoginQue = LoginFifoQueue.QueCreate();
	gpLoginQue = CirFifoQueue.QueCreate(MAX_BUF_NUM, BUFFER_LEN_512);
	
	//LoginFifoQueue.QueLock = &gLoginQueLock;
	/*OnlineLock.pMinerOnliLock = &gMinerOnliLock;
	OnlineLock.pCellOnliLock = &gCellOnliLock;
	DevAlarmTimerLock.pMinerAlarmTimerLock = &gMinerAlarmTimerLock;
	DevAlarmTimerLock.pCellAlarmTimerLock = &gCellAlarmTimerLock;*/

	LoginSrc.pIdSeq = &gLoginIdSeq;
	LoginSrc.pBuf = LoginBuf;
	LoginSrc.pList = &stCellUtlist;
	LoginSrc.pLoginComm = LoginComm;
	LoginSrc.iCommLen = dim(LoginComm);
	memset(LoginSrc.szParseDest, 0, sizeof(LoginSrc.szParseDest));

	//设置组播
	SetClientIp4(&gpLoginPool->MulticastAddr4, gCfgDataDL.CellMulticast.szMulticastIp, gCfgDataDL.CellMulticast.iMulticastPort);	
}

/*void InitLoginQue()
{
	gpLoginQue = LoginFifoQueue.QueCreate();
	LoginFifoQueue.QueLock = &gLoginQueLock;
	DEBUGMSG(1,("1 &gpLoginQue:%p\r\n", gpLoginQue));
}*/
/*int QueueWrite(queue *que, char *buf)
{
	//char *message = malloc(BUFFER_LEN_256);
	char *mess = NULL;
	LoginFifoQueue.QueMalloc(&mess, BUFFER_LEN_256);
	DEBUGMSG(1,("%s..., %p, %s\r\n", __FUNCTION__, mess, mess));
	sprintf(mess, "%s", buf);
	
	if (LoginFifoQueue.QuePush(que, (void *)mess, LoginFifoQueue.QueLock) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, queue_push error!\r\n", __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}*/

/*int LoginQueWrite(queue *que, char *buf)
{
	//char *message = malloc(BUFFER_LEN_256);
	char *mess = NULL;
	LoginFifoQueue.QueMalloc(&mess, BUFFER_LEN_256);
	DEBUGMSG(0,("%s..., %p, %s\r\n", __FUNCTION__, mess, mess));
	sprintf(mess, "%s", buf);
	DEBUGMSG(0,("\r\n\r\n1 ******************************\r\n\r\n"));
	if (LoginFifoQueue.QuePush(que, (void *)mess, LoginFifoQueue.QueLock) != 0)
	{
		//ap_log_debug_log("\r\n\r\n2 ******************************\r\n\r\n");
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, queue_push error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}*/

int PackMinerError(void *srcBuf, char *destBuf, int *pLen, int iSize, int iIndex)
{
	DEBUGMSG(1, ("%s...\r\n", __FUNCTION__));
	
	*pLen = snprintf(destBuf, iSize, "%s %d %s", 
		(LoginOutputHead+0)->byLoginData, 
		((stMinerConn *)srcBuf)->MinerData.iMinerId,
#if 1
		(LoginOutputError+iIndex)->byLoginError
#endif
        );
		//((stMinerConn *)srcBuf)->MinerRes.WbIpPort.byWbIp, ((stMinerConn *)srcBuf)->pMinerRes->WbIpPort.byWbPort);
	
	return 0;
}

int PackCellError(void *srcBuf, void *destBuf, int *pLen, int iSize, int iIndex)
{
	*pLen = snprintf(destBuf, iSize, "%s %d %s", 
		(LoginOutputHead+1)->byLoginData, 
		((stCellConn *)srcBuf)->CellData.iCellId,
#if 1
		(LoginOutputError+iIndex)->byLoginError);
#endif
		//((stCellConn *)srcBuf)->CellRes.WbIpPort.byWbIp, ((stCellConn *)srcBuf)->CellRes.WbIpPort.byWbPort,
		//((stCellConn *)srcBuf)->byDividXnonce);
	
	return 0;
}

int PackMinerRepeat(void *srcBuf, char *destBuf, int *pLen, int iSize)
{
	switch(((stMinerConn *)srcBuf)->MinerData.eMinerAlgo)
	{
	case E_ALGO_BTC:
	case E_ALGO_LTC:
		*pLen = snprintf(destBuf, iSize, "%s %d %s %d", 
			(LoginOutputHead+0)->byLoginData, 
			((stMinerConn *)srcBuf)->MinerData.iMinerId, 
			"result",
			((stMinerConn *)srcBuf)->pMinerRes->iWbsId);
			//((stMinerConn *)srcBuf)->MinerRes.WbIpPort.byWbIp, ((stMinerConn *)srcBuf)->MinerRes.WbIpPort.byWbPort);
		break;

	case E_ALGO_DUAL:
		*pLen = snprintf(destBuf, iSize, "%s %d %s %d %d", 
			(LoginOutputHead+0)->byLoginData, 
			((stMinerConn *)srcBuf)->MinerData.iMinerId, 
			"result",
			((stMinerConn *)srcBuf)->pMinerRes[0].iWbsId,
			((stMinerConn *)srcBuf)->pMinerRes[1].iWbsId);
		break;

	default:
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Algorithm[%d] does not exist!\r\n", __FILE__, __FUNCTION__, __LINE__, ((stMinerConn *)srcBuf)->eClinetType);
		break;
	}
	
	return 0;
}

int PackCellRepeat(stCellConn *pCellConn, char *destBuf, int *pLen, int iSize)
{
	int i = 0, iPos = 0;//iCount = 0, 

	DEBUGMSG(1, ("%s...\r\n", __FUNCTION__));

	snprintf(destBuf + iPos, iSize, 
		"%s %d %s %s %u %u",
		(LoginOutputHead+1)->byLoginData, 
		pCellConn->CellData.iCellId,
		"result",
		pCellConn->CellData.szDevId,
		pCellConn->CellData.iChipBTCFreq,
		pCellConn->CellData.iChipLTCFreq);
	iPos += strlen(destBuf+iPos);

	DEBUGMSG(1,("111111 %s\r\n", destBuf));

	pthread_mutex_lock(&pCellConn->CellDataLock.PoolConfLock);
	for(i = 0;i < pCellConn->CellPoolConf.iBTCPoolCount && pCellConn->CellPoolConf.iBTCPoolCount <= MAX_POOL_COUNT;i++)
	{	
		snprintf(destBuf + iPos, iSize, 
			" %d/%s/%s/%s",
			pCellConn->CellPoolConf.stBTCPoolConf[i].eAlgo+1,
			pCellConn->CellPoolConf.stBTCPoolConf[i].szUrl,
			pCellConn->CellPoolConf.stBTCPoolConf[i].szUser,
			pCellConn->CellPoolConf.stBTCPoolConf[i].szPasswd
			);
		iPos += strlen(destBuf+iPos);
	}

	DEBUGMSG(1,("222222 %s\r\n", destBuf));
	
	for(i = 0;i < pCellConn->CellPoolConf.iLTCPoolCount && pCellConn->CellPoolConf.iLTCPoolCount <= MAX_POOL_COUNT;i++)
	{	
		snprintf(destBuf + iPos, iSize, 
			" %d/%s/%s/%s",
			pCellConn->CellPoolConf.stLTCPoolConf[i].eAlgo+1,
			pCellConn->CellPoolConf.stLTCPoolConf[i].szUrl,
			pCellConn->CellPoolConf.stLTCPoolConf[i].szUser,
			pCellConn->CellPoolConf.stLTCPoolConf[i].szPasswd
			);
		iPos += strlen(destBuf+iPos);
	}
	pthread_mutex_unlock(&pCellConn->CellDataLock.PoolConfLock);

	*pLen = iPos;
	DEBUGMSG(1,("iPos:%d\r\n", iPos));
	DEBUGMSG(1,("%s\r\n", destBuf));

	/*switch(((stCellConn *)srcBuf)->CellData.eCellAlgo)
	{
	case E_ALGO_BTC:
	case E_ALGO_LTC:
		*pLen = snprintf(destBuf, iSize, "%s %d %s %d %d %d", 
			(LoginOutputHead+1)->byLoginData,
			((stCellConn *)srcBuf)->CellData.iCellId,
			"result",
			((stCellConn *)srcBuf)->CellData.iChipBTCFreq,
			((stCellConn *)srcBuf)->pCellRes->iConnCellNum,
			((stCellConn *)srcBuf)->pCellRes->iWbcId);
			//((stCellConn *)srcBuf)->CellRes.WbIpPort.byWbIp, ((stCellConn *)srcBuf)->CellRes.WbIpPort.byWbPort,
			//((stCellConn *)srcBuf)->byDividXnonce);
		break;

	case E_ALGO_DUAL:
		*pLen = snprintf(destBuf, iSize, "%s %d %s %d %d %d %d", 
			(LoginOutputHead+1)->byLoginData,
			((stCellConn *)srcBuf)->CellData.iCellId,
			"result",
			((stCellConn *)srcBuf)->CellData.iChipBTCFreq,
			((stCellConn *)srcBuf)->pCellRes[0].iConnCellNum,
			((stCellConn *)srcBuf)->pCellRes[0].iWbcId,
			//((stCellConn *)srcBuf)->pCellRes[1].iConnCellNum,
			((stCellConn *)srcBuf)->pCellRes[1].iWbcId);
		break;
			
	default:
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Algorithm[%d] does not exist!\r\n", __FILE__, __FUNCTION__, __LINE__, ((stCellConn *)srcBuf)->eClinetType);
		break;
	}*/
	
	return 0;
}

/*int PackMinerData(stMinerConn *pMinerConn, char *destBuf, int *pLen, int iSize)
{
	//DEBUGMSG(0, ("%s..., iWbsId:%d\r\n", __FUNCTION__, pMinerConn->pMinerRes->iWbsId));
	switch(pMinerConn->MinerData.eMinerAlgo)
	{
	case E_ALGO_BTC:
	case E_ALGO_LTC:
		*pLen = snprintf(destBuf, iSize, "%s %d %s %d", 
			(LoginOutputHead+0)->byLoginData, 
			pMinerConn->MinerData.iMinerId,
			"result",
			pMinerConn->pMinerRes->iWbsId);
			//pMinerConn->MinerRes.WbIpPort.byWbIp, pMinerConn->MinerRes.WbIpPort.byWbPort);
		break;

	case E_ALGO_DUAL:
		*pLen = snprintf(destBuf, iSize, "%s %d %s %d %d", 
			(LoginOutputHead+0)->byLoginData, 
			pMinerConn->MinerData.iMinerId, 
			"result",
			pMinerConn->pMinerRes[0].iWbsId,
			pMinerConn->pMinerRes[1].iWbsId);
		break;

	default:
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Algorithm[%d] does not exist!\r\n", __FILE__, __FUNCTION__, __LINE__, pMinerConn->eClinetType);
		break;
	}
	DEBUGMSG(0,("%s\r\n", destBuf));

	return 0;
}*/

int PackCellData(stCellConn *pCellConn, char *destBuf, int *pLen, int iSize)
{
	//int iTotal = 0;
	//int iShare = 1;
	//int iDivRtn = 0;
	//structList *pList = NULL;
	//list_node_t *pNode = NULL;
	int i = 0, iPos = 0;//iCount = 0, 

	DEBUGMSG(1, ("%s...\r\n", __FUNCTION__));
	//iTotal = pCellConn->CellRes.iConnCellNum;
	/*DEBUGMSG(0,("iTotal:%d, ResIndex:%d\r\n", iTotal, gpDividXnonce[ResIndex].bDividDisable));
	if (gpDividXnonce[ResIndex].bDividDisable == FALSE)
	{
		memset((LPSTR)pCellConn->byDividXnonce, 0 , sizeof(pCellConn->byDividXnonce));

		DEBUGMSG(0, ("2 %s...\r\n", __FUNCTION__));
		if ((iDivRtn = DividXnonce2(gpDividXnonce + ResIndex, (LPSTR)pCellConn->byDividXnonce, iShare, iTotal, sizeof(pCellConn->byDividXnonce))) != 0 && iDivRtn != 3)
		{
			strncpy((char *)pCellConn->byDividXnonce, cDividXnonce, sizeof(cDividXnonce));
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d,  iDivRtn:%d!\r\n", __FILE__, __FUNCTION__, __LINE__, iDivRtn);
		}

		DEBUGMSG(0,("---iDivRtn:%d\r\n", iDivRtn));
	}*/
	/*switch(pCellConn->CellData.eCellAlgo)
	{
	case E_ALGO_BTC:
	case E_ALGO_LTC:
		*pLen = snprintf(destBuf, iSize, "%s %d %s %d %d %d", 
			(LoginOutputHead+1)->byLoginData, 
			pCellConn->CellData.iCellId,
			"result",
			pCellConn->CellData.iChipBTCFreq,
			pCellConn->pCellRes->iConnCellNum,
			pCellConn->pCellRes->iWbcId);
			//pCellConn->CellRes.WbIpPort.byWbIp, pCellConn->CellRes.WbIpPort.byWbPort, pCellConn->byDividXnonce);
		break;

	case E_ALGO_DUAL:
		*pLen = snprintf(destBuf, iSize, "%s %d %s %d %d %d %d", 
			(LoginOutputHead+1)->byLoginData,
			pCellConn->CellData.iCellId,
			"result",
			pCellConn->CellData.iChipBTCFreq,
			pCellConn->pCellRes[0].iConnCellNum,
			pCellConn->pCellRes[0].iWbcId,
			//pCellConn->pCellRes[1].iConnCellNum,
			pCellConn->pCellRes[1].iWbcId);
		break;
			
	default:
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Algorithm[%d] does not exist!\r\n", __FILE__, __FUNCTION__, __LINE__, pCellConn->eClinetType);
		break;
	}*/



	snprintf(destBuf + iPos, iSize, 
		"%s %d %s %s %u %u",
		(LoginOutputHead+1)->byLoginData, 
		pCellConn->CellData.iCellId,
		"result",
		pCellConn->CellData.szDevId,
		pCellConn->CellData.iChipBTCFreq,
		pCellConn->CellData.iChipLTCFreq);
	iPos += strlen(destBuf+iPos);

	DEBUGMSG(1,("111111 %s\r\n", destBuf));

	//初始化为默认池
	pthread_mutex_lock(&pCellConn->CellDataLock.PoolConfLock);
	pCellConn->CellPoolConf.iBTCPoolCount = 0;
	for(i = 0;i < gPoolDefaultConf.iBTCPoolCount && gPoolDefaultConf.iBTCPoolCount <= MAX_POOL_COUNT;i++)	
	{	
		pCellConn->CellPoolConf.stBTCPoolConf[i].iPoolId = gPoolDefaultConf.stBTCPoolConf[i].iPoolId;
		DEBUGMSG(0,("BTC iPoolId[%d]:%d\r\n", i, pCellConn->CellPoolConf.stBTCPoolConf[i].iPoolId));
		strcpy((char *)pCellConn->CellPoolConf.stBTCPoolConf[i].szUrl, (char *)gPoolDefaultConf.stBTCPoolConf[i].szUrl);
		strcpy((char *)pCellConn->CellPoolConf.stBTCPoolConf[i].szUser, (char *)gPoolDefaultConf.stBTCPoolConf[i].szUser);
		strcpy((char *)pCellConn->CellPoolConf.stBTCPoolConf[i].szPasswd, (char *)gPoolDefaultConf.stBTCPoolConf[i].szPasswd);
		pCellConn->CellPoolConf.stBTCPoolConf[i].eAlgo = gPoolDefaultConf.stBTCPoolConf[i].eAlgo;
		pCellConn->CellPoolConf.iBTCPoolCount++;
	
		snprintf(destBuf + iPos, iSize, 
			" %d/%s/%s/%s",
			pCellConn->CellPoolConf.stBTCPoolConf[i].eAlgo+1,
			pCellConn->CellPoolConf.stBTCPoolConf[i].szUrl,
			pCellConn->CellPoolConf.stBTCPoolConf[i].szUser,
			pCellConn->CellPoolConf.stBTCPoolConf[i].szPasswd
			);
		iPos += strlen(destBuf+iPos);
	}

	DEBUGMSG(1,("222222 %s\r\n", destBuf));
	
	pCellConn->CellPoolConf.iLTCPoolCount = 0;
	for(i = 0;i < gPoolDefaultConf.iLTCPoolCount && gPoolDefaultConf.iLTCPoolCount <= MAX_POOL_COUNT;i++)	
	{	
		pCellConn->CellPoolConf.stLTCPoolConf[i].iPoolId = gPoolDefaultConf.stLTCPoolConf[i].iPoolId;
		DEBUGMSG(0,("LTC iPoolId[%d]:%d\r\n", i, pCellConn->CellPoolConf.stLTCPoolConf[i].iPoolId));
		strcpy((char *)pCellConn->CellPoolConf.stLTCPoolConf[i].szUrl, (char *)gPoolDefaultConf.stLTCPoolConf[i].szUrl);
		strcpy((char *)pCellConn->CellPoolConf.stLTCPoolConf[i].szUser, (char *)gPoolDefaultConf.stLTCPoolConf[i].szUser);
		strcpy((char *)pCellConn->CellPoolConf.stLTCPoolConf[i].szPasswd, (char *)gPoolDefaultConf.stLTCPoolConf[i].szPasswd);
		pCellConn->CellPoolConf.stLTCPoolConf[i].eAlgo = gPoolDefaultConf.stLTCPoolConf[i].eAlgo;
		pCellConn->CellPoolConf.iLTCPoolCount++;

		snprintf(destBuf + iPos, iSize, 
			" %d/%s/%s/%s",
			pCellConn->CellPoolConf.stLTCPoolConf[i].eAlgo+1,
			pCellConn->CellPoolConf.stLTCPoolConf[i].szUrl,
			pCellConn->CellPoolConf.stLTCPoolConf[i].szUser,
			pCellConn->CellPoolConf.stLTCPoolConf[i].szPasswd
			);
		iPos += strlen(destBuf+iPos);
	}
	pthread_mutex_unlock(&pCellConn->CellDataLock.PoolConfLock);

	/*pList = &stBTCPoolConfList;
	pthread_mutex_lock(pList->ListLenLock);
	iCount = pList->List->len;
	pthread_mutex_unlock(pList->ListLenLock);
	
	pthread_mutex_lock(pList->RemoveLock);
	for(i = 0;i < iCount && i < MAX_POOL_COUNT;i++)
	{	
		pNode = pList->ListAt(pList->List, i);
		snprintf(destBuf + iPos, iSize, 
			" %d/%s/%s/%s",
			((stPoolConfInfo *)pNode->val)->eAlgo+1,
			((stPoolConfInfo *)pNode->val)->szUrl,
			((stPoolConfInfo *)pNode->val)->szUser,
			((stPoolConfInfo *)pNode->val)->szPasswd
			);

		iPos += strlen(destBuf+iPos);
	}
	pthread_mutex_unlock(pList->RemoveLock);

	DEBUGMSG(0,("2 %s\r\n", destBuf));*/
	
	/*if (iCount > 0)
	{
		snprintf(destBuf + iPos, iSize, "%s", " ");
		iPos += strlen(destBuf+iPos);
	}*/

	/*pList = &stLTCPoolConfList;
	pthread_mutex_lock(pList->ListLenLock);
	iCount = pList->List->len;
	pthread_mutex_unlock(pList->ListLenLock);

	pthread_mutex_lock(pList->RemoveLock);
	for(i = 0;i < iCount && i < MAX_POOL_COUNT;i++)
	{
		pNode = pList->ListAt(pList->List, i);
		snprintf(destBuf + iPos, iSize, 
			" %d/%s/%s/%s",
			((stPoolConfInfo *)pNode->val)->eAlgo+1,
			((stPoolConfInfo *)pNode->val)->szUrl,
			((stPoolConfInfo *)pNode->val)->szUser,
			((stPoolConfInfo *)pNode->val)->szPasswd
			);

		iPos += strlen(destBuf+iPos);
	}
	pthread_mutex_unlock(pList->RemoveLock);*/
	*pLen = iPos;
	DEBUGMSG(1,("iPos:%d\r\n", iPos));
	DEBUGMSG(1,("%s\r\n", destBuf));

	return 0;
}

int PackCellIdData(stIdAllocTab *pIdTable, char *destBuf, int *pLen, int iSize)
{		
	*pLen = snprintf(destBuf, iSize, "%s %d %s", 
		(LoginOutputHead+1)->byLoginData, 
		pIdTable->iId,
		"result"
		);

	return 0;
}


//打成1包
int PackJsonCellRepeat(void *pSrc, void *pDev, void *dest, int destLen)
{
	int rtn = 0;
	//structUthash *pList = NULL;
	//int iTotListLen = 0;
	//devlist_ist *pNode = NULL;//*tmp = NULL;
	char *pBuf = NULL;
	int i = 0;
	cJSON *pArray = NULL;
	cJSON *pArrToJson = NULL;
	cJSON *pResultJson = NULL;
	stCellConn *pCellConn = NULL;
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	pCellConn = (stCellConn *)pDev;
		
	cJSON* pJsonRoot = cJSON_CreateObject();
    if (pJsonRoot == NULL )
    {
        return -1;
    }

	cJSON_AddNumberToObject(pJsonRoot, "id", *((stLoginSrc*)pSrc)->pIdSeq);
	cJSON_AddStringToObject(pJsonRoot, "cellid", (char *)pCellConn->CellData.szCellId);
	cJSON_AddStringToObject(pJsonRoot, "method", (char *)((stLoginSrc*)pSrc)->pLoginComm[0].cmd);

	pArray = cJSON_CreateArray();
    if(NULL == pArray)
    {
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pArray!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);        
        return -1;
    }
	
	//发送cell自己的池
	pthread_mutex_lock(&pCellConn->CellDataLock.PoolConfLock);
	for(i = 0;i < pCellConn->CellPoolConf.iBTCPoolCount && pCellConn->CellPoolConf.iBTCPoolCount <= MAX_POOL_COUNT;i++)
	{	
		pArrToJson = cJSON_CreateObject();
		if (pArrToJson == NULL)
			continue;
		cJSON_AddNumberToObject(pArrToJson, "algo", pCellConn->CellPoolConf.stBTCPoolConf[i].eAlgo + 1);
		cJSON_AddStringToObject(pArrToJson, "url", (char *)pCellConn->CellPoolConf.stBTCPoolConf[i].szUrl);
		cJSON_AddStringToObject(pArrToJson, "worker",(char *) pCellConn->CellPoolConf.stBTCPoolConf[i].szUser);
		cJSON_AddStringToObject(pArrToJson, "pass", (char *)pCellConn->CellPoolConf.stBTCPoolConf[i].szPasswd);
		//cJSON_AddItemToObject(pArray, "", pArrToJson);
		cJSON_AddItemToArray(pArray, pArrToJson);	
	}
	
	for(i = 0;i < pCellConn->CellPoolConf.iLTCPoolCount && pCellConn->CellPoolConf.iLTCPoolCount <= MAX_POOL_COUNT;i++)
	{	
		pArrToJson = cJSON_CreateObject();
		if (pArrToJson == NULL)
			continue;
		cJSON_AddNumberToObject(pArrToJson, "algo", pCellConn->CellPoolConf.stLTCPoolConf[i].eAlgo + 1);
		cJSON_AddStringToObject(pArrToJson, "url", (char *)pCellConn->CellPoolConf.stLTCPoolConf[i].szUrl);
		cJSON_AddStringToObject(pArrToJson, "worker", (char *)pCellConn->CellPoolConf.stLTCPoolConf[i].szUser);
		cJSON_AddStringToObject(pArrToJson, "pass", (char *)pCellConn->CellPoolConf.stLTCPoolConf[i].szPasswd);
		//cJSON_AddItemToObject(pArray, "", pArrToJson);
		cJSON_AddItemToArray(pArray, pArrToJson);	
	}
	pthread_mutex_unlock(&pCellConn->CellDataLock.PoolConfLock);

	pResultJson = cJSON_CreateObject();
    if(NULL == pResultJson)
    {
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pResultJson!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);        
        return -1;
    }
	
	cJSON_AddNumberToObject(pResultJson, "btc_freq", pCellConn->CellData.iChipBTCFreq);
	cJSON_AddNumberToObject(pResultJson, "ltc_freq", pCellConn->CellData.iChipLTCFreq);
	cJSON_AddItemToObject(pResultJson, "pools", pArray);
	
	cJSON_AddItemToObject(pJsonRoot, "result", pResultJson);
	pBuf = cJSON_Print(pJsonRoot);
    if(NULL == pBuf)
    {
        // create object faild, exit
        cJSON_Delete(pJsonRoot);
        return -1;
    }
	destLen = strlen(pBuf);
	strncpy(dest, pBuf, destLen);

	DEBUGMSG(1,("destLen:%d\ndest:%s\n", destLen, (char *)dest));

	//释放
	cJSON_Delete(pJsonRoot);
	free(pBuf);

    return rtn;
}


//打成1包
int PackJsonCellLogin(void *pSrc, void *pDev, void *dest, int destLen)
{
	int rtn = 0;
	//structUthash *pList = NULL;
	//int iTotListLen = 0;
	//devlist_ist *pNode = NULL;//*tmp = NULL;
	char *pBuf = NULL;
	int i = 0;
	cJSON *pArray = NULL;
	cJSON *pArrToJson = NULL;
	cJSON *pResultJson = NULL;
	stCellConn *pCellConn = NULL;
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	pCellConn = (stCellConn *)pDev;
		
	cJSON* pJsonRoot = cJSON_CreateObject();
    if (pJsonRoot == NULL )
    {
        return -1;
    }

	cJSON_AddNumberToObject(pJsonRoot, "id", *((stLoginSrc*)pSrc)->pIdSeq);
	cJSON_AddStringToObject(pJsonRoot, "cellid", (char *)pCellConn->CellData.szCellId);
	cJSON_AddStringToObject(pJsonRoot, "method", (char *)((stLoginSrc*)pSrc)->pLoginComm[0].cmd);

	
	pArray = cJSON_CreateArray();
    if(NULL == pArray)
    {
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pArray!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);        
        return -1;
    }
	
	//初始化为默认池
	pthread_mutex_lock(&pCellConn->CellDataLock.PoolConfLock);
	pCellConn->CellPoolConf.iBTCPoolCount = 0;
	for(i = 0;i < gPoolDefaultConf.iBTCPoolCount && gPoolDefaultConf.iBTCPoolCount <= MAX_POOL_COUNT;i++)	
	{	
		pCellConn->CellPoolConf.stBTCPoolConf[i].iPoolId = gPoolDefaultConf.stBTCPoolConf[i].iPoolId;
		DEBUGMSG(0,("BTC iPoolId[%d]:%d\r\n", i, pCellConn->CellPoolConf.stBTCPoolConf[i].iPoolId));
		strcpy((char *)pCellConn->CellPoolConf.stBTCPoolConf[i].szUrl, (char *)gPoolDefaultConf.stBTCPoolConf[i].szUrl);
		strcpy((char *)pCellConn->CellPoolConf.stBTCPoolConf[i].szUser, (char *)gPoolDefaultConf.stBTCPoolConf[i].szUser);
		strcpy((char *)pCellConn->CellPoolConf.stBTCPoolConf[i].szPasswd, (char *)gPoolDefaultConf.stBTCPoolConf[i].szPasswd);
		pCellConn->CellPoolConf.stBTCPoolConf[i].eAlgo = gPoolDefaultConf.stBTCPoolConf[i].eAlgo;

		pArrToJson = cJSON_CreateObject();
		if (pArrToJson == NULL)
			continue;
		cJSON_AddNumberToObject(pArrToJson, "algo", gPoolDefaultConf.stBTCPoolConf[i].eAlgo + 1);
		cJSON_AddStringToObject(pArrToJson, "url", (char *)gPoolDefaultConf.stBTCPoolConf[i].szUrl);
		cJSON_AddStringToObject(pArrToJson, "worker", (char *)gPoolDefaultConf.stBTCPoolConf[i].szUser);
		cJSON_AddStringToObject(pArrToJson, "pass", (char *)gPoolDefaultConf.stBTCPoolConf[i].szPasswd);
		//cJSON_AddItemToObject(pArray, "", pArrToJson);
		cJSON_AddItemToArray(pArray, pArrToJson);

		pCellConn->CellPoolConf.iBTCPoolCount++;
	}
	
	pCellConn->CellPoolConf.iLTCPoolCount = 0;
	for(i = 0;i < gPoolDefaultConf.iLTCPoolCount && gPoolDefaultConf.iLTCPoolCount <= MAX_POOL_COUNT;i++)	
	{	
		pCellConn->CellPoolConf.stLTCPoolConf[i].iPoolId = gPoolDefaultConf.stLTCPoolConf[i].iPoolId;
		DEBUGMSG(0,("LTC iPoolId[%d]:%d\r\n", i, pCellConn->CellPoolConf.stLTCPoolConf[i].iPoolId));
		strcpy((char *)pCellConn->CellPoolConf.stLTCPoolConf[i].szUrl, (char *)gPoolDefaultConf.stLTCPoolConf[i].szUrl);
		strcpy((char *)pCellConn->CellPoolConf.stLTCPoolConf[i].szUser, (char *)gPoolDefaultConf.stLTCPoolConf[i].szUser);
		strcpy((char *)pCellConn->CellPoolConf.stLTCPoolConf[i].szPasswd, (char *)gPoolDefaultConf.stLTCPoolConf[i].szPasswd);
		pCellConn->CellPoolConf.stLTCPoolConf[i].eAlgo = gPoolDefaultConf.stLTCPoolConf[i].eAlgo;

		pArrToJson = cJSON_CreateObject();
		if (pArrToJson == NULL)
			continue;
		cJSON_AddNumberToObject(pArrToJson, "algo", gPoolDefaultConf.stLTCPoolConf[i].eAlgo + 1);
		cJSON_AddStringToObject(pArrToJson, "url", (char *)gPoolDefaultConf.stLTCPoolConf[i].szUrl);
		cJSON_AddStringToObject(pArrToJson, "worker", (char *)gPoolDefaultConf.stLTCPoolConf[i].szUser);
		cJSON_AddStringToObject(pArrToJson, "pass", (char *)gPoolDefaultConf.stLTCPoolConf[i].szPasswd);
		//cJSON_AddItemToObject(pArray, "", pArrToJson);
		cJSON_AddItemToArray(pArray, pArrToJson);

		pCellConn->CellPoolConf.iLTCPoolCount++;
	}
	pthread_mutex_unlock(&pCellConn->CellDataLock.PoolConfLock);

	pResultJson = cJSON_CreateObject();
    if(NULL == pResultJson)
    {
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pResultJson!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);        
        return -1;
    }
	
	cJSON_AddNumberToObject(pResultJson, "btc_freq", pCellConn->CellData.iChipBTCFreq);
	cJSON_AddNumberToObject(pResultJson, "ltc_freq", pCellConn->CellData.iChipLTCFreq);
	cJSON_AddItemToObject(pResultJson, "pools", pArray);
	
	cJSON_AddItemToObject(pJsonRoot, "result", pResultJson);
	pBuf = cJSON_Print(pJsonRoot);
    if(NULL == pBuf)
    {
        // create object faild, exit
        cJSON_Delete(pJsonRoot);
        return -1;
    }
	destLen = strlen(pBuf);
	strncpy(dest, pBuf, destLen);

	DEBUGMSG(1,("destLen:%d\ndest:%s\n", destLen, (char *)dest));

	//释放
	cJSON_Delete(pJsonRoot);
	free(pBuf);

    return rtn;
}


int ParseAlgo(int argc, char**argv, int iStartPos, E_ALGO *peAlgo)
{
	int index = 0, i = 0;
	int iCmdLen = 0, iParamLen = 0, iAlgLen = 0;;
	int rtn = -1;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	index = iStartPos;
	iCmdLen = dim(Algo);	
	iParamLen = strlen(argv[index]);
	DEBUGMSG(0,("argv[%d]:%s\n", index, argv[index]));
	
	for (i = 0; i < iCmdLen; i++)
	{
		iAlgLen = strlen((char *)Algo[i].algo);
		if (strncasecmp((char *)Algo[i].algo, argv[index], iAlgLen) == 0 && iAlgLen == iParamLen)
		{	
			switch (i)
			{
			case 0:
				*peAlgo = E_ALGO_BTC;
				index++;
				rtn = index;
				break;
				
			case 1:
				*peAlgo = E_ALGO_LTC;
				index++;
				rtn = index;
				break;

			case 2:		
				*peAlgo = E_ALGO_DUAL;
				index++;
				rtn = index;	
				break;
				
			default:
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d,  dev algorithm[%d] does not exist!\r\n", __FILE__, __FUNCTION__, __LINE__, i);				
				break;
			}		
			break;
		}
	}
	DEBUGMSG(0,("^^^^^^^^^^^rtn:%d\n", rtn));
	return rtn;
}

#if 0
/****************************************************************
** Function name:      	ParseMinerLogin
** Descriptions:   	

** input parameters:   	无
** output parameters:   	无
** Returned value:     	1:argc < iStartPos
						2:Miner algorithm does not exist
						3:分配资源空间错误
						7:over btc number
						8:over ltc number

****************************************************************/
int ParseMinerLogin(int argc, char**argv, int iStartPos)
{
	int rtn = -1, iQueryRtn = 0;
	int index = 0;
	BYTE byMinerIp[BUFFER_LEN_16] = {0};	//miner的ip
	int	iMinerPort = 0;						//miner的port
	int iLen = 0, iListLen = 0;
	BYTE destBuf[BUFFER_LEN_256] = {0};
	int i = 0;
	stMinerConn *val = NULL;
	E_LOGIN_ERROR eLoginErr;
	char bufDbDest[BUFFER_LEN_512] = {0};
	
	if (argc < iStartPos || argc != MINER_LOGIN_COMMANDLINE)
		return 1;
	
	index = iStartPos;
	DEBUGMSG(1, ("%s...\r\n", __FUNCTION__));
	val = malloc(sizeof(stMinerConn));
	memset(val, 0, sizeof(stMinerConn));

	val->MinerData.iMinerId = strtoul(argv[index++], 0, 0);
	iStartPos = index;
	
	index = ParseAlgo(argc, argv, iStartPos, &val->MinerData.eMinerAlgo);
	if (index < 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, ParseAlgo error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return 2;
	}

	//分配资源空间
	/*if (AllocMinerRes(val->MinerData.eMinerAlgo, &val->pMinerRes) < 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, AllocRes error!\r\n",  __FILE__, __FUNCTION__, __LINE__);	
		return 3;
	}*/

	DEBUGMSG(0,("index:%d, @pMinerRes:%p\r\n", index, val->pMinerRes));
	//return 0;
	strncpy((char *)byMinerIp, argv[index++], sizeof(byMinerIp));
	iMinerPort = atoi(argv[index++]);

	DEBUGMSG(0,("MinerIp:%s, MinerPort:%d, %d\r\n", (char *)byMinerIp, iMinerPort, val->MinerData.iMinerId));
		
	SetClientIp4(&val->remote.addr4, byMinerIp, iMinerPort);	
	val->MinerParent.sock = atoi(argv[index++]);

	DEBUGMSG(0,("---%s ---%d, sock:%d, eMinerAlgo:%d\r\n", 
		inet_ntoa(val->remote.addr4.sin_addr), val->remote.addr4.sin_port, val->MinerParent.sock, val->MinerData.eMinerAlgo));
	
#if 1
	DEBUGMSG(0,("@Miner LoginRemLock:%p\r\n", stMinerUtlist.LoginRemLock));
	pthread_mutex_lock(stMinerUtlist.LoginRemLock);
	pthread_mutex_lock(stMinerUtlist.InitNodeLock);
	
	//查找MinerId	
	devlist_ist *MinerNode = stMinerUtlist.FindUserInt(stMinerUtlist.HashUsers, val->MinerData.iMinerId);
	if (MinerNode != NULL)
	{
		//printf("--- id : %d ?= %d\r\n", ((stMinerConn *)MinerNode->val)->MinerData.iMinerId, val->MinerData.iMinerId);
		pthread_mutex_lock(&((stMinerConn *)MinerNode->val)->MinerDataLock.AlarmTimerLock);		
		((stMinerConn *)MinerNode->val)->MinerData.iMinerAlarmTimer = GetTime();
		pthread_mutex_unlock(&((stMinerConn *)MinerNode->val)->MinerDataLock.AlarmTimerLock);
		
		if (strcmp(inet_ntoa(val->remote.addr4.sin_addr), inet_ntoa(((stMinerConn *)MinerNode->val)->remote.addr4.sin_addr)) != 0
			|| val->remote.addr4.sin_port != ((stMinerConn *)MinerNode->val)->remote.addr4.sin_port)
		{
			pthread_mutex_lock(&((stMinerConn *)MinerNode->val)->MinerDataLock.IpPortLock);
			SetClientIp4(&(((stMinerConn *)MinerNode->val)->remote.addr4), byMinerIp, iMinerPort);
			pthread_mutex_unlock(&((stMinerConn *)MinerNode->val)->MinerDataLock.IpPortLock);
			//有数据已经插入数据库后，才能进行更新。
			if (((stMinerConn *)MinerNode->val)->eMinerMySql != E_QUERY_TYPE_INSERT)				
				((stMinerConn *)MinerNode->val)->eMinerMySql = E_QUERY_TYPE_UPDATE;
		}	
		//printf("val->eMinerMySql:%d, @:%p\r\n", ((stMinerConn *)MinerNode->val)->eMinerMySql, &((stMinerConn *)MinerNode->val)->eMinerMySql);
		/*pthread_mutex_lock(OnlineLock.pMinerOnliLock);
		((stMinerConn *)MinerNode->val)->MinerData.bMinerOnline = TRUE;
		pthread_mutex_unlock(OnlineLock.pMinerOnliLock);*/

		val->eMinerComm = E_COMMAND_TYPE_STAT;
		PackMinerRepeat((stMinerConn *)MinerNode->val, (char *)destBuf, &iLen, sizeof(destBuf));
		free(val);
	}
	else
	{
		pthread_mutex_lock(stMinerUtlist.ListLenLock);
		iListLen = stMinerUtlist.CountUsersInt(*stMinerUtlist.HashUsers);
		pthread_mutex_unlock(stMinerUtlist.ListLenLock);
		
		//DEBUGMSG(1,("^^^^^^^^iListLen:%d, iMinerNum:%d\r\n", iListLen, gLoginResIndex.iMinerNum));
		if (iListLen < gLoginResIndex.iMinerNum)
		{
			DEBUGMSG(0, ("eMinerAlgo:%d, len:%d\r\n", val->MinerData.eMinerAlgo, sizeof(gMinerAssi)/sizeof(gMinerAssi[0])));
			if ((i = val->MinerData.eMinerAlgo) < sizeof(gMinerAssi)/sizeof(gMinerAssi[0]))
			{
				DEBUGMSG(0,("i:%d, pCrrAlgoLen:%d, pSumAlgoLen:%d\r\n", i, *gMinerAssi[i].pCrrAlgoLen, *gMinerAssi[i].pSumAlgoLen));
				if (*gMinerAssi[i].pCrrAlgoLen < *gMinerAssi[i].pSumAlgoLen)
				{

					pthread_mutex_lock(gResourceLock.CrrMinerAlgoLenLock);
					eLoginErr = IncrCrrentAlgoLen(i, gMinerAssi);
					pthread_mutex_unlock(gResourceLock.CrrMinerAlgoLenLock);	

					if (eLoginErr == 0)
					{
						gMinerAssi[i].ResToAlgoFunc(val, gpResource, gLoginResIndex.iMinerNum); 		
						PackMinerData(val, (char *)destBuf, &iLen, sizeof(destBuf));
						
						//初始化
						pthread_mutex_init(&val->MinerDataLock.OnlineLock, NULL);
						pthread_mutex_init(&val->MinerDataLock.AlarmTimerLock, NULL);
						pthread_mutex_init(&val->MinerDataLock.IpPortLock, NULL);
						val->MinerData.bMinerOnline = TRUE;
						val->eMinerComm = E_COMMAND_TYPE_STAT;

						//注册时间、登陆时间与告警判断时间
						val->MinerData.iRegtime = val->MinerData.iLoginTime = val->MinerData.iMinerAlarmTimer = GetTime(); 

						MinerSqlQuery[E_QUERY_TYPE_INSERT - 1].QueryPackFunc1(
										val, 
										&gCfgDataDL.MinerBaseInfo, 
										&gCfgDataDL.MinerColName, 
										bufDbDest, 
										sizeof(bufDbDest));
						iQueryRtn = MysqlFunc.dbDoQuery(&myMinerBase, bufDbDest, E_QUERY_TYPE_INSERT, NULL);
						memset(bufDbDest, 0, sizeof(bufDbDest));
						if (iQueryRtn != 0)
						{
							//如果新结点插入失败，在switch没有重启时，则定时插入此结点。
							val->eMinerMySql = E_QUERY_TYPE_INSERT;
						}

						//无重复MinerId，插入链表
						DEBUGMSG(0,("1 @Miner HashUsers:%p\r\n", stMinerUtlist.HashUsers));
						stMinerUtlist.AddUserInt(stMinerUtlist.HashUsers, val->MinerData.iMinerId, (void**)&val);
						rtn = 0;

						DEBUGMSG(0,("2 @Miner HashUsers:%p\r\n", stMinerUtlist.HashUsers));
					}
					else
					{
						GetLocalTimeLog();
						ap_log_debug_log("\t\t%s, %s, %d, MinerId [%d] BL has exceeded the maximum length set!\r\n",  __FILE__, __FUNCTION__, __LINE__, val->MinerData.iMinerId);			
						PackMinerError(val, (char *)destBuf, &iLen, sizeof(destBuf), eLoginErr - 1);
						rtn = eLoginErr;
					}
				}
				else
				{
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, MinerId [%d] algo has exceeded the maximum length set!\r\n",  __FILE__, __FUNCTION__, __LINE__, val->MinerData.iMinerId);			
					PackMinerError(val, (char *)destBuf, &iLen, sizeof(destBuf), E_MINER_LOGIN_ERROR_OVERALGO - 1);
				}
			}

		}
		else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, MinerId [%d] has exceeded the maximum length set!\r\n",  __FILE__, __FUNCTION__, __LINE__, val->MinerData.iMinerId);			
			PackMinerError(val, (char *)destBuf, &iLen, sizeof(destBuf), E_MINER_LOGIN_ERROR_OVERNUM - 1);
		}
	}
	pthread_mutex_unlock(stMinerUtlist.LoginRemLock);
	pthread_mutex_unlock(stMinerUtlist.InitNodeLock);
	
	if (stApiNetWork.ApiNetSendToClient(val->MinerParent.sock, val->remote.addr4, destBuf, iLen, gpLoginPool) < 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d,  ApiNetSendToClient error!\r\n", __FILE__, __FUNCTION__, __LINE__);
	}
	else
	{
		usleep(1);
	}

#endif
	DEBUGMSG(0,("Miner destBuf:%s\r\n", (char *)destBuf));
	
	return rtn;
}
#endif

/****************************************************************
** Function name:      	ParseCellAllocId
** Descriptions:   	

** input parameters:   	pCellConn:CellNode
** output parameters:   	无
** Returned value:     	1:没有连接上数据库
						2:对数据库操作失败

****************************************************************/
int ParseCellAllocId(stCellConn *pCellConn)
{
	int rtn = 0, iListLen = 0, i = 0, iQueryRtn = 0;
	//long lDevIdTemp = 0;
	BYTE szDevIdTemp[BUFFER_LEN_28] = {0};
	structList *pList = NULL;
	list_node_t *pNode = NULL;
	char bufDest[BUFFER_LEN_512] = {0};
	int iCellIdTemp = 0;

	DEBUGMSG(1, ("%s...\r\n", __FUNCTION__));
	iCellIdTemp = pCellConn->CellData.iCellId;
	//lDevIdTemp = pCellConn->CellData.lDevId;
	strncpy((char *)szDevIdTemp, (char *)pCellConn->CellData.szDevId, sizeof(szDevIdTemp));

	DEBUGMSG(1,("iCellIdTemp:%d, szDevIdTemp:%s\r\n", iCellIdTemp, (char *)szDevIdTemp));
	//iDevIdTemp = strtoul(argv[index++], 0, 0);
	//strncpy((char *)byCellIp, argv[index++], sizeof(byCellIp));
	//iCellPort = strtoul(argv[index++], 0, 0);
	//sockTemp = strtoul(argv[index++], 0, 0);
	//DEBUGMSG(0,("+++ MinerIp:%s, MinerPort:%d\r\n", byCellIp, iCellPort));
	//SetClientIp4(&addr4Temp, byCellIp, iCellPort);

	//查找DevId	
	devlist_st *CellIdNode = stCellIdUtlist.FindUserStr(stCellIdUtlist.StrHUsers, (char *)szDevIdTemp);
	if (CellIdNode != NULL)
	{		
		//PackCellIdData((stIdAllocTab *)CellIdNode->val, (char *)destBuf, &iLen, sizeof(destBuf));
		pCellConn->CellData.iCellId = ((stIdAllocTab *)CellIdNode->val)->iId;

		if (iCellIdTemp != pCellConn->CellData.iCellId)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, Create newnode,CellId %d != %d is error!\r\n",	
					__FILE__, __FUNCTION__, __LINE__, iCellIdTemp, pCellConn->CellData.iCellId);				
		}
	}
	else
	{		
		//建立新节点时，iCellId = 0，否则只报警告
		if (iCellIdTemp != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, Create newnode,CellId = %d != 0 is error!\r\n",	
					__FILE__, __FUNCTION__, __LINE__, iCellIdTemp);				
		}
		
		stIdAllocTab *val = malloc(sizeof(stIdAllocTab));
		memset(val, 0, sizeof(stIdAllocTab));
		strncpy((char *)val->szDevId, (char *)szDevIdTemp, sizeof(val->szDevId));

		pList = &stDelCellIdList;
		pthread_mutex_lock(pList->ListLenLock);
		iListLen = pList->List->len;
		pthread_mutex_unlock(pList->ListLenLock);
		if (pList->List == NULL || iListLen == 0)
		{
			val->iId = ++gCrrMaxCellId;
		}
		else
		{	
			pthread_mutex_lock(pList->RemoveLock);
			for (i = 0; i < iListLen; i++)
			{
				pNode = stDelCellIdList.ListAt(stDelCellIdList.List, i);
				if (pNode == NULL)
					continue;
				else
				{				
					if (bIniDelCellIdBase)
					{
						DelCellIdBaseQuery[E_QUERY_TYPE_DELETE - 1].QueryPackFunc1(
											pNode->val, 
											&gCfgDataDL.DelCellIdBaseInfo, 
											&gCfgDataDL.DelCellIdBaColName, 
											bufDest, 
											sizeof(bufDest));
						
						iQueryRtn = MysqlFunc.dbDoQuery(&myDelCellIdBase, bufDest, E_QUERY_TYPE_DELETE, NULL);
						memset(bufDest, 0, sizeof(bufDest));

						if (iQueryRtn == 0)	
						{
							val->iId = ((stDelIdAllocTab *)pNode->val)->iDelCellId;	
							stDelCellIdList.ListRemove(stDelCellIdList.List, pNode);
						}
						else
						{
							//对数据库操作失败时，则用当前最大id。
							val->iId = ++gCrrMaxCellId;
							rtn = 2;
						}
					}
					else
					{
						//如果对数据库没有连接上，则用当前最大id。
						val->iId = ++gCrrMaxCellId;
						rtn = 1;
					}
					break;
				}
			}
			pthread_mutex_unlock(pList->RemoveLock);		
		}	
		pCellConn->CellData.iCellId = val->iId;
		stCellIdUtlist.AddUserStr(stCellIdUtlist.StrHUsers, (char *)val->szDevId, sizeof(val->szDevId), (void**)&val);		
		rtn = 0;
	}
	return rtn;
}

/****************************************************************
** Function name:      	ParseCellLogin
** Descriptions:   	

** input parameters:   	无
** output parameters:   	无
** Returned value:     	1:argc < iStartPos
						2:Cell algorithm does not exist
						3:分配资源空间错误
						7:over btc number
						8:over ltc number

****************************************************************/
int ParseCellLogin(int argc, char**argv, int iStartPos)
{
	int rtn = -1, iQueryRtn = 0, rtnLoginSrc = 0;//CellIdRtn = 0, 
	int index = 0;
	BYTE byCellIp[BUFFER_LEN_16];       //cell的ip	
	int	iCellPort;                      //cell的port	
	int iLen = 0;
	BYTE destBuf[BUFFER_LEN_512] = {0};
	//int iListLen = 0;
	//int i = 0;
	//E_LOGIN_ERROR eLoginErr;
	BOOL bUpdateDb = FALSE;
	char bufDbDest[BUFFER_LEN_1024] = {0};
	devlist_st *CellNode;
	structUthash *pUtList = NULL;

	if (argc < iStartPos || argc != CELL_LOGIN_COMMANDLINE)
		return 1;
	pUtList = &stCellUtlist;
	index = iStartPos;
	DEBUGMSG(1, ("%s...\r\n", __FUNCTION__));

	stCellConn *val = malloc(sizeof(stCellConn));
	memset(val, 0, sizeof(stCellConn));
	
	//val->CellData.szCellId = strtoul(argv[index++], 0, 0);
	//strncpy((char *)val->CellData.bySwVer, argv[index++], sizeof(val->CellData.bySwVer));
	//strncpy((char *)val->CellData.byHwVer, argv[index++], sizeof(val->CellData.byHwVer));
	//val->CellData.iSwVer = strtoul(argv[index++], 0, 0);
	//val->CellData.iHwVer = strtoul(argv[index++], 0, 0);
	strncpy((char *)val->CellData.szCellId, argv[index++], sizeof(val->CellData.szCellId));
	strncpy((char *)val->CellData.szSwVer, argv[index++], sizeof(val->CellData.szSwVer));
	strncpy((char *)val->CellData.szHwVer, argv[index++], sizeof(val->CellData.szHwVer));

	DEBUGMSG(1,("CellId:%s, SwVer:%s, HwVer:%s\r\n", 
		val->CellData.szCellId, val->CellData.szSwVer, val->CellData.szHwVer));

	/*iStartPos = index;
	index = ParseAlgo(argc, argv, iStartPos, &val->CellData.eCellAlgo);
	if (index < 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, ParseAlgo error!\r\n",  __FILE__, __FUNCTION__, __LINE__);	
		return 2;
	}*/
	//BTC/LTC/DUAL,0/1/2
	val->CellData.eCellAlgo = strtoul(argv[index++], 0, 0) - 1;

	
	//分配资源空间
	/*if (AllocCellRes(val->CellData.eCellAlgo, &val->pCellRes) < 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, AllocRes error!\r\n",  __FILE__, __FUNCTION__, __LINE__);	
		return 3;
	}*/

	DEBUGMSG(0,("index:%d\r\n", index));
	//strncpy((char *)val->CellData.byCalcPower, argv[index++], sizeof(val->CellData.byCalcPower));
	val->CellData.iChipCnt = strtoul(argv[index++], 0, 0);
	val->CellData.iChipBTCHash = strtoll(argv[index++], 0, 0);
	val->CellData.iChipLTCHash = strtoll(argv[index++], 0, 0);
	val->CellData.iChipBTCFreq = strtoul(argv[index++], 0, 0);
	val->CellData.iChipLTCFreq = strtoul(argv[index++], 0, 0);
	strncpy((char *)val->CellData.szTestResult, argv[index++], sizeof(val->CellData.szTestResult));
	//strncpy((char *)val->CellData.szDevId, argv[index++], sizeof(val->CellData.szDevId));

	strncpy((char *)byCellIp, argv[index++], sizeof(byCellIp));
	iCellPort = strtoul(argv[index++], 0, 0);
	DEBUGMSG(1,("+++ CellIp:%s, CellPort:%d\r\n", byCellIp, iCellPort));
	SetClientIp4(&val->remote.addr4, byCellIp, iCellPort);

	val->CellParent.sock = strtoul(argv[index++], 0, 0);

	DEBUGMSG(1,("+++%s +++%d, sock:%d\r\n", inet_ntoa(val->remote.addr4.sin_addr), val->remote.addr4.sin_port, val->CellParent.sock));	

	//val->CellData.iCellTestNum = atoi(argv[index++]);

	//此处要加事务操作2015-01-07
	//判断是否是新的设备，val->CellData.iCellId = 0表示新的设备
	/*if ((CellIdRtn = ParseCellAllocId(val)) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, CellIdRtn:%d, ParseCellAllocId is error!\r\n",  __FILE__, __FUNCTION__, __LINE__, CellIdRtn);
	}*/
	//else		
	//	DEBUGMSG(1,("CellIdRtn:%d, iCellId:%d\r\n", CellIdRtn, val->CellData.iCellId));
	
	pthread_mutex_lock(pUtList->LoginRemLock);
	//查找CellId	
	//CellNode = pUtList->FindUserInt(pUtList->HashUsers, val->CellData.iCellId);
	CellNode = pUtList->FindUserStr(pUtList->StrHUsers, (char *)val->CellData.szCellId);	
	if (CellNode != NULL)
	{
		DEBUGMSG(0,("+++++++=====new ip:%s, old ip:%s\n", 
			(char *)byCellIp, 
			inet_ntoa(((stCellConn *)CellNode->val)->remote.addr4.sin_addr)));

		DEBUGMSG(0,("+++++++=====new port:%d, old port:%d\n", 
			val->remote.addr4.sin_port, ((stCellConn *)CellNode->val)->remote.addr4.sin_port));
	
		if (strncmp((char *)((stCellConn *)CellNode->val)->CellData.szDevId, (char *)val->CellData.szDevId, sizeof(val->CellData.szDevId)) != 0)
			strncpy((char *)((stCellConn *)CellNode->val)->CellData.szDevId, (char *)val->CellData.szDevId, sizeof(((stCellConn *)CellNode->val)->CellData.szDevId));
		
		DEBUGMSG(0,("%s\r\n", ((stCellConn *)CellNode->val)->CellData.szCellId));
		//DEBUGMSG(0,("1 WbNo:%d, CellNum:%d\r\n", ((stCellConn *)CellNode->val)->pCellRes->iWbNo, ((stCellConn *)CellNode->val)->pCellRes->iConnCellNum));

		pthread_mutex_lock(&((stCellConn *)CellNode->val)->CellDataLock.AlarmTimerLock);
		((stCellConn *)CellNode->val)->CellData.iCellAlarmTimer = GetTime();
		pthread_mutex_unlock(&((stCellConn *)CellNode->val)->CellDataLock.AlarmTimerLock);

		if (strcmp(inet_ntoa(((stCellConn *)CellNode->val)->remote.addr4.sin_addr), (char *)byCellIp) != 0
			|| val->remote.addr4.sin_port != ((stCellConn *)CellNode->val)->remote.addr4.sin_port)
		{
			pthread_mutex_lock(&((stCellConn *)CellNode->val)->CellDataLock.IpPortLock);
			SetClientIp4(&(((stCellConn *)CellNode->val)->remote.addr4), byCellIp, iCellPort);
			pthread_mutex_unlock(&((stCellConn *)CellNode->val)->CellDataLock.IpPortLock);
			bUpdateDb = TRUE;
		}

		//if (val->CellData.iSwVer != ((stCellConn *)CellNode->val)->CellData.iSwVer)
		if (strncmp((char *)val->CellData.szSwVer, (char *)((stCellConn *)CellNode->val)->CellData.szSwVer, sizeof(val->CellData.szSwVer)) != 0)
		{
			//((stCellConn *)CellNode->val)->CellData.iSwVer = val->CellData.iSwVer;
			strcpy((char *)((stCellConn *)CellNode->val)->CellData.szSwVer, (char *)val->CellData.szSwVer);
			bUpdateDb = TRUE;
		}
		//if (val->CellData.iHwVer != ((stCellConn *)CellNode->val)->CellData.iHwVer)
		if (strncmp((char *)val->CellData.szHwVer, (char *)((stCellConn *)CellNode->val)->CellData.szSwVer, sizeof(val->CellData.szHwVer)) != 0)
		{
			//((stCellConn *)CellNode->val)->CellData.iHwVer = val->CellData.iHwVer;
			strcpy((char *)((stCellConn *)CellNode->val)->CellData.szHwVer, (char *)val->CellData.szHwVer);
			bUpdateDb = TRUE;
		}
		if (val->CellData.iChipBTCFreq != ((stCellConn *)CellNode->val)->CellData.iChipBTCFreq)
		{
			((stCellConn *)CellNode->val)->CellData.iChipBTCFreq = val->CellData.iChipBTCFreq;
			bUpdateDb = TRUE;
		}
		if (val->CellData.iChipLTCFreq != ((stCellConn *)CellNode->val)->CellData.iChipLTCFreq)
		{
			((stCellConn *)CellNode->val)->CellData.iChipLTCFreq = val->CellData.iChipLTCFreq;
			bUpdateDb = TRUE;
		}		
		/*pthread_mutex_lock(OnlineLock.pCellOnliLock);
		val->CellData.bCellOnline = TRUE;
		pthread_mutex_unlock(OnlineLock.pCellOnliLock);*/

		//val->eCellMySql = E_QUERY_TYPE_UPDATE;
		//在保证有数据更新且有数据已经插入数据库后，才能进行更新。
		if (bUpdateDb && val->eCellMySql != E_QUERY_TYPE_INSERT)
			((stCellConn *)CellNode->val)->eCellMySql = E_QUERY_TYPE_UPDATE;
		val->eCellComm = E_COMMAND_TYPE_STAT;
		DEBUGMSG(0,("szCellId:%s, eCellMySql:%d\r\n", val->CellData.szCellId, val->eCellMySql));
		//PackCellData(((stCellConn *)CellNode->val), (char *)destBuf, &iLen, sizeof(destBuf));
		//PackCellRepeat(((stCellConn *)CellNode->val), (char *)destBuf, &iLen, sizeof(destBuf));
		//rtnLoginSrc = LoginSrc.pLoginComm[0].CommPackFunc(&LoginSrc, ((stCellConn *)CellNode->val), destBuf, sizeof(destBuf));
		rtnLoginSrc = PackJsonCellRepeat(&LoginSrc, ((stCellConn *)CellNode->val), destBuf, sizeof(destBuf));
		
		free(val);
	}
	else
	{		
		/*pthread_mutex_lock(pUtList->ListLenLock);
		iListLen = pUtList->CountUsersInt(*pUtList->HashUsers);
		pthread_mutex_unlock(pUtList->ListLenLock);*/
		//DEBUGMSG(1,("gLoginResIndex.iCellSum:%d\r\n", gLoginResIndex.iCellSum));
		//if (iListLen < gLoginResIndex.iCellSum)
		//{
			//if ((i = val->CellData.eCellAlgo) < sizeof(gCellAssi)/sizeof(gCellAssi[0]))
		//{
			//DEBUGMSG(1,("i:%d, pCrrAlgoLen:%d, pSumAlgoLen:%d\r\n", i, *gCellAssi[i].pCrrAlgoLen, *gCellAssi[i].pSumAlgoLen));
			//if (*gCellAssi[i].pCrrAlgoLen < *gCellAssi[i].pSumAlgoLen)
		//{
			//pthread_mutex_lock(gResourceLock.CrrCellAlgoLenLock);
			//eLoginErr = IncrCrrentAlgoLen(i, gCellAssi);
			//pthread_mutex_unlock(gResourceLock.CrrCellAlgoLenLock);

			//if (eLoginErr == 0)
		//{
			//gCellAssi[i].ResToAlgoFunc(val, gpResource, gLoginResIndex.iMinerNum);	
			DEBUGMSG(0,("1 ----here \r\n"));
			//计算额定算力
			CalculateRatedKHash(val);
			//PackCellData(val, (char *)destBuf, &iLen, sizeof(destBuf));
			rtnLoginSrc = LoginSrc.pLoginComm[0].CommPackFunc(&LoginSrc, val, destBuf, sizeof(destBuf));

			//初始化
			pthread_mutex_init(&val->CellDataLock.OnlineLock, NULL);
			pthread_mutex_init(&val->CellDataLock.AlarmTimerLock, NULL);
			pthread_mutex_init(&val->CellDataLock.IpPortLock, NULL);
			val->CellData.bCellOnline = TRUE;
			val->eCellComm = E_COMMAND_TYPE_STAT;
			//有新的cell，可以发给dataserver
			val->eCellDataSer = E_DEV_DATAUPDATE_NEWCELL;

			DEBUGMSG(0,("2 ----here \r\n"));
			DEBUGMSG(0,("00000000222222 @val:%p, sin_port:%d\n", val, val->remote.addr4.sin_port));
			//注册时间、登陆时间与告警判断时间
			val->CellData.iRegtime = val->CellData.iLoginTime = val->CellData.iCellAlarmTimer = GetTime(); 

			CellSqlQuery[E_QUERY_TYPE_INSERT - 1].QueryPackFunc1(
													val, 
													&gCfgDataDL.CellBaseInfo, 
													&gCfgDataDL.CellColName, 
													bufDbDest, 
													sizeof(bufDbDest));

			DEBUGMSG(0,("3 ----here \r\n"));
			pthread_mutex_lock(&gCellBaseLock);
			iQueryRtn = MysqlFunc.dbDoQuery(&myCellBase, bufDbDest, E_QUERY_TYPE_INSERT, NULL);
			pthread_mutex_unlock(&gCellBaseLock);
			memset(bufDbDest, 0, sizeof(bufDbDest));
			if (iQueryRtn != 0)
			{
				//如果新结点插入失败，在switch没有重启时，则定时插入此结点。
				val->eCellMySql = E_QUERY_TYPE_INSERT;
			}

			/*if (PackCellNewInfo((void *)&MoniSrc, 
				gCliSendDaTa.bySeCmdBuf, 
				sizeof(gCliSendDaTa.bySeCmdBuf),
				val) == 0)
			{
				stApiNetMonitor.ConnectionPoolSend(
					tcp_client->conns->parent, tcp_client->conns->idx, 
					(char *)gCliSendDaTa.bySeCmdBuf, 
					strlen((char *)gCliSendDaTa.bySeCmdBuf));
			}*/

			
			DEBUGMSG(0,("val->eCellMySql:%d\n", val->eCellMySql));
			//无重复CellId，插入链表
			DEBUGMSG(0,("000000001111111 @val:%p, sin_port:%d\n", val, val->remote.addr4.sin_port));
			//pUtList->AddUserInt(pUtList->HashUsers, val->CellData.iCellId, (void**)&val);
			pUtList->AddUserStr(pUtList->StrHUsers, (char *)val->CellData.szCellId, sizeof(val->CellData.szCellId), (void**)&val);
			rtn = 0;
		//}
		/*	else
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, CellId [%d] BL has exceeded the maximum length set!\r\n",  __FILE__, __FUNCTION__, __LINE__, val->CellData.iCellId);			
				PackCellError(val, (char *)destBuf, &iLen, sizeof(destBuf), eLoginErr - 1);						
			}*/
		//}
			/*else
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, CellId [%d] algo has exceeded the maximum length set!\r\n",  __FILE__, __FUNCTION__, __LINE__, val->CellData.iCellId);			
				PackCellError(val, (char *)destBuf, &iLen, sizeof(destBuf), E_CELL_LOGIN_ERROR_OVERALGO - 1);
			}*/
		//}
		//}
		/*else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, CellId [%d] has exceeded the maximum length set!\r\n",  __FILE__, __FUNCTION__, __LINE__, val->CellData.iCellId);			
			PackCellError(val, (char *)destBuf, &iLen, sizeof(destBuf), E_CELL_LOGIN_ERROR_OVERNUM - 1);
		}*/
	}
	pthread_mutex_unlock(pUtList->LoginRemLock);
	
	DEBUGMSG(1,("4 ----here \r\n"));

	DEBUGMSG(1,("1 iLen:%d, destBuf:%s\r\n", iLen, destBuf));
	if (rtnLoginSrc == 0)
	{
		iLen = strlen((char *)destBuf);
		DEBUGMSG(0,("2 iLen:%d, destBuf:%s\r\n", iLen, destBuf));
		
		if (stApiNetWork.ApiNetSendToClient(val->CellParent.sock, val->remote.addr4, destBuf, iLen, gpLoginPool) < 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, ApiNetSendToClient error!\r\n",  __FILE__, __FUNCTION__, __LINE__);	
		}
		else
		{
			usleep(1);				  //避免出现EAGAIN
		}
	}

	DEBUGMSG(1,("cell destBuf:%s\r\n", destBuf));

	return rtn;
}

#if 0
int AddCellList(stCellConn *val)
{
	int rtn = -1, iQueryRtn = 0;//CellIdRtn = 0, 
	//int index = 0;
	BYTE byCellIp[BUFFER_LEN_16];       //cell的ip	
	int	iCellPort;                      //cell的port	
	int iLen = 0;
	BYTE destBuf[BUFFER_LEN_512] = {0};
	//int iListLen = 0;
	//int i = 0;
	//E_LOGIN_ERROR eLoginErr;
	BOOL bUpdateDb = FALSE;
	char bufDbDest[BUFFER_LEN_512] = {0};
	devlist_ist *CellNode;
	structUthash *pUtList = NULL;

	pUtList = &stCellUtlist;

	pthread_mutex_lock(pUtList->LoginRemLock);
	//查找CellId	
	CellNode = pUtList->FindUserInt(pUtList->HashUsers, val->CellData.iCellId);
	if (CellNode != NULL)
	{
		DEBUGMSG(0,("+++++++=====new ip:%s, old ip:%s\n", 
			(char *)byCellIp, 
			inet_ntoa(((stCellConn *)CellNode->val)->remote.addr4.sin_addr)));

		DEBUGMSG(0,("+++++++=====new port:%d, old port:%d\n", 
			val->remote.addr4.sin_port, ((stCellConn *)CellNode->val)->remote.addr4.sin_port));
	
		if (strncmp((char *)((stCellConn *)CellNode->val)->CellData.szDevId, (char *)val->CellData.szDevId, sizeof(val->CellData.szDevId)) != 0)
			strncpy((char *)((stCellConn *)CellNode->val)->CellData.szDevId, (char *)val->CellData.szDevId, sizeof(((stCellConn *)CellNode->val)->CellData.szDevId));
		
		DEBUGMSG(0,("%d\r\n", ((stCellConn *)CellNode->val)->CellData.iCellId));
		//DEBUGMSG(0,("1 WbNo:%d, CellNum:%d\r\n", ((stCellConn *)CellNode->val)->pCellRes->iWbNo, ((stCellConn *)CellNode->val)->pCellRes->iConnCellNum));

		pthread_mutex_lock(&((stCellConn *)CellNode->val)->CellDataLock.AlarmTimerLock);
		((stCellConn *)CellNode->val)->CellData.iCellAlarmTimer = GetTime();
		pthread_mutex_unlock(&((stCellConn *)CellNode->val)->CellDataLock.AlarmTimerLock);

		if (strcmp(inet_ntoa(((stCellConn *)CellNode->val)->remote.addr4.sin_addr), (char *)byCellIp) != 0
			|| val->remote.addr4.sin_port != ((stCellConn *)CellNode->val)->remote.addr4.sin_port)
		{
			pthread_mutex_lock(&((stCellConn *)CellNode->val)->CellDataLock.IpPortLock);
			SetClientIp4(&(((stCellConn *)CellNode->val)->remote.addr4), byCellIp, iCellPort);
			pthread_mutex_unlock(&((stCellConn *)CellNode->val)->CellDataLock.IpPortLock);
			bUpdateDb = TRUE;
		}

		if (val->CellData.iSwVer != ((stCellConn *)CellNode->val)->CellData.iSwVer)
		{
			((stCellConn *)CellNode->val)->CellData.iSwVer = val->CellData.iSwVer;
			bUpdateDb = TRUE;
		}
		if (val->CellData.iHwVer != ((stCellConn *)CellNode->val)->CellData.iHwVer)
		{
			((stCellConn *)CellNode->val)->CellData.iHwVer = val->CellData.iHwVer;
			bUpdateDb = TRUE;
		}
		if (val->CellData.iChipBTCFreq != ((stCellConn *)CellNode->val)->CellData.iChipBTCFreq)
		{
			((stCellConn *)CellNode->val)->CellData.iChipBTCFreq = val->CellData.iChipBTCFreq;
			bUpdateDb = TRUE;
		}
		if (val->CellData.iChipLTCFreq != ((stCellConn *)CellNode->val)->CellData.iChipLTCFreq)
		{
			((stCellConn *)CellNode->val)->CellData.iChipLTCFreq = val->CellData.iChipLTCFreq;
			bUpdateDb = TRUE;
		}		
		/*pthread_mutex_lock(OnlineLock.pCellOnliLock);
		val->CellData.bCellOnline = TRUE;
		pthread_mutex_unlock(OnlineLock.pCellOnliLock);*/

		//val->eCellMySql = E_QUERY_TYPE_UPDATE;
		//在保证有数据更新且有数据已经插入数据库后，才能进行更新。
		if (bUpdateDb && val->eCellMySql != E_QUERY_TYPE_INSERT)
			((stCellConn *)CellNode->val)->eCellMySql = E_QUERY_TYPE_UPDATE;
		val->eCellComm = E_COMMAND_TYPE_STAT;
		DEBUGMSG(0,("CellId:%d, eCellMySql:%d\r\n", val->CellData.iCellId, val->eCellMySql));
		//PackCellData(((stCellConn *)CellNode->val), (char *)destBuf, &iLen, sizeof(destBuf));
		PackCellRepeat(((stCellConn *)CellNode->val), (char *)destBuf, &iLen, sizeof(destBuf));
		
		free(val);
	}
	else
	{		
		/*pthread_mutex_lock(pUtList->ListLenLock);
		iListLen = pUtList->CountUsersInt(*pUtList->HashUsers);
		pthread_mutex_unlock(pUtList->ListLenLock);*/
		//DEBUGMSG(1,("gLoginResIndex.iCellSum:%d\r\n", gLoginResIndex.iCellSum));
		//if (iListLen < gLoginResIndex.iCellSum)
		//{
			//if ((i = val->CellData.eCellAlgo) < sizeof(gCellAssi)/sizeof(gCellAssi[0]))
		//{
			//DEBUGMSG(1,("i:%d, pCrrAlgoLen:%d, pSumAlgoLen:%d\r\n", i, *gCellAssi[i].pCrrAlgoLen, *gCellAssi[i].pSumAlgoLen));
			//if (*gCellAssi[i].pCrrAlgoLen < *gCellAssi[i].pSumAlgoLen)
		//{
			//pthread_mutex_lock(gResourceLock.CrrCellAlgoLenLock);
			//eLoginErr = IncrCrrentAlgoLen(i, gCellAssi);
			//pthread_mutex_unlock(gResourceLock.CrrCellAlgoLenLock);

			//if (eLoginErr == 0)
		//{
			//gCellAssi[i].ResToAlgoFunc(val, gpResource, gLoginResIndex.iMinerNum);	
			DEBUGMSG(0,("1 ----here \r\n"));
			//计算额定算力
			CalculateRatedKHash(val);
			PackCellData(val, (char *)destBuf, &iLen, sizeof(destBuf));

			//初始化
			pthread_mutex_init(&val->CellDataLock.OnlineLock, NULL);
			pthread_mutex_init(&val->CellDataLock.AlarmTimerLock, NULL);
			pthread_mutex_init(&val->CellDataLock.IpPortLock, NULL);
			val->CellData.bCellOnline = TRUE;
			val->eCellComm = E_COMMAND_TYPE_STAT;

			DEBUGMSG(0,("2 ----here \r\n"));
			DEBUGMSG(0,("00000000222222 @val:%p, sin_port:%d\n", val, val->remote.addr4.sin_port));
			//注册时间、登陆时间与告警判断时间
			val->CellData.iRegtime = val->CellData.iLoginTime = val->CellData.iCellAlarmTimer = GetTime(); 

			CellSqlQuery[E_QUERY_TYPE_INSERT - 1].QueryPackFunc1(
													val, 
													&gCfgDataDL.CellBaseInfo, 
													&gCfgDataDL.CellColName, 
													bufDbDest, 
													sizeof(bufDbDest));

			DEBUGMSG(0,("3 ----here \r\n"));
			iQueryRtn = MysqlFunc.dbDoQuery(&myCellBase, bufDbDest, E_QUERY_TYPE_INSERT, NULL);
			memset(bufDbDest, 0, sizeof(bufDbDest));
			if (iQueryRtn != 0)
			{
				//如果新结点插入失败，在switch没有重启时，则定时插入此结点。
				val->eCellMySql = E_QUERY_TYPE_INSERT;
			}
			DEBUGMSG(0,("val->eCellMySql:%d\n", val->eCellMySql));
			//无重复CellId，插入链表
			DEBUGMSG(0,("000000001111111 @val:%p, sin_port:%d\n", val, val->remote.addr4.sin_port));
			pUtList->AddUserInt(pUtList->HashUsers, val->CellData.iCellId, (void**)&val);
			rtn = 0;
		//}
		/*	else
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, CellId [%d] BL has exceeded the maximum length set!\r\n",  __FILE__, __FUNCTION__, __LINE__, val->CellData.iCellId);			
				PackCellError(val, (char *)destBuf, &iLen, sizeof(destBuf), eLoginErr - 1);						
			}*/
		//}
			/*else
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, CellId [%d] algo has exceeded the maximum length set!\r\n",  __FILE__, __FUNCTION__, __LINE__, val->CellData.iCellId);			
				PackCellError(val, (char *)destBuf, &iLen, sizeof(destBuf), E_CELL_LOGIN_ERROR_OVERALGO - 1);
			}*/
		//}
		//}
		/*else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, CellId [%d] has exceeded the maximum length set!\r\n",  __FILE__, __FUNCTION__, __LINE__, val->CellData.iCellId);			
			PackCellError(val, (char *)destBuf, &iLen, sizeof(destBuf), E_CELL_LOGIN_ERROR_OVERNUM - 1);
		}*/
	}
	pthread_mutex_unlock(pUtList->LoginRemLock);
	
	DEBUGMSG(0,("4 ----here \r\n"));
	DEBUGMSG(0,("iLen:%d, destBuf:%s\r\n", iLen, destBuf));
	if (stApiNetWork.ApiNetSendToClient(val->CellParent.sock, val->remote.addr4, destBuf, iLen, gpLoginPool) < 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, ApiNetSendToClient error!\r\n",  __FILE__, __FUNCTION__, __LINE__);	
	}
	else
	{
		usleep(1);                //避免出现EAGAIN
	}
		
	DEBUGMSG(1,("cell destBuf:%s\r\n", destBuf));

	return rtn;
}
#endif 

int ParseJsonCellLogin(void * pJson, void *dest, int destLen, int *pId)
{
	int rtn = 0;
	cJSON *pJsonParam = NULL;
	cJSON *pSub = NULL;
	int iPos = 0, i = 0;

	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	pJsonParam = cJSON_GetObjectItem((cJSON *)pJson, "param");
	
	if(NULL == pJsonParam)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		//return E_COMMAND_ERROR_JSON;	
		return -1;
	}
	//stCellConn *val = malloc(sizeof(stCellConn));
	//memset(val, 0, sizeof(stCellConn));

	DEBUGMSG(0,("destLen:%d, pId:%s\n", destLen, (char *)pId));
	snprintf((char *)dest+iPos, destLen, "cell_login %s", (char *)pId);
	iPos += strlen((char *)dest+iPos);
	DEBUGMSG(0,("dest:%s\n", (char *)dest));

	for (i = 0; i < dim(szCellLoginParam); i++)
	{
		pSub = cJSON_GetObjectItem(pJsonParam, szCellLoginParam[i]);
		if(NULL == pSub)
		{
			// get "result" from json faild
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, \"sw_ver\" is error!\r\n",  
				__FILE__, __FUNCTION__, __LINE__);
			//free(val);
			return E_LOGIN_ERROR_JSON;;
		}
		else
		{
			//iParamLen = srrlen(szCellLoginParam[i]);
			//iValueLen = strlen(pSub->valuestring);
			//if (iParamLen == iValueLen && strncmp(pSub->valuestring, szCellLoginParam[i]) == 0)
			if (i == 0 || i == 1 || i == 8)
			{
				snprintf((char *)dest+iPos, destLen, " %s", pSub->valuestring);
				DEBUGMSG(0,("%d, dest:%s, val:%s\n", i, (char *)dest+iPos, pSub->valuestring));

			}
			else
			{
				//snprintf((char *)dest+iPos, destLen, " %d", pSub->valueint);
				//DEBUGMSG(0,("%d, dest:%s, val:%d\n", i, (char *)dest+iPos, pSub->valueint));
				snprintf((char *)dest+iPos, destLen, " %llu", (unsigned long long)pSub->valuedouble);
				DEBUGMSG(0,("%d, dest:%s, valuedouble:%llu\n", i,(char *)dest+iPos, (unsigned long long)pSub->valuedouble));
			}
			iPos += strlen((char *)dest+iPos);
		}
	}


    /*pSub = cJSON_GetObjectItem(pJsonParam, "hw_ver");
    if(NULL == pSub)
    {
        // get "result" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"hw_ver\" is error!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);
		//free(val);
		return -1;
    }
	else
	{
		val->CellData.iHwVer = strtoul(pSub->valuestring, 0, 0);
	}

    pSub = cJSON_GetObjectItem(pJsonParam, "algo");
    if(NULL == pSub)
    {
        // get "result" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"algo\" is error!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);
		free(val);
		return -1;
    }
	else
	{
		val->CellData.eCellAlgo = pSub->valueint;
	}

    pSub = cJSON_GetObjectItem((cJSON *)pJson, "chip_cnt");
    if(NULL == pSub)
    {
        // get "result" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"chip_cnt\" is error!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);
		free(val);
		return -1;
    }
	else
	{
		val->CellData.iChipCnt = pSub->valueint;
	}

    pSub = cJSON_GetObjectItem(pJsonParam, "btc_hash");
    if(NULL == pSub)
    {
        // get "result" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"btc_hash\" is error!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);
		free(val);
		return -1;
    }
	else
	{
		val->CellData.iChipBTCHash = pSub->valueint;
	}

    pSub = cJSON_GetObjectItem(pJsonParam, "ltc_hash");
    if(NULL == pSub)
    {
        // get "result" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"ltc_hash\" is error!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);
		free(val);
		return -1;
    }
	else
	{
		val->CellData.iChipLTCHash = pSub->valueint;
	}

    pSub = cJSON_GetObjectItem(pJsonParam, "btc_freq");
    if(NULL == pSub)
    {
        // get "result" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"btc_freq\" is error!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);
		free(val);
		return -1;
    }
	else
	{
		val->CellData.iChipBTCFreq = pSub->valueint;
	}

    pSub = cJSON_GetObjectItem(pJsonParam, "ltc_freq");
    if(NULL == pSub)
    {
        // get "result" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"ltc_freq\" is error!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);
		free(val);
		return -1;
    }
	else
	{
		val->CellData.iChipLTCFreq = pSub->valueint;
	}

    pSub = cJSON_GetObjectItem(pJsonParam, "test_result");
    if(NULL == pSub)
    {
        // get "result" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"test_result\" is error!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);
		free(val);
		return -1;
    }
	else
	{
		strncpy(val->CellData.szTestResult, pSub->valuestring, sizeof(val->CellData.szTestResult));
	}

	if (AddCellList(val) < 0)
	{
		DEBUGMSG(1,("FUCK here!\n"));
		free(val);
	}*/

	return rtn;
}

	
/****************************************************************
** Function name:		ParseJsonLogin
** Descriptions:	

** input parameters:		无
** output parameters:		无
** Returned value:			1:id错误
							2:switchid错误


****************************************************************/
int ParseJsonLogin
(char *pMsg, void *pSrc, int *pIndex, void *dest, int destLen)
{
	int rtn = 0;
	int iParamLen = 0, iMethodLen = 0;
	//int iSize = 0, iCnt = 0;
	int i = 0;//, iSize = -1;
	//int iValue = 0;
	//char delim[] = " ";
	//int PackArgc = 0;
	//char *PackArgv[BUFFER_LEN_256] = {0};
	char szCellIdTemp[BUFFER_LEN_28] = {0};
	//char dest[BUFFER_LEN_4096] = {0};
	//int destLen = sizeof(dest);

	DEBUGMSG(0,("pMsg:%s\n", pMsg));
	
	cJSON * pJson = cJSON_Parse(pMsg);
    //cJSON * root = NULL;

	if (NULL == pJson)                                                                                         
	{
		// parse faild, return
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, pJson is NULL!\r\n",  __FILE__, __FUNCTION__, __LINE__);	      
		return E_LOGIN_ERROR_JSON;
	}

	cJSON * pSub = NULL;

    /*cJSON * pSub = cJSON_GetObjectItem(pJson, "id");
    if(NULL == pSub)
    {
    	// get "id" from json faild
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"id\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return -1;
    }
	else
	{
		if (pSub->valueint != *((stLoginSrc*)pSrc)->pIdSeq)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, command is error, id:%d != %d!\r\n",  
				__FILE__, __FUNCTION__, __LINE__, pSub->valueint, *((stLoginSrc*)pSrc)->pIdSeq);
			return 1;
		}
	}*/

    pSub = cJSON_GetObjectItem(pJson, "cellid");
    if(NULL == pSub)
    {
        // get "switchid" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"cellid\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return -1;
    }
	else
	{
		strncpy(szCellIdTemp, pSub->valuestring, sizeof(szCellIdTemp));
	}

    // get "method" from json
    pSub = cJSON_GetObjectItem(pJson, "method");
    if(NULL == pSub)
    {
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"method\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);   
		return E_LOGIN_ERROR_JSON;
    }
	else
	{
		iParamLen = strlen(pSub->valuestring);

		for (i = 0; i < ((stLoginSrc*)pSrc)->iCommLen; i++)
		{
			iMethodLen = strlen((char *)((stLoginSrc*)pSrc)->pLoginComm[i].cmd);
			if (iMethodLen == iParamLen && 
				strncasecmp((char *)((stLoginSrc*)pSrc)->pLoginComm[i].cmd, pSub->valuestring, iMethodLen) == 0)
			{	
				*pIndex = i;
				break;
			}
		}
		DEBUGMSG(1,("valuestring:%s\n", pSub->valuestring));
		//没有对应的命令
		DEBUGMSG(1,("i:%d, iCommLen:%d\n", i, ((stLoginSrc*)pSrc)->iCommLen));
		if (i == ((stLoginSrc*)pSrc)->iCommLen)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, \"method\" param is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);   
			return E_LOGIN_ERROR_JSON;
		}
		//return 0;
	}

	DEBUGMSG(1,("*pIndex:%d < iCommLen:%d\n", *pIndex, ((stLoginSrc*)pSrc)->iCommLen));
	//ParseJsonCellLogin
	rtn = ((stLoginSrc*)pSrc)->pLoginComm[*pIndex].UserCmdFunc(pJson, dest, destLen, (void *)szCellIdTemp);
	
	DEBUGMSG(1,("rtn:%d\n", rtn));
	DEBUGMSG(0,("1 *pIndex:%d, dest:%s\n", *pIndex, (char *)dest));
	/*if ( *pIndex < ((stLoginSrc*)pSrc)->iCommLen) 
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
				if (*pIndex > 1)
				{
					//PackConfirm(pSrc, *pIndex, pConn);
				
					//清除command接受返回过来的错误id
					//memset(CommMonitorFlag.szIdbufRtn, 0, sizeof(CommMonitorFlag.szIdbufRtn));
					//CommMonitorFlag.iBufPos = 0;
					rtn = ((stLoginSrc*)pSrc)->pLoginComm[*pIndex].CommPackFunc(
							PackArgc, PackArgv, 
							(void *)((stLoginSrc*)pSrc)->szParseDest, 
							sizeof(((stLoginSrc*)pSrc)->szParseDest),
							&ParData);
					//rtn = PackCellSetLedMonitorToComm(PackArgc, PackArgv, (void *)((stLoginSrc*)pSrc)->szParseDest, sizeof(((stLoginSrc*)pSrc)->szParseDest));

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
	}*/

    cJSON_Delete(pJson);

	return rtn;
}


/*******************************************************************************
** Function name:      LoginProcess
** Descriptions:   	

** input parameters:   	无
** output parameters:   	无
** Returned value:     	无
********************************************************************************/
void *LoginParseProcess(void *arg)
{
	int i = 0, rtnPar = 0;
	void *data;
	char * string = NULL;
	int argc = 0;
	char *argv[BUFFER_LEN_1024] = {0};
	char delim[] = " ";
	int iIndex = 0;
	char dest[BUFFER_LEN_4096] = {0};
	int destLen = sizeof(dest), iPos = 0;

	
#ifdef _DEBUG_MANY_DEV_
	char *Idtemp[] = {"200","10000"};                //测试用
#endif	
	DEBUGMSG(1,("2 %s...\r\n", __FUNCTION__));
	
	while(1)
	{
		DEBUGMSG(0,("1 ^^^^^^\r\n"));
		//sleep(5);
		//if (gpLoginPool == NULL)
		//	continue;
		
		/*if ((data = LoginFifoQueue.QuePop(gpLoginQue, LoginFifoQueue.QueLock)) == NULL)
		{
			DEBUGMSG(0,("2 ^^^^^^\r\n"));
			sleep(1);
			LoginFifoQueue.QueFree(data);
			continue;
		}*/

		if ((data = CirFifoQueue.QuePop(gpLoginQue)) == NULL)
		{
			DEBUGMSG(0,("2 ^^^^^^\r\n"));
			dbgdumpLoginSend();
			bStaTime = TRUE;
			//bLoginQueValue = FALSE;
			MSleep(1000);
			//CommandCirFifoQueue.QueFree(data);
			continue;
		}
		else
		{
			//bLoginQueValue = TRUE;
			if (bStaTime)
			{
				gLoginStartTime = GetTime();
				bStaTime = FALSE;
			}
		}

			
		string = (char *)data;
		DEBUGMSG(0,("string:%s\r\n", string));
		if (Parse(string,&argc,argv, delim) != 0)
		{
			LoginFifoQueue.QueFree(data);
			continue;
		}

		if (argc < COMMANDLINE_3)
			continue;
		
		//for (i = 0; i < argc; i++)
		//	DEBUGMSG(1,("*****111 [%d] %s\r\n", i, argv[i]));
		
		rtnPar = ParseJsonLogin(argv[0], (void *)&LoginSrc, &iIndex, dest, destLen);
		if (rtnPar != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, \"Parse JsonLogin\" is error!rtnPar:%d\r\n",  
				__FILE__, __FUNCTION__, __LINE__, rtnPar);		
			continue;
		}
		DEBUGMSG(0,("iIndex:%d\n", iIndex));
		DEBUGMSG(0,("2 dest:%s\n", dest));
		//ip port
		iPos = strlen(dest);
		DEBUGMSG(0,("iPos:%d, argc:%d\n", iPos, argc));
		for (i = 1; i < argc; i++)
		{
			snprintf(dest + iPos, destLen, " %s", argv[i]);
			iPos += strlen(dest + iPos);
		}

		DEBUGMSG(1,("3 dest:%s\n", dest));

		for (i = 0; i < argc; i++)
		{
			DEBUGMSG(0,("***%p, %s\r\n", *(argv+i), *(argv+i)));
			memset(argv+i, 0, sizeof(*argv));
		}
		argc = iPos = 0;
		
		if (Parse(dest,&argc,argv, delim) != 0)
		{
			LoginFifoQueue.QueFree(data);
			continue;
		}
		
		//for (i = 0; i < argc; i++)
		//	DEBUGMSG(1,("*****222 %s\r\n", argv[i]));

		pthread_mutex_lock(&gLoginMonitorLock);
		for (i = 0; i < dim(loginTypeInput); i++)
		{
			if (strncmp(argv[0], (char *)loginTypeInput[i].byLoginDevType, sizeof(loginTypeInput[i].byLoginDevType)) != 0)
			{
				pthread_mutex_unlock(&gLoginMonitorLock);
				continue;
			}

		
			if (loginTypeInput[i].LoginFunc(argc, argv, 1) != 0)
			{
				pthread_mutex_unlock(&gLoginMonitorLock);
				continue;
			}
		}
		pthread_mutex_unlock(&gLoginMonitorLock);

		
		//测试用
#ifdef _DEBUG_MANY_DEV_		
		if (((strcmp(argv[1], Idtemp[0]) == 0) || (strcmp(argv[1], Idtemp[1]) == 0))&& argc >= 2)
		{
			dbgdumpLoginRecv();		
		}
#endif		
		//ap_log_debug_log("queue_pop: %s, @%p\r\n", string, data);		
		for (i = 0; i < argc; i++)
		{
			DEBUGMSG(1,("***%p, %s\r\n", *(argv+i), *(argv+i)));
			memset(argv+i, 0, sizeof(*argv));
		}

		//for (i = 0; i < argc; i++)
		//	DEBUGMSG(1,("*****333 %p, %s\r\n", *(argv+i), *(argv+i)));
		
		argc = 0;
		LoginFifoQueue.QueFree(data);
#ifndef _DEBUG_MANY_DEV_
		dbgdumpLoginRecvLen();		//测试用
#endif		
		//sleep(1);
	}
}

/*void *LoginRecvProcess(void *arg)
{
	DEBUGMSG(1,("2 %s...\r\n", __FUNCTION__));
	//DEBUGMSG(0,("Login sock:%d\r\n", gpLoginPool->listener.sock));
	//int rtn = 0;
	
	while(1)
	{
		if (stApiNetWork.ApiNetConnPoolPoll(gpLoginPool) != 0)
		{
			//MSleep(10);
			continue;	
		}
		//DEBUGMSG(1,("2 %s...\r\n", __FUNCTION__));
		//sleep(1);
		//rtn = LoginQueWrite(gpLoginQue, gpLoginPool->recv_buf);
		//if (LoginQueWrite(gpLoginQue, gpLoginPool->recv_buf) < 0)
		//{
		//	GetLocalTimeLog();
		//	ap_log_debug_log("\t\t%s, %s, %d, CommandQueWrite error!\r\n",  
		//						__FILE__, __FUNCTION__, __LINE__);
		//}

		if (LoginCirFifoQueue.QuePush(gpLoginQue, (void *)gpLoginPool->recv_buf) != 0)
		{
			//LoginError++;
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, cirqueue_push error!\r\n", __FILE__, __FUNCTION__, __LINE__);
		}
		//else
		//{
		//	LoginCount++;
		//}

		memset(gpLoginPool->recv_buf, 0, sizeof(gpLoginPool->recv_buf));
	}
}*/

void dbgdumpLoginRecvLen(void)
{
	//printf("---Miner len:%d\r\n", stMinerUtlist.CountUsersInt(*stMinerUtlist.HashUsers));

	printf("---Cell len:%d\r\n\r\n", stCellUtlist.CountUsersStr(*stCellUtlist.StrHUsers));
}

void dbgdumpLoginRecv(void)
{
	long Interval = 0;                 //秒

	printf("------------\r\n");
	timeprintf();
	Interval = GetTime() - gLoginStartTime;
	bStaTime = TRUE;

	//printf("---MinerList.len:%d\r\n", stMinerUtlist.CountUsersInt(*stMinerUtlist.HashUsers));

	printf("---CellList.len:%d\r\n\r\n", stCellUtlist.CountUsersStr(*stCellUtlist.StrHUsers));

	printf("LoginRecv---Count:%d, Error:%d, fdError:%d\r\n", gpLoginPool->Recv.Count, gpLoginPool->Recv.Error, gpLoginPool->Recv.fdError);

	printf("%d num took time %ld S\r\n",  gpLoginPool->Recv.Count - 1, Interval);

	gpLoginPool->Recv.Count = gpLoginPool->Recv.Error = gpLoginPool->Recv.fdError = 0;
	gbLoginsendOK = TRUE;
}

void dbgdumpLoginSend(void)
{	
	if (gbLoginsendOK)
	{
		printf("------------\r\n");
		printf("LoginSend---Count:%d, Error:%d, Error1:%d\r\n", gpLoginPool->Send.Count, gpLoginPool->Send.Error, gpLoginPool->Send.Error1);
		gbLoginsendOK = FALSE;
		gpLoginPool->Send.Count = gpLoginPool->Send.Error = gpLoginPool->Send.Error1 = 0;
	}
}

