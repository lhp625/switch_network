/***********************************************************************
* $Id$
* Project:	 switch_network
* File:		 api_comm/api_command.c		 
* Description: create control command 
*
* @version:		V1.0.0
* @date:		5th September 2014
*
* Copyright (C) 2014 SFARDS Co., Ltd. All rights reserved.
*-----------------------------------------------------------------
*
* Revise:  
* 		 date		 version 		author 		description
*  		2014/09/05	 1.0.0		zhengchao	create control command
*
*
*
**********************************************************************/
#include <stdio.h>
#include <math.h>
#include "./api_command.h"
#include "../app/debug.h"
#include "../list/list.h"
#include "../db_op/db_mysql.h"
#include "../cJSON/cJSON.h"

stApiNetConnPool *gpCommandPool = NULL;
int gCommandPort = 50000;
stQueue *gpCommandQue = NULL;
long gCommStartTime = 0;
BOOL gbCommRecvOK = FALSE, bCommStaTime = TRUE;
//pthread_mutex_t gCommandQueLock = PTHREAD_MUTEX_INITIALIZER;
//int gMinerCommandIndex = 0;
//int gCellCommandIndex = 0;

//临时命令类型
int gCommandType = 0;

char szHead[][BUFFER_LEN_8] = 
{
	{"list"},
	{"user"},
	{"initall"}
};

/*stIdCommandPack idMinerControlInput[] = 
{
	{szHead, "get_stat", PackMinerGetStatCommand, SendMinerAll, SendMinerSingle, gMinerSrcBuf[0], gMinerId[0], NoParseShellCommand},
	{szHead, "relogin", PackMinerReloginCommand, SendMinerAll, SendMinerSingle, gMinerSrcBuf[2], gMinerId[2], ParseShellMinerReloginCommand},						
	{szHead, "reboot", PackMinerRebootCommand, SendMinerAll, SendMinerSingle, gMinerSrcBuf[1], gMinerId[1], NoParseShellCommand},
	{szHead, "set_algo", PackMinerAlgoCommand, SendMinerAll, SendMinerSingle, gMinerSrcBuf[3], gMinerId[3], ParseShellMinerSetAlgoCommand},    
    {szHead, "get_detail", PackMinerGetdetailCommand, SendMinerAll, SendMinerSingle, gMinerSrcBuf[4], gMinerId[4], ParseShellMinerGetdetailCommand}   
};

int iMinerCommandLen = sizeof(idMinerControlInput);*/

stIdCommandPack idCellControlInput[] =
{
	//get_stat
	{szHead, "getstat", PackJsonCellGetStatCommand, SendCellAll, SendCellSingle, gCellSrcBuf[0],gCellId[0], NoParseShellCommand},
	{szHead, "relogin", JsonPackNoParamCommand, SendCellAll, SendCellSingle, gCellSrcBuf[1], gCellId[1], ParseShellCellReloginCommand},			
	{szHead, "reboot", JsonPackRebootCommand, SendCellAll, SendCellSingle, gCellSrcBuf[2], gCellId[2], ParseShellCellRebootCommand},
	{szHead, "restore", JsonPackNoParamCommand, SendCellAll, SendCellSingle, gCellSrcBuf[3], gCellId[3], ParseShellCellResetFlashCommand},
	{szHead, "setled", JsonPackSetLedCommand, SendCellAll, SendCellSingle, gCellSrcBuf[4], gCellId[4], ParseShellCellSetLedCommand},
	{szHead, "setpool", JsonPackSetPool, SendCellAll, SendCellSingle, gCellSrcBuf[5], gCellId[5], ParseShellSetPoolCommand},
	//{szHead, "get_temperature", PackCellTemperatureCommand, SendCellAll, SendCellSingle, gCellSrcBuf[7], gCellId[7], ParseShellCellGetTemperatureCommand},
	//{szHead, "add_miner", PackCellAddMinerCommand, SendCellAll, SendCellSingle, gCellSrcBuf[4], gCellId[4], NoParseShellCommand},
	//{szHead, "del_miner", PackCellDelMinerCommand, SendCellAll, SendCellSingle, gCellSrcBuf[5], gCellId[5], NoParseShellCommand},
	//{szHead, "update_sw", PackCellUpdateSwCommand, SendCellAll, SendCellSingle, gCellSrcBuf[8], gCellId[8], ParseShellCellTftpCommand},
	//{szHead, "start_test", PackCellStartTestCommand, SendCellAll, SendCellSingle, gCellSrcBuf[7], gCellId[7], ParseShellCellStartTestCommand},
	//{szHead, "get_test_result", PackCellGetTestCommand, SendCellAll, SendCellSingle, gCellSrcBuf[8], gCellId[8], ParseShellCellGetTestResultCommand},
	//{szHead, "mcu", PackCellGetTestResultCommand, SendCellAll, SendCellSingle, gCellSrcBuf[9], gCellId[9], ParseShellCellCustomCommand},
	{szHead, "setfreq", JsonPackSetFreqCommand, SendCellAll, SendCellSingle, gCellSrcBuf[6], gCellId[6], ParseShellCellSetFreqCommand},
	
};

int iCellCommandLen = sizeof(idCellControlInput);
int gCellrTimer[sizeof(idCellControlInput)] = {0};

const stIdControlInput idControlInput[] = 
{
	//{"miner", &stMinerUtlist, &stApiNetWork, idMinerControlInput, &iMinerCommandLen, CommShellFlag.bMinerShellFlag, giSendMinerCommTimer},
	{"cell", &stCellUtlist, &stApiNetWork, idCellControlInput, &iCellCommandLen, CommShellFlag.bCellShellFlag, giSendCellCommTimer, CommMonitorFlag.bCellMonitorFlag},
};

stCommandErr gCommandErr[] = 
{
	{"Type error!", NULL},
	{"Command layer, number of parameters is error,or please check the end of the space!", NULL},
	{"Command layer, ID does not exist!", NULL},
	{"The fourth parameter error!", NULL},
	{"The fifth parameter error!", NULL},
	{"Command layer, no such algorithm!", NULL},
	{"No such reboot function!", NULL},
	{"No such LED function!", NULL},
	{"Command layer, list is empty!", NULL},
	{"No error!", NULL},
	{"Get a error Json or command!", NULL},
	{"Operate pool error!", NULL},
	{"Id number of errors", NULL}
};

stCommandErr gCommParseErr[] =
{
	{"Alarming!Number of parameters is error,or please check the end of the space!", NULL},
	{"The first custom parameter error !", NULL},
	{"The second custom parameter error !", NULL},	
	{"The third custom parameter error !", NULL},	
	{"User serial number error !", NULL}
};

stReboot gReboot[] = 
{
	{"all"},
	{"mcu"},
	//{"power"},
	{"fpga"},
	{"bd1"},
	{"bd2"},
	{"bd3"},
	//{"bd4"},
	//{"bd5"}
};

stSetLed gSetLed[] =
{
	{"on"},
	{"off"},
	{"flash"},
	{"normal"}
};

stWorkProc gWorkProc[] = 
{
	{&gpLoginQue, &gpLoginPool},
	{&gpCommandQue, &gpCommandPool}
};

char szRetBuf[][BUFFER_LEN_16] = 
{
	{"Success"},
	{"offline"},
	{"AllInterruption"},
	{"notexist_pool"},
	{"notexist_cell"}
};

int AlgoEnumToString(E_ALGO eAlgo, char *dest, int destLen)
{
	int iIndex = 0;
	int rtn = 0;

	iIndex = eAlgo;
	switch (eAlgo)
	{
	case E_ALGO_BTC:			
	case E_ALGO_LTC:
	case E_ALGO_DUAL:	
		strncpy(dest, (char *)((Algo + iIndex)->algo), destLen);
		break;
		
	default:
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Miner eAlgo[%d] does not exist.\r\n",  __FILE__, __FUNCTION__, __LINE__, eAlgo);		
		rtn = -1;
		break;
	}
	
	return rtn;
}

/*int CommandQueWrite(stQueue *que, char *buf)
{
	//char *mess = NULL;
	
	//CommandCirFifoQueue.QueMalloc(&mess, BUFFER_LEN_256);
	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));
	//sprintf(mess, "%s", buf);
	
	if (CommandCirFifoQueue.QuePush(que, (void *)buf) != 0)
	{
		ap_log_debug_log("%s, queue_push error\r\n", __FUNCTION__);
		return -1;
	}

	return 0;
}*/

stCommandSrc CommandSrc = {0}; 
UINT gCommandIdSeq = 1;                                 //全局变量，每发送一包序列号加1
CHAR CommandBuf[BUFFER_LEN_512];
stJsonCommand CommandComm[] = 
{
	{"getstat", JsonParseStat},
	{"reboot", JsonParseResult},
	{"relogin", JsonParseResult},
	{"setled", JsonParseResult},
	{"setfreq", JsonParseResult},
	{"setpool", JsonParseResult},
	{"restore", JsonParseResult},
	{"setpool", JsonParseResult}
};

char szCellStatParam[][BUFFER_LEN_16] = 
{
	{"entry_temp"},
	{"exit_temp"},
	{"led_stat"},
	{"elapse"},
	{"btc_send"},
	{"btc_accept"},	
	{"btc_reject"},
	{"ltc_send"},
	{"ltc_accept"},
	{"ltc_reject"},
	{"chip_temp"}
};

void InitCommand()
{
	int i = 0;
	
    gpCommandPool = stApiNetWork.ApiNetConnPoolCreate(API_NET_POOL_FLAGS_ASYNC, MAX_PER_CLIENT);
    if (gpCommandPool == NULL
        || ! stApiNetWork.ApiNetSetIp4Addr(&gpCommandPool->listener.addr4, INADDR_ANY, gCommandPort)
        || -1 == stApiNetWork.ApiNetConnPoolListenerCreate(gpCommandPool, 1, 1))
    {
        printf("* !ERROR: udp_pool: %s\r\n", ap_error_get_string());
        exit(1);
    }
	DEBUGMSG(0,("3 %s... 1111\r\n", __FUNCTION__));

	if (gpPoller == NULL)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, gpPoller error!\r\n", __FILE__, __FUNCTION__, __LINE__);
		exit(1);
	}
	else
	{	
		stApiNetWork.ApiNetPollControl(gpCommandPool->listener.sock, gpPoller);
	}

	gpCommandQue = CirFifoQueue.QueCreate(MAX_BUF_NUM, BUFFER_LEN_512);
	//gpMinerStatusDbQue = CirFifoQueue.QueCreate(MAX_BUF_NUM, BUFFER_LEN_512);
	//gpCellStatusDbQue = CirFifoQueue.QueCreate(MAX_BUF_NUM, BUFFER_LEN_512);
	
	DEBUGMSG(0,("3 %s... 2222\r\n", __FUNCTION__));
	//CommandFifoQueue.QueLock = &gCommandQueLock;

	for (i = 0; i < MAX_MINER_COMMAND_NUM; i++)
	{
		//gbMinerFlag[i] = FALSE;
		giSendMinerCommTimer[i] = GetTime();
		DEBUGMSG(0,("giSendMinerCommTimer[%d]:%ld\r\n", i, giSendMinerCommTimer[i]));
	}
	
	for (i = 0; i < MAX_CELL_COMMAND_NUM; i++)
	{
		//gbCellFlag[i] = FALSE;
		giSendCellCommTimer[i] = GetTime();
	}
	//memset(gMinerSrcBuf, 0, sizeof(gMinerSrcBuf));
	memset(gCellSrcBuf, 0, sizeof(gCellSrcBuf));
	memset(gMinerId, 0, sizeof(gMinerId));
	memset(gCellId, 0, sizeof(gCellId));



	CommandSrc.pIdSeq = &gCommandIdSeq;
	CommandSrc.pBuf = CommandBuf;
	CommandSrc.pList = &stCellUtlist;
	CommandSrc.pCommandComm = CommandComm;
	CommandSrc.iCommLen = dim(CommandComm);

	

	/*for (i = 0; i < MAX_MINER_COMMAND_NUM; i++)
	{
		DEBUGMSG(1,("\r\n%p\r\n", gMinerSrcBuf[i]));
		DEBUGMSG(1,("22222222222222 ################pSrcBuf:%p\r\n", idControlInput[0].pIdCommandPack[i].pSrcBuf));
	}
	for (i = 0; i < MAX_MINER_COMMAND_NUM; i++)
		DEBUGMSG(1,("1 giSendMinerCommTimer[%d]:%ld\r\n", i, giSendMinerCommTimer[i]));*/	
}

/*int PackMinerGetStatCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{	
	return (*pLen = snprintf(dest, destLen, "%s,%s %d %s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+0), 
								"miner",
								((stMinerConn*)pCoon)->MinerData.iMinerId,
								((stIdCommandPack *)pCommand)->pstrControl
							 )
			);
}

int PackMinerAlgoCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	//AlgoEnumToString(((stMinerConn*)pCoon)->MinerData.eMinerAlgo, (char *)destBuf, sizeof(destBuf));
	return (*pLen = snprintf(dest, destLen, "%s,%s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+1),
								((stIdCommandPack *)pCommand)->pSrcBuf
								//(*((stIdCommandPack *)pCommand)->pSendId),
								//((stIdCommandPack *)pCommand)->pstrControl,
								//destBuf
							 )
			);
}

int PackMinerReloginCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{
	return (*pLen = snprintf(dest, destLen, "%s,%s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+1), 
								((stIdCommandPack *)pCommand)->pSrcBuf
							 )
			);
}

int PackMinerRebootCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	return (*pLen = snprintf(dest, destLen, "%s,%s %d %s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+2), 
								"miner",
								((stMinerConn*)pCoon)->MinerData.iMinerId,
								((stIdCommandPack *)pCommand)->pstrControl
							 )
			);
}


int PackMinerGetdetailCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{
	return (*pLen = snprintf(dest, destLen, "%s,%s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+1), 
								((stIdCommandPack *)pCommand)->pSrcBuf
							 )
			);
}
*/

//int CalculateBTCRealKHash(int argc, char **argv, int iStartPos, stCellConn *pCellConn)
int CalculateBTCRealKHash(LLONG ullBTCNcsend, stCellConn *pCellConn)
{
	int iBTCIndex = 0;
	//int iIndex = 0;
	//ULLONG ullDifference = 0;
	LLONG llDifference = 0;

	//abs(llDifference)
	//iIndex = iStartPos;
	//DEBUGMSG(1,("%s..., argv[%d]:%s\n", __FUNCTION__, iIndex, argv[iIndex]));
	DEBUGMSG(1,("%s..., ullBTCNcsend:%llu\n", __FUNCTION__, ullBTCNcsend));
	//实际算力计算方法，单位：khash/s，iBTCRealKHash=(|lBTCNcsend[Crr] - lBTCNcsend[30M before]| * BTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000)
	iBTCIndex = pCellConn->CellStat.iBTCNcIndex;
	if (iBTCIndex < INTERVAL_30_SECOND)
	{
		//iIndex = 8
		pCellConn->CellStat.llBTCNcsend[iBTCIndex] = ullBTCNcsend;//strtoull(argv[iIndex], 0, 16);
		
		if (pCellConn->CellStat.llBTCNcsend[iBTCIndex+1] != 0 && iBTCIndex != INTERVAL_30_SECOND - 1)
		{	
			llDifference = pCellConn->CellStat.llBTCNcsend[iBTCIndex+1] - pCellConn->CellStat.llBTCNcsend[iBTCIndex];

			DEBUGMSG(0,("1 llDifference:%lld\n", llDifference));
			if (llDifference < 0)
				llDifference = HASH_MAX_RANGE + llDifference;
			DEBUGMSG(0,("2 llDifference:%lld\n", llDifference));
			
			//pCellConn->CellStat.llBTCRealKHash = 
			//	(abs(llDifference) * BTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000);
		}
		else
		{
			llDifference = pCellConn->CellStat.llBTCNcsend[iBTCIndex] - pCellConn->CellStat.llBTCNcsend[0];

			DEBUGMSG(0,("3 llDifference:%lld\n", llDifference));
			if (llDifference < 0)
				llDifference = HASH_MAX_RANGE + llDifference;
			DEBUGMSG(0,("4 llDifference:%lld\n", llDifference));

			//pCellConn->CellStat.llBTCRealKHash =
			//	(abs(llDifference) * BTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000);
		}
	
		pCellConn->CellStat.iBTCNcIndex++;
	}
	else
	{
		//iIndex = 8
		pCellConn->CellStat.llBTCNcsend[0] = ullBTCNcsend;//strtoull(argv[iIndex], 0, 16);
		//ullDifference = abs(HASH_MAX_RANGE - pCellConn->CellStat.llBTCNcsend[INTERVAL_30_SECOND - 1]);

		llDifference = pCellConn->CellStat.llBTCNcsend[0] - pCellConn->CellStat.llBTCNcsend[INTERVAL_30_SECOND - 1];

		DEBUGMSG(0,("5 llDifference:%lld\n", llDifference));
		if (llDifference < 0)
			llDifference = HASH_MAX_RANGE + llDifference;
		DEBUGMSG(0,("6 llDifference:%lld\n", llDifference));
	
		//pCellConn->CellStat.llBTCRealKHash =
		//	(abs(llDifference) * BTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000);
	
		pCellConn->CellStat.iBTCNcIndex = 0;
	}

	pCellConn->CellStat.llBTCRealKHash = 
		(abs(llDifference) * BTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000);

	DEBUGMSG(0,("&&&&&&&&&&&&&\nllBTCRealKHash:%llu\n", pCellConn->CellStat.llBTCRealKHash));
	
	return 0;
}

int CalculateLTCRealKHash(LLONG ullLTCNcsend, stCellConn *pCellConn)
{
	int iLTCIndex = 0;
	//int iIndex = 0;
	//ULLONG ullDifference = 0;
	LLONG llDifference = 0;

	//iIndex = iStartPos;

	//DEBUGMSG(1,("%s..., argv[%d]:%s\n", __FUNCTION__, iIndex, argv[iIndex]));
	DEBUGMSG(1,("%s..., ullLTCNcsend:%llu\n", __FUNCTION__, ullLTCNcsend));
	
	//实际算力计算方法，单位：khash/s，iLTCRealKHash=(|lLTCNcsend[Crr] - lLTCNcsend[30M before]| * LTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000)
	iLTCIndex = pCellConn->CellStat.iLTCNcIndex;
	if (iLTCIndex < INTERVAL_30_SECOND)
	{
		//iIndex = 11
		pCellConn->CellStat.llLTCNcsend[iLTCIndex] = ullLTCNcsend;//strtoull(argv[iIndex], 0, 16);
		
		if (pCellConn->CellStat.llLTCNcsend[iLTCIndex+1] != 0 && iLTCIndex != INTERVAL_30_SECOND - 1)
		{	
			llDifference = pCellConn->CellStat.llLTCNcsend[iLTCIndex+1] - pCellConn->CellStat.llLTCNcsend[iLTCIndex];

			DEBUGMSG(0,("1 llDifference:%lld\n", llDifference));
			if (llDifference < 0)
				llDifference = HASH_MAX_RANGE + llDifference;
			DEBUGMSG(0,("2 llDifference:%lld\n", llDifference));
		
			//pCellConn->CellStat.llLTCRealKHash =
			//	(abs(llDifference) * LTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000);
		}
		else
		{
			llDifference = pCellConn->CellStat.llLTCNcsend[iLTCIndex] - pCellConn->CellStat.llLTCNcsend[0];

			DEBUGMSG(0,("3 llDifference:%lld\n", llDifference));
			if (llDifference < 0)
				llDifference = HASH_MAX_RANGE + llDifference;
			DEBUGMSG(0,("4 llDifference:%lld\n", llDifference));
			
			//pCellConn->CellStat.llLTCRealKHash =
			//	(abs(llDifference) * LTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000);
		}

		pCellConn->CellStat.iLTCNcIndex++;
	}
	else
	{
		//iIndex = 11
		//ullDifference = abs(HASH_MAX_RANGE - pCellConn->CellStat.llLTCNcsend[INTERVAL_30_SECOND - 1]);

		pCellConn->CellStat.llLTCNcsend[0] = ullLTCNcsend;//strtoull(argv[iIndex], 0, 16);
		llDifference = pCellConn->CellStat.llLTCNcsend[0] - pCellConn->CellStat.llLTCNcsend[INTERVAL_30_SECOND - 1];

		DEBUGMSG(0,("5 llDifference:%lld\n", llDifference));
		if (llDifference < 0)
			llDifference = HASH_MAX_RANGE + llDifference;
		DEBUGMSG(0,("6 llDifference:%lld\n", llDifference));

		//pCellConn->CellStat.llLTCRealKHash =
		//	(abs(llDifference) * LTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000);
	
		pCellConn->CellStat.iLTCNcIndex = 0;
	}

	pCellConn->CellStat.llLTCRealKHash =
		(abs(llDifference) * LTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000);	
	
	DEBUGMSG(0,("############\nllLTCRealKHash:%llu\n", pCellConn->CellStat.llLTCRealKHash));	
	
	return 0;
}


int PackCellGetStatCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{	
	return (*pLen = snprintf(dest, destLen, "%s,%s %s %s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+0), 
								"cell",
								((stCellConn*)pCoon)->CellData.szCellId,
								((stIdCommandPack *)pCommand)->pstrControl
							 )
			);
}
	
int PackCellRebootCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{
	int argc = 0;
	char *argv[BUFFER_LEN_64] = {0};
	char delim[] = " ";
	char destBuf[BUFFER_LEN_256] = {0};
	int iHeadIndex = 0;

	DEBUGMSG(0,("%s..., iCellId:%d, bMassFlag:%d, pSrcBuf:%s\r\n", 
		__FUNCTION__, ((stCellConn*)pCoon)->CellData.iCellId, CommShellFlag.bMassFlag, ((stIdCommandPack *)pCommand)->pSrcBuf));

	//再次解析出 uSeqId和all/mcu/fpga/bd1/bd2/bd3
	strncpy(destBuf, ((stIdCommandPack *)pCommand)->pSrcBuf, sizeof(destBuf));
	
	if (Parse(destBuf, &argc, argv, delim) != 0)
	{
		return 0;
	}
	
	if (argc != COMMANDLINE_2)
		return 0;
	
	DEBUGMSG(1,("*******pSrcBuf:%s, argc:%d, argv[0]:%s, argv[1]:%s\n", 
		((stIdCommandPack *)pCommand)->pSrcBuf, argc, argv[0], argv[1]));
	
	
	//把用户序列号保存下来，接收时比较判断
	((stCellConn*)pCoon)->uSeqId = strtoul(argv[0], 0, 0);
	DEBUGMSG(1,("*******uSeqId:%d, \n", ((stCellConn*)pCoon)->uSeqId));

	if (CommShellFlag.bMassFlag || CommMonitorFlag.bMassFlag)
	{
		iHeadIndex = 2;
	}
	else
	{
		iHeadIndex = 1;
	}

	*pLen = snprintf(dest, destLen, "%s,%s %d reboot %s", 
							*(((stIdCommandPack *)pCommand)->pszstrHead+iHeadIndex), 
							argv[0],
							((stCellConn*)pCoon)->CellData.iCellId,
							argv[1]);

	return *pLen;	
}

int PackCellTemperatureCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{	
	return (*pLen = snprintf(dest, destLen, "%s,%s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+1),  
								((stIdCommandPack *)pCommand)->pSrcBuf
							 )
			);
}

/*int PackCellAddMinerCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{	
	return (*pLen = snprintf(dest, destLen, "%s,%s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+1), 
								((stIdCommandPack *)pCommand)->pSrcBuf
							 )
			);
}

int PackCellDelMinerCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{	
	return (*pLen = snprintf(dest, destLen, "%s,%s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+1), 
								((stIdCommandPack *)pCommand)->pSrcBuf
							 )
			);
}*/

int PackCellSetLedCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{	
	int argc = 0;
	char *argv[BUFFER_LEN_64] = {0};
	char delim[] = " ";
	char destBuf[BUFFER_LEN_256] = {0};
	int iHeadIndex = 0;

	DEBUGMSG(0,("%s..., iCellId:%d, bMassFlag:%d, pSrcBuf:%s\r\n", 
		__FUNCTION__, ((stCellConn*)pCoon)->CellData.iCellId, CommShellFlag.bMassFlag, ((stIdCommandPack *)pCommand)->pSrcBuf));

	//再次解析出 uSeqId和on/off/flash/normal
	strncpy(destBuf, ((stIdCommandPack *)pCommand)->pSrcBuf, sizeof(destBuf));
	
	if (Parse(destBuf, &argc, argv, delim) != 0)
	{
		return 0;
	}
	
	if (argc != COMMANDLINE_2)
		return 0;
	
	DEBUGMSG(1,("*******pSrcBuf:%s, argc:%d, argv[0]:%s, argv[1]:%s\n", 
		((stIdCommandPack *)pCommand)->pSrcBuf, argc, argv[0], argv[1]));
	
	
	//把用户序列号保存下来，接收时比较判断
	((stCellConn*)pCoon)->uSeqId = strtoul(argv[0], 0, 0);
	DEBUGMSG(1,("*******uSeqId:%d, \n", ((stCellConn*)pCoon)->uSeqId));

	if (CommShellFlag.bMassFlag || CommMonitorFlag.bMassFlag)
	{
		iHeadIndex = 2;
	}
	else
	{
		iHeadIndex = 1;
	}

	*pLen = snprintf(dest, destLen, "%s,%s %d setled %s", 
							*(((stIdCommandPack *)pCommand)->pszstrHead+iHeadIndex), 
							argv[0],
							((stCellConn*)pCoon)->CellData.iCellId,
							argv[1]);

	return *pLen;	
}

int PackCellUpdateSwCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{	
	return (*pLen = snprintf(dest, destLen, "%s,%s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+1), 
								((stIdCommandPack *)pCommand)->pSrcBuf
							 )
			);
}

int PackCellStartTestCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{	
	return (*pLen = snprintf(dest, destLen, "%s,%s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+1), 
								((stIdCommandPack *)pCommand)->pSrcBuf
							 )
			);
}

int PackCellGetTestCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{	
	return (*pLen = snprintf(dest, destLen, "%s,%s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+1), 
								((stIdCommandPack *)pCommand)->pSrcBuf
							 )
			);
}

int PackCellGetTestResultCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{	
	return (*pLen = snprintf(dest, destLen, "%s,%s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+1), 
								((stIdCommandPack *)pCommand)->pSrcBuf
							 )
			);
}

int PackCellReloginCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{	
	int iHeadIndex = 0;
	
	DEBUGMSG(1,("%s..., bMassFlag:%d\r\n", __FUNCTION__, CommShellFlag.bMassFlag));
	/*if (CommShellFlag.bMassFlag || CommMonitorFlag.bMassFlag)
	{
		//把用户序列号保存下来，接收时比较判断
		((stCellConn*)pCoon)->uSeqId = strtoul(((stIdCommandPack *)pCommand)->pSrcBuf, 0, 0);
		*pLen = snprintf(dest, destLen, "%s,%s %d relogin", 
										*(((stIdCommandPack *)pCommand)->pszstrHead+2),
										((stIdCommandPack *)pCommand)->pSrcBuf,
										((stCellConn*)pCoon)->CellData.iCellId
										);
	}
	else
	{
		*pLen = snprintf(dest, destLen, "%s,%s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+1), 
								((stIdCommandPack *)pCommand)->pSrcBuf);
	}*/

	//把用户序列号保存下来，接收时比较判断
	if (CommShellFlag.bMassFlag || CommMonitorFlag.bMassFlag)
	{
		iHeadIndex = 2;
	}
	else
	{
		iHeadIndex = 1;
	}

	//把用户序列号保存下来，接收时比较判断
	((stCellConn*)pCoon)->uSeqId = strtoul(((stIdCommandPack *)pCommand)->pSrcBuf, 0, 0);
	*pLen = snprintf(dest, destLen, "%s,%s %d relogin", 
									*(((stIdCommandPack *)pCommand)->pszstrHead+iHeadIndex),
									((stIdCommandPack *)pCommand)->pSrcBuf,
									((stCellConn*)pCoon)->CellData.iCellId
									);

	return *pLen;
}

int PackCellSetFreqCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{	
	return (*pLen = snprintf(dest, destLen, "%s,%s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+1), 
								((stIdCommandPack *)pCommand)->pSrcBuf
							 )
			);
}

int PackCellResetFlashCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{	
	int iHeadIndex = 0;
	
	DEBUGMSG(0,("%s..., bMassFlag:%d\r\n", __FUNCTION__, CommShellFlag.bMassFlag));
	/*if (CommShellFlag.bMassFlag || CommMonitorFlag.bMassFlag)
	{
		//把用户序列号保存下来，接收时比较判断
		((stCellConn*)pCoon)->uSeqId = strtoul(((stIdCommandPack *)pCommand)->pSrcBuf, 0, 0);
		*pLen = snprintf(dest, destLen, "%s,%s %d resetflash", 
										*(((stIdCommandPack *)pCommand)->pszstrHead+2),
										((stIdCommandPack *)pCommand)->pSrcBuf,
										((stCellConn*)pCoon)->CellData.iCellId
										);
	}
	else
	{
		*pLen = snprintf(dest, destLen, "%s,%s", 
								*(((stIdCommandPack *)pCommand)->pszstrHead+1), 
								((stIdCommandPack *)pCommand)->pSrcBuf);
	}*/

	//把用户序列号保存下来，接收时比较判断
	if (CommShellFlag.bMassFlag || CommMonitorFlag.bMassFlag)
	{
		iHeadIndex = 2;
	}
	else
	{
		iHeadIndex = 1;
	}

	//把用户序列号保存下来，接收时比较判断
	((stCellConn*)pCoon)->uSeqId = strtoul(((stIdCommandPack *)pCommand)->pSrcBuf, 0, 0);
	*pLen = snprintf(dest, destLen, "%s,%s %d resetflash", 
									*(((stIdCommandPack *)pCommand)->pszstrHead+iHeadIndex),
									((stIdCommandPack *)pCommand)->pSrcBuf,
									((stCellConn*)pCoon)->CellData.iCellId
									);

	return *pLen;
}

/*int SendMinerSingle(void *pList, void *pApiNet, void *pCommand, int iSrcIndex, int CommandSock)
{
	int rtn = 0;
	devlist_ist  *pNode;
	char destBuf[BUFFER_LEN_256] = {0};
	int iLen = 0, iListLen = 0, iId = 0;
	
	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));

	pthread_mutex_lock(((structUthash *)pList)->ListLenLock);
	iListLen = ((structUthash *)pList)->CountUsersInt(*((structUthash *)pList)->HashUsers);
	pthread_mutex_unlock(((structUthash *)pList)->ListLenLock);

	if (*((structUthash *)pList)->HashUsers == NULL || iListLen == 0)
		return (rtn = 1);
	DEBUGMSG(1,("1 @@@@@@@@@@@Miner, pDevId:%s\r\n", (((stIdCommandPack *)pCommand)+iSrcIndex)->pDevId));	

	//查找MinerId	
	pthread_mutex_lock(((structUthash *)pList)->CommRemLock);
	//pNode = ((structList *)pList)->ListFind(((structList *)pList)->List, (((stIdCommandPack *)pCommand)+iSrcIndex)->pDevId); 	

	iId = strtoul((((stIdCommandPack *)pCommand)+iSrcIndex)->pDevId, 0, 0);
	pNode = ((structUthash*)pList)->FindUserInt(((structUthash *)pList)->HashUsers, iId);
	if (pNode != NULL)
	{
		//miner,id command
		//DEBUGMSG(0,("1   -----byWbIp:%s\r\n", ((stMinerConn *)pNode->val)->pMinerRes->WbIpPort.byWbIp));
		if ((((stIdCommandPack *)pCommand)+iSrcIndex)->CommandPackFunc
				(
					((stMinerConn *)pNode->val),
					((stIdCommandPack *)pCommand)+iSrcIndex,
					destBuf,
					sizeof(destBuf),
					&iLen
				)		  
			  < 0
		 	)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, iSrcIndex:%d, CommandPackFunc error.\r\n",  __FILE__, __FUNCTION__, __LINE__, iSrcIndex);			
			rtn = 2;
		}
		DEBUGMSG(0,("CommandSock:%d\r\n", CommandSock));
		if (((structApiNet *)pApiNet)->ApiNetSendToClient(
														CommandSock, 
													   ((stMinerConn *)pNode->val)->remote.addr4, 
													   destBuf, 
													   iLen,
													   gpCommandPool) 

			< 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d,  ApiNetSendToClient error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
			rtn = 3;
		}
		DEBUGMSG(1,("\r\nSingle--------------------------------------\r\n"));
		DEBUGMSG(1,("send:%s\r\n", destBuf));
		timeprintf();
	}
	else
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Did not find the Minerlist!\r\n",	__FILE__, __FUNCTION__, __LINE__);		
		rtn = 4;
	}
	pthread_mutex_unlock(((structUthash *)pList)->CommRemLock);
	
	return rtn;
}

int SendMinerAll(void *pList, void *pApiNet, void *pCommand, int iSrcIndex, int CommandSock)
{
	int rtn = -1;//i = 0;
	devlist_ist *pNode,  *tmp = NULL;
	char destBuf[BUFFER_LEN_256] = {0};
	int iLen = 0, iListLen = 0;
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	pthread_mutex_lock(((structUthash *)pList)->ListLenLock);
	iListLen = ((structUthash *)pList)->CountUsersInt(*((structUthash *)pList)->HashUsers);
	pthread_mutex_unlock(((structUthash *)pList)->ListLenLock);

	if (*((structUthash *)pList)->HashUsers == NULL || iListLen == 0)
		return (rtn = 1);

	pthread_mutex_lock(((structUthash *)pList)->CommRemLock);
	//for (i = 0; i < iListLen; i++)
	HASH_ITER(hh, *((structUthash *)pList)->HashUsers, pNode, tmp)
	{
			//pNode = ((structList *)pList)->ListAt(((structList *)pList)->List, i);
			//if (pNode == NULL)
			//	continue;
			//miner,id command
			//DEBUGMSG(0,("1   -----byWbIp:%s\r\n", ((stMinerConn *)pNode->val)->pMinerRes->WbIpPort.byWbIp));
		DEBUGMSG(0,("eMinerComm:%d, iSrcIndex:%d\r\n", ((stMinerConn *)pNode->val)->eMinerComm, iSrcIndex));
		if (((stMinerConn *)pNode->val)->eMinerComm == iSrcIndex)
		{
			if ((((stIdCommandPack *)pCommand)+iSrcIndex)->CommandPackFunc
					(
						((stMinerConn *)pNode->val),
						((stIdCommandPack *)pCommand)+iSrcIndex,
						destBuf,
						sizeof(destBuf),
						&iLen
					)		  
				  < 0
			 	)
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, iSrcIndex:%d, CommandPackFunc error!\r\n",	__FILE__, __FUNCTION__, __LINE__, iSrcIndex);			
				continue;
			}
			DEBUGMSG(0,("CommandSock:%d\r\n", CommandSock));
			if (((structApiNet *)pApiNet)->ApiNetSendToClient(
															CommandSock, 
														   ((stMinerConn *)pNode->val)->remote.addr4, 
														   destBuf, 
														   iLen,
														   gpCommandPool) 

				< 0)
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, ApiNetSendToClient error!\r\n",	__FILE__, __FUNCTION__, __LINE__);			
				continue;
			}
			else
			{
				rtn = 0;
				usleep(1);
			}
			DEBUGMSG(1,("\r\n--------------------------------------\r\n"));
			DEBUGMSG(1,("send:%s\r\n", destBuf));
			//timeprintf();
		}
	}
	pthread_mutex_unlock(((structUthash *)pList)->CommRemLock);
	
	return rtn;
}*/

int PackJsonCellGetStatCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{
	char byIdbufTemp[BUFFER_LEN_32] = {0};
	char *pBuf = NULL;

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	snprintf(byIdbufTemp, sizeof(byIdbufTemp), "%s,cell", *(((stIdCommandPack *)pCommand)->pszstrHead+0));
		
	cJSON* pJsonRoot = cJSON_CreateObject();
    if (pJsonRoot == NULL )
    {
        return -1;
    }

	cJSON_AddStringToObject(pJsonRoot, "id", byIdbufTemp);
	cJSON_AddStringToObject(pJsonRoot, "cellid", (char *)((stCellConn*)pCoon)->CellData.szCellId);
	cJSON_AddStringToObject(pJsonRoot, "method", ((stIdCommandPack *)pCommand)->pstrControl);

	pBuf = cJSON_Print(pJsonRoot);
    if(NULL == pBuf)
    {
        // create object faild, exit
        cJSON_Delete(pJsonRoot);
        return -1;
    }
	*pLen = strlen(pBuf);
	snprintf((char *)dest, destLen, "%s", pBuf);
	DEBUGMSG(1,("destLen:%d\ndest:%s\n", destLen, (char *)dest));

	//释放
	cJSON_Delete(pJsonRoot);
	free(pBuf);


	return 0;
}

int SendCellSingle(void *pList, void *pApiNet, void *pCommand, int iSrcIndex, int CommandSock)
{
	int rtn = 0;
	devlist_st *pNode;
	char destBuf[BUFFER_LEN_2048] = {0};
	int iLen = 0, iListLen = 0;//, iId = 0;
	char szCellIdTrmp[BUFFER_LEN_32] = {0};
	
	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));

	pthread_mutex_lock(((structList *)pList)->ListLenLock);
	iListLen = ((structUthash *)pList)->CountUsersStr(*((structUthash *)pList)->StrHUsers);
	pthread_mutex_unlock(((structList *)pList)->ListLenLock);
	
	if (((structList *)pList)->List == NULL || iListLen == 0)
		return (rtn = 1);

	DEBUGMSG(0,("%d\r\n", iListLen));
	DEBUGMSG(1,("1 @@@@@@@@@@@Cell, pDevId:%s\r\n", (((stIdCommandPack *)pCommand)+iSrcIndex)->pDevId));

	//查找CellId	
	pthread_mutex_lock(((structUthash *)pList)->CommRemLock);
	//iId = strtoul((((stIdCommandPack *)pCommand)+iSrcIndex)->pDevId, 0, 0);
	strncpy(szCellIdTrmp, (((stIdCommandPack *)pCommand)+iSrcIndex)->pDevId, sizeof(szCellIdTrmp));
	//pNode = ((structUthash*)pList)->FindUserInt(((structUthash *)pList)->HashUsers, iId);
	pNode = ((structUthash*)pList)->FindUserStr(((structUthash*)pList)->StrHUsers, szCellIdTrmp);	

	if (pNode != NULL)
	{
		//miner,id command
		if ((((stIdCommandPack *)pCommand)+iSrcIndex)->CommandPackFunc
				(
					((stCellConn *)pNode->val),
					((stIdCommandPack *)pCommand)+iSrcIndex,
					destBuf,
					sizeof(destBuf),
					&iLen
				)		  
			  < 0
			)

		{
			rtn = 2;
		}
		
		if (((structApiNet *)pApiNet)->ApiNetSendToClient(
														CommandSock, 
													   ((stCellConn *)pNode->val)->remote.addr4, 
													   destBuf, 
													   iLen,
													   gpCommandPool) 

			< 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, ApiNetSendToClient error!\r\n",	__FILE__, __FUNCTION__, __LINE__);			
			rtn = 3;
		}
		DEBUGMSG(1,("\r\nSingle++++++++++++++++++++++++++++++++++++++\r\n"));
		DEBUGMSG(1,("send:%s\r\n", destBuf));
		timeprintf();
	}
	else
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Did not find the Celllist!\r\n", __FILE__, __FUNCTION__, __LINE__);		
		rtn = 4;
	}
	pthread_mutex_unlock(((structUthash *)pList)->CommRemLock);
	
	return rtn;
}

int SendCellAll(void *pList, void *pApiNet, void *pCommand, int iSrcIndex, int CommandSock)
{
	int rtn = -1;//i = 0;
	devlist_st *pNode,  *tmp = NULL;
	char destBuf[BUFFER_LEN_2048] = {0};
	int iLen = 0, iListLen = 0;
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	pthread_mutex_lock(((structList *)pList)->ListLenLock);
	iListLen = ((structUthash *)pList)->CountUsersStr(*((structUthash *)pList)->StrHUsers);
	pthread_mutex_unlock(((structList *)pList)->ListLenLock);
	
	if (((structList *)pList)->List == NULL || iListLen == 0)
		return (rtn = 1);

	DEBUGMSG(1,("%d\r\n", iListLen));

	pthread_mutex_lock(((structUthash *)pList)->CommRemLock);
	pthread_mutex_lock(CommMonitorFlag.OnlineCountLock);
	//for (i = 0; i < iListLen; i++)
	HASH_ITER(hh, *((structUthash *)pList)->StrHUsers, pNode, tmp)	
	{
		//pNode = ((structList *)pList)->ListAt(((structList *)pList)->List, i);
		//if (pNode == NULL)
		//	continue;	
		//miner,id command
		DEBUGMSG(0,("iCellId:%d, eCellComm:%d, iSrcIndex:%d, bCellOnline:%d\r\n", ((stCellConn *)pNode->val)->CellData.iCellId, 
				((stCellConn *)pNode->val)->eCellComm, iSrcIndex, ((stCellConn *)pNode->val)->CellData.bCellOnline));
		//对于离线的设备不再发送任何命令
		if (((stCellConn *)pNode->val)->eCellComm == iSrcIndex
			&& ((stCellConn *)pNode->val)->CellData.bCellOnline == TRUE)
		{
			//每发出去一个cell统计一下设备数量
			if (((stCellConn *)pNode->val)->eCellComm != E_COMMAND_TYPE_STAT)
				CommMonitorFlag.iOnlineCount++;
			if ((((stIdCommandPack *)pCommand)+iSrcIndex)->CommandPackFunc
					(
						((stCellConn *)pNode->val),
						((stIdCommandPack *)pCommand)+iSrcIndex,
						destBuf,
						sizeof(destBuf),
						&iLen
					)		  
				  < 0
				)

			{
				continue;
			}
			
			if (((structApiNet *)pApiNet)->ApiNetSendToClient(
															CommandSock, 
														   ((stCellConn *)pNode->val)->remote.addr4, 
														   destBuf, 
														   iLen,
														   gpCommandPool) 

				< 0)
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, ApiNetSendToClient error!\r\n", __FILE__, __FUNCTION__, __LINE__); 				
				continue;
			}
			else
			{
				if (((stCellConn *)pNode->val)->eCellComm != E_COMMAND_TYPE_STAT)
					CommMonitorFlag.iSendLen++;
				rtn = 0;
				usleep(1);
			}
			DEBUGMSG(1,("\r\n++++++++++++++++++++++++++++++++++++++\r\n"));
			DEBUGMSG(1,("CommMonitorFlag.iSendLen:%d\n", CommMonitorFlag.iSendLen));
			DEBUGMSG(1,("send:%s\r\n", destBuf));	
		}
		//timeprintf();
	}
	pthread_mutex_unlock(CommMonitorFlag.OnlineCountLock);
	pthread_mutex_unlock(((structUthash *)pList)->CommRemLock);
#ifdef _DEBUG_MANY_DEV_
	{
		printf("1111 allCommMonitorFlag.iSendLen:%d\n", CommMonitorFlag.iSendLen);
		printf("CommMonitorFlag.iOnlineCount:%d\n", CommMonitorFlag.iOnlineCount);
	}
#endif			
	return rtn;
}

int JsonPackNoParamCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{
	char byIdbufTemp[BUFFER_LEN_32] = {0};
	char *pBuf = NULL;
	int iHeadIndex = 0;

	DEBUGMSG(1,("%s..., bMassFlag:%d\r\n", __FUNCTION__, CommShellFlag.bMassFlag));
	
	//把用户序列号保存下来，接收时比较判断
	if (CommShellFlag.bMassFlag || CommMonitorFlag.bMassFlag)
	{
		iHeadIndex = 2;
	}
	else
	{
		iHeadIndex = 1;
	}

	//把用户序列号保存下来，接收时比较判断
	((stCellConn*)pCoon)->uSeqId = strtoul(((stIdCommandPack *)pCommand)->pSrcBuf, 0, 0);
	
	snprintf(byIdbufTemp, sizeof(byIdbufTemp), "%s,%d", 
		*(((stIdCommandPack *)pCommand)->pszstrHead+iHeadIndex), ((stCellConn*)pCoon)->uSeqId);
		
	cJSON* pJsonRoot = cJSON_CreateObject();
    if (pJsonRoot == NULL )
    {
        return -1;
    }

	cJSON_AddStringToObject(pJsonRoot, "id", byIdbufTemp);
	cJSON_AddStringToObject(pJsonRoot, "cellid", (char *)((stCellConn*)pCoon)->CellData.szCellId);
	cJSON_AddStringToObject(pJsonRoot, "method", ((stIdCommandPack *)pCommand)->pstrControl);

	pBuf = cJSON_Print(pJsonRoot);
    if(NULL == pBuf)
    {
        // create object faild, exit
        cJSON_Delete(pJsonRoot);
        return -1;
    }
	*pLen = strlen(pBuf);
	snprintf((char *)dest, destLen, "%s", pBuf);
	DEBUGMSG(1,("destLen:%d\ndest:%s\n", destLen, (char *)dest));

	//释放
	cJSON_Delete(pJsonRoot);
	free(pBuf);

	return 0;
}

int JsonPackRebootCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{
	int argc = 0;
	char *argv[BUFFER_LEN_64] = {0};
	char delim[] = " ";
	char destBuf[BUFFER_LEN_256] = {0};
	char byIdbufTemp[BUFFER_LEN_32] = {0};
	char *pBuf = NULL;
	int iHeadIndex = 0;
	int iParam = 0;
	int iAlgLen = 0, iParamLen = 0;
	int i = 0;

	DEBUGMSG(0,("%s..., iCellId:%d, bMassFlag:%d, pSrcBuf:%s\r\n", 
		__FUNCTION__, ((stCellConn*)pCoon)->CellData.iCellId, CommShellFlag.bMassFlag, ((stIdCommandPack *)pCommand)->pSrcBuf));

	//再次解析出 uSeqId和all/mcu/fpga/bd1/bd2/bd3
	strncpy(destBuf, ((stIdCommandPack *)pCommand)->pSrcBuf, sizeof(destBuf));
	
	if (Parse(destBuf, &argc, argv, delim) != 0)
	{
		return 0;
	}

	if (argc != COMMANDLINE_2)
		return 0;

	DEBUGMSG(1,("*******pSrcBuf:%s, argc:%d, argv[0]:%s, argv[1]:%s\n", 
		((stIdCommandPack *)pCommand)->pSrcBuf, argc, argv[0], argv[1]));

	//把用户序列号保存下来，接收时比较判断
	((stCellConn*)pCoon)->uSeqId = strtoul(argv[0], 0, 0);
	DEBUGMSG(1,("*******uSeqId:%d, \n", ((stCellConn*)pCoon)->uSeqId));

	if (CommShellFlag.bMassFlag || CommMonitorFlag.bMassFlag)
	{
		iHeadIndex = 2;
	}
	else
	{
		iHeadIndex = 1;
	}

	snprintf(byIdbufTemp, sizeof(byIdbufTemp), "%s,%d", 
		*(((stIdCommandPack *)pCommand)->pszstrHead+iHeadIndex), ((stCellConn*)pCoon)->uSeqId);
		
	cJSON* pJsonRoot = cJSON_CreateObject();
    if (pJsonRoot == NULL )
    {
        return -1;
    }


	iParamLen = strlen(argv[COMMANDLINE_1]);
	for (i = 0; i < dim(gReboot); i++)
	{
		iAlgLen = strlen((char *)gReboot[i].byReboot);
		if (strncasecmp((char *)gReboot[i].byReboot, argv[COMMANDLINE_1], iAlgLen) == 0 && iAlgLen == iParamLen)
		{
			iParam = i;
			break;
		}
	}

	cJSON_AddStringToObject(pJsonRoot, "id", byIdbufTemp);
	cJSON_AddStringToObject(pJsonRoot, "cellid", (char *)((stCellConn*)pCoon)->CellData.szCellId);
	cJSON_AddStringToObject(pJsonRoot, "method", ((stIdCommandPack *)pCommand)->pstrControl);
	cJSON_AddNumberToObject(pJsonRoot, "param", iParam);

	pBuf = cJSON_Print(pJsonRoot);
    if(NULL == pBuf)
    {
        // create object faild, exit
        cJSON_Delete(pJsonRoot);
        return -1;
    }
	*pLen = strlen(pBuf);
	snprintf((char *)dest, destLen, "%s", pBuf);
	DEBUGMSG(1,("destLen:%d\ndest:%s\n", destLen, (char *)dest));

	//释放
	cJSON_Delete(pJsonRoot);
	free(pBuf);

	return 0;
}

int JsonPackSetLedCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{
	int argc = 0;
	char *argv[BUFFER_LEN_64] = {0};
	char delim[] = " ";
	char destBuf[BUFFER_LEN_256] = {0};
	char byIdbufTemp[BUFFER_LEN_32] = {0};
	char *pBuf = NULL;
	int iHeadIndex = 0;
	int iParam = 0;
	int iAlgLen = 0, iParamLen = 0;
	int i = 0;

	DEBUGMSG(0,("%s..., iCellId:%d, bMassFlag:%d, pSrcBuf:%s\r\n", 
		__FUNCTION__, ((stCellConn*)pCoon)->CellData.iCellId, CommShellFlag.bMassFlag, ((stIdCommandPack *)pCommand)->pSrcBuf));

	//再次解析出 uSeqId和all/mcu/fpga/bd1/bd2/bd3
	strncpy(destBuf, ((stIdCommandPack *)pCommand)->pSrcBuf, sizeof(destBuf));
	
	if (Parse(destBuf, &argc, argv, delim) != 0)
	{
		return 0;
	}

	if (argc != COMMANDLINE_2)
		return 0;

	DEBUGMSG(1,("*******pSrcBuf:%s, argc:%d, argv[0]:%s, argv[1]:%s\n", 
		((stIdCommandPack *)pCommand)->pSrcBuf, argc, argv[0], argv[1]));

	//把用户序列号保存下来，接收时比较判断
	((stCellConn*)pCoon)->uSeqId = strtoul(argv[0], 0, 0);
	DEBUGMSG(1,("*******uSeqId:%d, \n", ((stCellConn*)pCoon)->uSeqId));

	if (CommShellFlag.bMassFlag || CommMonitorFlag.bMassFlag)
	{
		iHeadIndex = 2;
	}
	else
	{
		iHeadIndex = 1;
	}

	snprintf(byIdbufTemp, sizeof(byIdbufTemp), "%s,%d", 
		*(((stIdCommandPack *)pCommand)->pszstrHead+iHeadIndex), ((stCellConn*)pCoon)->uSeqId);
		
	cJSON* pJsonRoot = cJSON_CreateObject();
    if (pJsonRoot == NULL )
    {
        return -1;
    }


	iParamLen = strlen(argv[COMMANDLINE_1]);
	for (i = 0; i < dim(gSetLed); i++)
	{
		iAlgLen = strlen((char *)gSetLed[i].bySetLed);
		if (strncasecmp((char *)gSetLed[i].bySetLed, argv[COMMANDLINE_1], iAlgLen) == 0 && iAlgLen == iParamLen)
		{
			iParam = i;
			break;
		}
	}

	cJSON_AddStringToObject(pJsonRoot, "id", byIdbufTemp);
	cJSON_AddStringToObject(pJsonRoot, "cellid", (char *)((stCellConn*)pCoon)->CellData.szCellId);
	cJSON_AddStringToObject(pJsonRoot, "method", ((stIdCommandPack *)pCommand)->pstrControl);
	cJSON_AddNumberToObject(pJsonRoot, "param", iParam);

	pBuf = cJSON_Print(pJsonRoot);
    if(NULL == pBuf)
    {
        // create object faild, exit
        cJSON_Delete(pJsonRoot);
        return -1;
    }
	*pLen = strlen(pBuf);
	snprintf((char *)dest, destLen, "%s", pBuf);
	DEBUGMSG(1,("destLen:%d\ndest:%s\n", destLen, (char *)dest));

	//释放
	cJSON_Delete(pJsonRoot);
	free(pBuf);

	return 0;
}

int JsonPackSetFreqCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{
	int argc = 0;
	char *argv[BUFFER_LEN_64] = {0};
	char delim[] = " ";
	char destBuf[BUFFER_LEN_256] = {0};
	char byIdbufTemp[BUFFER_LEN_32] = {0};
	char *pBuf = NULL;
	int iHeadIndex = 0;
	cJSON * pJsonSub = NULL;
	char bufDest[BUFFER_LEN_1024] = {0};

	DEBUGMSG(1,("%s..., iCellId:%d, bMassFlag:%d, pSrcBuf:%s\r\n", 
		__FUNCTION__, ((stCellConn*)pCoon)->CellData.iCellId, CommShellFlag.bMassFlag, ((stIdCommandPack *)pCommand)->pSrcBuf));

	//再次解析出 uSeqId <btc_freq> <ltc_freq>
	strncpy(destBuf, ((stIdCommandPack *)pCommand)->pSrcBuf, sizeof(destBuf));
	
	if (Parse(destBuf, &argc, argv, delim) != 0)
	{
		return 0;
	}

	if (argc != COMMANDLINE_3)
		return 0;

	DEBUGMSG(1,("*******pSrcBuf:%s, argc:%d, argv[0]:%s, argv[1]:%s, argv[2]:%s\n", 
		((stIdCommandPack *)pCommand)->pSrcBuf, argc, argv[0], argv[1], argv[2]));

	//把用户序列号保存下来，接收时比较判断
	((stCellConn*)pCoon)->uSeqId = strtoul(argv[0], 0, 0);
	DEBUGMSG(1,("*******uSeqId:%d, \n", ((stCellConn*)pCoon)->uSeqId));

	if (CommShellFlag.bMassFlag || CommMonitorFlag.bMassFlag)
	{
		iHeadIndex = 2;
	}
	else
	{
		iHeadIndex = 1;
	}

	snprintf(byIdbufTemp, sizeof(byIdbufTemp), "%s,%d", 
		*(((stIdCommandPack *)pCommand)->pszstrHead+iHeadIndex), ((stCellConn*)pCoon)->uSeqId);
		
	cJSON* pJsonRoot = cJSON_CreateObject();
    if (pJsonRoot == NULL )
    {
        return -1;
    }

	//改变结点中的BTC频率、LTC频率
	((stCellConn*)pCoon)->CellData.iChipBTCFreq = strtoul(argv[COMMANDLINE_1], 0, 0);
	((stCellConn*)pCoon)->CellData.iChipLTCFreq = strtoul(argv[COMMANDLINE_2], 0, 0);

	//更新数据库中的频率
	if (bIniCellBase)
	{
		CellSqlQuery[E_QUERY_TYPE_UPDATE - 1].QueryPackFunc4(
														pCoon, 
														&gCfgDataDL.CellBaseInfo, 
														&gCfgDataDL.CellColName, 
														bufDest, sizeof(bufDest));
		DEBUGMSG(0,("bufDest:%s\r\n", bufDest));
		pthread_mutex_lock(&gCellBaseLock);
		MysqlFunc.dbDoQuery(&myCellBase, bufDest, E_QUERY_TYPE_UPDATE, NULL);
		pthread_mutex_unlock(&gCellBaseLock);
		//memset(bufDest, 0, sizeof(bufDest));	
	}	



	cJSON_AddStringToObject(pJsonRoot, "id", byIdbufTemp);
	cJSON_AddStringToObject(pJsonRoot, "cellid", (char *)((stCellConn*)pCoon)->CellData.szCellId);
	cJSON_AddStringToObject(pJsonRoot, "method", ((stIdCommandPack *)pCommand)->pstrControl);
	//cJSON_AddStringToObject(pJsonRoot, "param", argv[1]);

	pJsonSub = cJSON_CreateObject();
    if (pJsonSub == NULL )
    {
        return -1;
    }
	cJSON_AddNumberToObject(pJsonSub, "btc_freq", ((stCellConn*)pCoon)->CellData.iChipBTCFreq);
	cJSON_AddNumberToObject(pJsonSub, "ltc_freq", ((stCellConn*)pCoon)->CellData.iChipLTCFreq );
	cJSON_AddItemToObject(pJsonRoot, "param", pJsonSub);

	pBuf = cJSON_Print(pJsonRoot);
    if(NULL == pBuf)
    {
        // create object faild, exit
        cJSON_Delete(pJsonRoot);
        return -1;
    }
	*pLen = strlen(pBuf);
	snprintf((char *)dest, destLen, "%s", pBuf);
	DEBUGMSG(1,("destLen:%d\ndest:%s\n", destLen, (char *)dest));

	//释放
	cJSON_Delete(pJsonRoot);
	free(pBuf);

	return 0;
}

//打成1包
int JsonPackSetPool(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen)
{
	char byIdbufTemp[BUFFER_LEN_32] = {0};
	char *pBuf = NULL;
	int iHeadIndex = 0;
	int rtn = 0;
	//structUthash *pList = NULL;
	//int iTotListLen = 0;
	//devlist_ist *pNode = NULL;//*tmp = NULL;
	int i = 0;
	cJSON *pArray = NULL;
	cJSON *pArrToJson = NULL;
	//cJSON *pResultJson = NULL;
	//stCellConn *pCellConn = NULL;

	DEBUGMSG(1,("%s..., bMassFlag:%d\r\n", __FUNCTION__, CommShellFlag.bMassFlag));
	
	//把用户序列号保存下来，接收时比较判断
	if (CommShellFlag.bMassFlag || CommMonitorFlag.bMassFlag)
	{
		iHeadIndex = 2;
	}
	else
	{
		iHeadIndex = 1;
	}

	//把用户序列号保存下来，接收时比较判断
	((stCellConn*)pCoon)->uSeqId = strtoul(((stIdCommandPack *)pCommand)->pSrcBuf, 0, 0);
	
	snprintf(byIdbufTemp, sizeof(byIdbufTemp), "%s,%d", 
		*(((stIdCommandPack *)pCommand)->pszstrHead+iHeadIndex), ((stCellConn*)pCoon)->uSeqId);
		
	cJSON* pJsonRoot = cJSON_CreateObject();
    if (pJsonRoot == NULL )
    {
        return -1;
    }

	//把用户序列号保存下来，接收时比较判断
	((stCellConn*)pCoon)->uSeqId = strtoul(((stIdCommandPack *)pCommand)->pSrcBuf, 0, 0);

	cJSON_AddStringToObject(pJsonRoot, "id", byIdbufTemp);
	cJSON_AddStringToObject(pJsonRoot, "cellid", (char *)((stCellConn*)pCoon)->CellData.szCellId);
	cJSON_AddStringToObject(pJsonRoot, "method", ((stIdCommandPack *)pCommand)->pstrControl);

	pArray = cJSON_CreateArray();
    if(NULL == pArray)
    {
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pArray!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);        
        return -1;
    }
	
	//初始化为默认池
	pthread_mutex_lock(&((stCellConn*)pCoon)->CellDataLock.PoolConfLock);
	((stCellConn*)pCoon)->CellPoolConf.iBTCPoolCount = 0;
	for(i = 0;i < gPoolSpecifyConf.iBTCPoolCount && gPoolSpecifyConf.iBTCPoolCount <= MAX_POOL_COUNT;i++)	
	{	
		((stCellConn*)pCoon)->CellPoolConf.stBTCPoolConf[i].iPoolId = gPoolSpecifyConf.stBTCPoolConf[i].iPoolId;
		DEBUGMSG(0,("BTC iPoolId[%d]:%d\r\n", i, ((stCellConn*)pCoon)->CellPoolConf.stBTCPoolConf[i].iPoolId));
		strcpy((char *)((stCellConn*)pCoon)->CellPoolConf.stBTCPoolConf[i].szUrl, (char *)gPoolSpecifyConf.stBTCPoolConf[i].szUrl);
		strcpy((char *)((stCellConn*)pCoon)->CellPoolConf.stBTCPoolConf[i].szUser, (char *)gPoolSpecifyConf.stBTCPoolConf[i].szUser);
		strcpy((char *)((stCellConn*)pCoon)->CellPoolConf.stBTCPoolConf[i].szPasswd, (char *)gPoolSpecifyConf.stBTCPoolConf[i].szPasswd);
		((stCellConn*)pCoon)->CellPoolConf.stBTCPoolConf[i].eAlgo = gPoolSpecifyConf.stBTCPoolConf[i].eAlgo;

		pArrToJson = cJSON_CreateObject();
		if (pArrToJson == NULL)
			continue;
		cJSON_AddNumberToObject(pArrToJson, "algo", gPoolSpecifyConf.stBTCPoolConf[i].eAlgo + 1);
		cJSON_AddStringToObject(pArrToJson, "url", (char *)gPoolSpecifyConf.stBTCPoolConf[i].szUrl);
		cJSON_AddStringToObject(pArrToJson, "worker", (char *)gPoolSpecifyConf.stBTCPoolConf[i].szUser);
		cJSON_AddStringToObject(pArrToJson, "pass", (char *)gPoolSpecifyConf.stBTCPoolConf[i].szPasswd);
		//cJSON_AddItemToObject(pArray, "", pArrToJson);
		cJSON_AddItemToArray(pArray, pArrToJson);

		((stCellConn*)pCoon)->CellPoolConf.iBTCPoolCount++;
	}
	
	((stCellConn*)pCoon)->CellPoolConf.iLTCPoolCount = 0;
	for(i = 0;i < gPoolSpecifyConf.iLTCPoolCount && gPoolSpecifyConf.iLTCPoolCount <= MAX_POOL_COUNT;i++)	
	{	
		((stCellConn*)pCoon)->CellPoolConf.stLTCPoolConf[i].iPoolId = gPoolSpecifyConf.stLTCPoolConf[i].iPoolId;
		DEBUGMSG(0,("LTC iPoolId[%d]:%d\r\n", i, ((stCellConn*)pCoon)->CellPoolConf.stLTCPoolConf[i].iPoolId));
		strcpy((char *)((stCellConn*)pCoon)->CellPoolConf.stLTCPoolConf[i].szUrl, (char *)gPoolSpecifyConf.stLTCPoolConf[i].szUrl);
		strcpy((char *)((stCellConn*)pCoon)->CellPoolConf.stLTCPoolConf[i].szUser, (char *)gPoolSpecifyConf.stLTCPoolConf[i].szUser);
		strcpy((char *)((stCellConn*)pCoon)->CellPoolConf.stLTCPoolConf[i].szPasswd, (char *)gPoolSpecifyConf.stLTCPoolConf[i].szPasswd);
		((stCellConn*)pCoon)->CellPoolConf.stLTCPoolConf[i].eAlgo = gPoolSpecifyConf.stLTCPoolConf[i].eAlgo;

		pArrToJson = cJSON_CreateObject();
		if (pArrToJson == NULL)
			continue;
		cJSON_AddNumberToObject(pArrToJson, "algo", gPoolSpecifyConf.stLTCPoolConf[i].eAlgo + 1);
		cJSON_AddStringToObject(pArrToJson, "url", (char *)gPoolSpecifyConf.stLTCPoolConf[i].szUrl);
		cJSON_AddStringToObject(pArrToJson, "worker", (char *)gPoolSpecifyConf.stLTCPoolConf[i].szUser);
		cJSON_AddStringToObject(pArrToJson, "pass", (char *)gPoolSpecifyConf.stLTCPoolConf[i].szPasswd);
		//cJSON_AddItemToObject(pArray, "", pArrToJson);
		cJSON_AddItemToArray(pArray, pArrToJson);

		((stCellConn*)pCoon)->CellPoolConf.iLTCPoolCount++;
	}
	pthread_mutex_unlock(&((stCellConn*)pCoon)->CellDataLock.PoolConfLock);

	/*pResultJson = cJSON_CreateObject();
    if(NULL == pResultJson)
    {
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"param\" is pResultJson!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);        
        return -1;
    }*/
	
	//cJSON_AddNumberToObject(pResultJson, "btc_freq", pCellConn->CellData.iChipBTCFreq);
	//cJSON_AddNumberToObject(pResultJson, "ltc_freq", pCellConn->CellData.iChipLTCFreq);
	//cJSON_AddItemToObject(pResultJson, "pools", pArray);
	
	cJSON_AddItemToObject(pJsonRoot, "param", pArray);
	pBuf = cJSON_Print(pJsonRoot);
    if(NULL == pBuf)
    {
        // create object faild, exit
        cJSON_Delete(pJsonRoot);
        return -1;
    }
	//destLen = strlen(pBuf);
	//strncpy(dest, pBuf, destLen);
	*pLen = strlen(pBuf);
	snprintf((char *)dest, destLen, "%s", pBuf);
	DEBUGMSG(0,("destLen:%d\ndest:%s\n", destLen, (char *)dest));

	//释放
	cJSON_Delete(pJsonRoot);
	free(pBuf);

    return rtn;
}

int JsonParseStat(void * pJson, void *dest, int destLen, int *pId)
{
	int rtn = 0;
	cJSON *pJsonParam = NULL;
	cJSON *pSub = NULL;
	int iPos = 0, i = 0;

	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	pJsonParam = cJSON_GetObjectItem((cJSON *)pJson, "result");
	
	if(NULL == pJsonParam)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"result\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		//return E_COMMAND_ERROR_JSON;	
		return -1;
	}
	//stCellConn *val = malloc(sizeof(stCellConn));
	//memset(val, 0, sizeof(stCellConn));

	DEBUGMSG(0,("destLen:%d, pId:%s\n", destLen, (char *)pId));
	snprintf((char *)dest+iPos, destLen, "%s result", (char *)pId);
	iPos += strlen((char *)dest+iPos);
	DEBUGMSG(0,("dest:%s\n", (char *)dest));

	for (i = 0; i < dim(szCellStatParam); i++)
	{
		pSub = cJSON_GetObjectItem(pJsonParam, szCellStatParam[i]);
		if(NULL == pSub)
		{
			// get "result" from json faild
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, \"%s\" is error!\r\n",  
				__FILE__, __FUNCTION__, __LINE__, szCellStatParam[i]);
			//free(val);
			return E_LOGIN_ERROR_JSON;;
		}
		else
		{
			//iParamLen = srrlen(szCellLoginParam[i]);
			//iValueLen = strlen(pSub->valuestring);
			//if (iParamLen == iValueLen && strncmp(pSub->valuestring, szCellLoginParam[i]) == 0)
			if (i < 4)
			{
				snprintf((char *)dest+iPos, destLen, " %d", pSub->valueint);
				DEBUGMSG(0,("%d, dest:%s, val:%d\n", i, (char *)dest+iPos, pSub->valueint));
			}
			else if (i == 10)
			{
																										
				DEBUGMSG(1,("szCellStatParam[%d] : %s, string:%s\n", i, szCellStatParam[i] , pSub->string));
				
				int iSize = cJSON_GetArraySize(pSub);
				int iCnt = 0;
				DEBUGMSG(1,("iSize:%d\n", iSize));
				for(iCnt = 0; iCnt < iSize; iCnt++)
				{
					cJSON * pSubA = cJSON_GetArrayItem(pSub, iCnt);
					if(NULL == pSubA)
					{
						continue;
					}
					//int iValue = pSubA->valueint;
					//printf("value[%2d] : [%d]\n", iCnt, iValue);
					if (iCnt == 0)
						snprintf((char *)dest+iPos, destLen, " %d", pSubA->valueint);
					else
						snprintf((char *)dest+iPos, destLen, "|%d", pSubA->valueint);
					iPos += strlen((char *)dest+iPos);
				}
			}
			else
			{
				//snprintf((char *)dest+iPos, destLen, " %s", pSub->valuestring);
				//DEBUGMSG(0,("%d, dest:%s, val:%s\n", i, (char *)dest+iPos, pSub->valuestring));				
				snprintf((char *)dest+iPos, destLen, " %llu", (unsigned long long)pSub->valuedouble);
				DEBUGMSG(0,("%d, dest:%s, valuedouble:%llu\n", i, (char *)dest+iPos, (unsigned long long)pSub->valuedouble));
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

int JsonParseResult(void * pJson, void *dest, int destLen, int *pId)
{
	int rtn = 0;
	cJSON *pJsonParam = NULL;
	//cJSON *pSub = NULL;
	int iPos = 0;//, i = 0;

	DEBUGMSG(1,("%s...\n", __FUNCTION__));

	pJsonParam = cJSON_GetObjectItem((cJSON *)pJson, "result");
	
	if(NULL == pJsonParam)
	{
		// get "error" from json faild
 		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"result\" is error!\r\n",  
			__FILE__, __FUNCTION__, __LINE__);
		//return E_COMMAND_ERROR_JSON;	
		return -1;
	}
	else
	{
		DEBUGMSG(1,("1 -------------\n"));
		DEBUGMSG(1,("type :%d, result : %d\n", pJsonParam->type, pJsonParam->valueint));
		if (FALSE == pJsonParam->valueint)
		{
			rtn = 1;
		}
	}

	DEBUGMSG(1,("destLen:%d, pId:%s\n", destLen, (char *)pId));
	snprintf((char *)dest+iPos, destLen, "%s result", (char *)pId);
	iPos += strlen((char *)dest+iPos);
	DEBUGMSG(1,("dest:%s\n", (char *)dest));

	/*for (i = 0; i < dim(szCellStatParam); i++)
	{
		pSub = cJSON_GetObjectItem(pJsonParam, szCellStatParam[i]);
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
			if (i < 3)
			{
				snprintf((char *)dest+iPos, destLen, " %d", pSub->valueint);
				DEBUGMSG(0,("%d, dest:%s, val:%d\n", i, (char *)dest+iPos, pSub->valueint));
			}
			else if (i == 10)
			{
																										
				DEBUGMSG(1,("szCellStatParam[%d] : %s, string:%s\n", i, szCellStatParam[i] , pSub->string));
				
				int iSize = cJSON_GetArraySize(pSub);
				int iCnt = 0;
				DEBUGMSG(1,("iSize:%d\n", iSize));
				for(iCnt = 0; iCnt < iSize; iCnt++)
				{
					cJSON * pSubA = cJSON_GetArrayItem(pSub, iCnt);
					if(NULL == pSubA)
					{
						continue;
					}
					//int iValue = pSubA->valueint;
					//printf("value[%2d] : [%d]\n", iCnt, iValue);
					if (iCnt == 0)
						snprintf((char *)dest+iPos, destLen, " %d", pSubA->valueint);
					else
						snprintf((char *)dest+iPos, destLen, "|%d", pSubA->valueint);
					iPos += strlen((char *)dest+iPos);
				}
			}
			else
			{
				snprintf((char *)dest+iPos, destLen, " %s", pSub->valuestring);
				DEBUGMSG(0,("%d, dest:%s, val:%s\n", i, (char *)dest+iPos, pSub->valuestring));
			}
			iPos += strlen((char *)dest+iPos);
		}
	}*/

	return rtn;
}

	
/****************************************************************
** Function name:		ParseJsonCommand
** Descriptions:	

** input parameters:		无
** output parameters:		无
** Returned value:			1:id错误
							2:switchid错误


****************************************************************/
int ParseJsonCommand
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
	char szCellIdTemp[BUFFER_LEN_64] = {0};
	//char dest[BUFFER_LEN_4096] = {0};
	//int destLen = sizeof(dest);
	int iPos = 0;

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

    pSub = cJSON_GetObjectItem(pJson, "id");
    if(NULL == pSub)
    {
    	// get "id" from json faild
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, \"id\" is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return -1;
    }
	else
	{
		/*if (pSub->valueint != *((stLoginSrc*)pSrc)->pIdSeq)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, command is error, id:%d != %d!\r\n",  
				__FILE__, __FUNCTION__, __LINE__, pSub->valueint, *((stLoginSrc*)pSrc)->pIdSeq);
			return 1;
		}*/
		snprintf(szCellIdTemp, sizeof(szCellIdTemp), "%s", pSub->valuestring);
		iPos += strlen(szCellIdTemp + iPos); 
	}

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
		//strncpy(szCellIdTemp, pSub->valuestring, sizeof(szCellIdTemp));
		snprintf(szCellIdTemp + iPos, sizeof(szCellIdTemp), " %s", pSub->valuestring);
		iPos += strlen(szCellIdTemp + iPos); 	
	}

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

		for (i = 0; i < ((stCommandSrc*)pSrc)->iCommLen; i++)
		{
			iMethodLen = strlen((char *)((stCommandSrc*)pSrc)->pCommandComm[i].cmd);
			if (iMethodLen == iParamLen && 
				strncasecmp((char *)((stCommandSrc*)pSrc)->pCommandComm[i].cmd, pSub->valuestring, iMethodLen) == 0)
			{	
				*pIndex = i;
				break;
			}
		}
		DEBUGMSG(1,("valuestring:%s\n", pSub->valuestring));
		//没有对应的命令
		DEBUGMSG(1,("i:%d, iCommLen:%d\n", i, ((stCommandSrc*)pSrc)->iCommLen));
		if (i == ((stCommandSrc*)pSrc)->iCommLen)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, \"method\" param is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);   
			return E_COMMAND_ERROR_JSON;
		}
		//return 0;
	}

	DEBUGMSG(1,("*pIndex:%d < iCommLen:%d\n", *pIndex, ((stCommandSrc*)pSrc)->iCommLen));
	//ParseJsonCellLogin
	rtn = ((stCommandSrc*)pSrc)->pCommandComm[*pIndex].UserCmdFunc(pJson, dest, destLen, (void *)szCellIdTemp);
	
	DEBUGMSG(1,("rtn:%d\n", rtn));
	DEBUGMSG(0,("1 *pIndex:%d, dest:%s\n", *pIndex, (char *)dest));



    cJSON_Delete(pJson);

	return rtn;
}


int NoParseShellCommand(int argc, char **argv, int iStartPos)
{
	int rtn = E_COMMAND_ERROR_TYPE;

	return rtn;
}

/*int ParseShellMinerSetAlgoCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;
	int i = 0, index = 0, iCountI = 0;
	int iParamLen = 0, iAlgLen = 0;

	if (argc < iStartPos || argc != COMMANDLINE_5)
		return (rtn = E_COMMAND_ERROR_PARAM_NUM);

	index = iStartPos;
	iParamLen = strlen(argv[index]);
	for (i = 0; i < dim(Algo); i++)
	{
		iAlgLen = strlen((char *)Algo[i].algo);
		if (strncasecmp((char *)Algo[i].algo, argv[index], iAlgLen) == 0 && iAlgLen == iParamLen)
		{
			break;
		}
		else
		{
			if (++iCountI == dim(Algo))
				rtn = E_COMMAND_ERROR_ALGO;
		}
	}
	
	return rtn;
}

int ParseShellMinerReloginCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;

	if (argc < iStartPos || argc != COMMANDLINE_4)
		rtn = E_COMMAND_ERROR_PARAM_NUM;

	return rtn;
}

int ParseShellMinerGetdetailCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;

	if (argc < iStartPos || argc != COMMANDLINE_4)
		rtn = E_COMMAND_ERROR_PARAM_NUM;

	return rtn;
}
*/

int ParseShellCellRebootCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;
	int i = 0, index = 0, iCountI = 0;
	int iParamLen = 0, iAlgLen = 0;

	if (argc < iStartPos || argc != COMMANDLINE_5)
		return (rtn = E_COMMAND_ERROR_PARAM_NUM);

	index = iStartPos;
	iParamLen = strlen(argv[index]);
	for (i = 0; i < dim(gReboot); i++)
	{
		iAlgLen = strlen((char *)gReboot[i].byReboot);
		if (strncasecmp((char *)gReboot[i].byReboot, argv[index], iAlgLen) == 0 && iAlgLen == iParamLen)
		{
			break;
		}
		else
		{
			if (++iCountI == dim(gReboot))
				rtn = E_COMMAND_ERROR_REBOOT;
		}
	}	
	
	return rtn;
}

int ParseShellCellGetTemperatureCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;

	if (argc < iStartPos || argc != COMMANDLINE_4)
		rtn = E_COMMAND_ERROR_PARAM_NUM;

	return rtn;
}

int ParseShellCellTftpCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;
	//int i = 0, index = 0, iCountI = 0;
	//int iParamLen = 0, iAlgLen = 0;

	if (argc < iStartPos || argc != COMMANDLINE_6)
		return (rtn = E_COMMAND_ERROR_PARAM_NUM);

	return rtn;
}

int ParseShellCellSetLedCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;
	int i = 0, index = 0, iCountI = 0;
	int iParamLen = 0, iAlgLen = 0;

	if (argc < iStartPos || argc != COMMANDLINE_5)
		return (rtn = E_COMMAND_ERROR_PARAM_NUM);

	index = iStartPos;
	iParamLen = strlen(argv[index]);
	for (i = 0; i < dim(gSetLed); i++)
	{
		iAlgLen = strlen((char *)gSetLed[i].bySetLed);
		if (strncasecmp((char *)gSetLed[i].bySetLed, argv[index], iAlgLen) == 0 && iAlgLen == iParamLen)
		{
			break;
		}
		else
		{
			if (++iCountI == dim(gSetLed))
				rtn = E_COMMAND_ERROR_LED;
		}
	}	
	
	return rtn;
}


int ParseShellCellStartTestCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;

	if (argc < iStartPos || argc != COMMANDLINE_4)
		rtn = E_COMMAND_ERROR_PARAM_NUM;

	return rtn;
}

int ParseShellCellGetTestResultCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;

	if (argc < iStartPos || argc != COMMANDLINE_4)
		rtn = E_COMMAND_ERROR_PARAM_NUM;

	return rtn;
}

//自定义命令
int ParseShellCellCustomCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;

	if (argc < iStartPos || argc != COMMANDLINE_5)
		return (rtn = E_COMMAND_ERROR_PARAM_NUM);

	return rtn;
}

int ParseShellCellReloginCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;

	if (argc < iStartPos || argc != COMMANDLINE_4)
		rtn = E_COMMAND_ERROR_PARAM_NUM;

	return rtn;
}

int ParseShellCellSetFreqCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;

	if (argc < iStartPos || argc != COMMANDLINE_6)
		return (rtn = E_COMMAND_ERROR_PARAM_NUM);

	return rtn;
}

int ParseShellCellResetFlashCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;

	if (argc < iStartPos || argc != COMMANDLINE_4)
		rtn = E_COMMAND_ERROR_PARAM_NUM;

	return rtn;
}

int ParseShellSetPoolCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;

	if (argc < iStartPos || argc != COMMANDLINE_4)
		rtn = E_COMMAND_ERROR_PARAM_NUM;

	return rtn;
}

//在PackComm()函数中被调用
int ParseShellCommand(int argc, char **argv, int iStartPos)
{
	int rtn = -1;
	int i = 0, j = 0, k = 0, l = 0, index = 0;
	int iControlLen = 0, iHeadLen = 0, iCommandLen = 0, iSecndParamLen = 0, iThirdParamLen = 0, iFourthParamLen = 0, iArgLen = 0;
	int iCountI = 0, iCountJ[MAX_COMMAND_NUM] = {0};
	devlist_st *pNode, *tmp = NULL;
	//int m = 0;
	BYTE bySrcBuf[BUFFER_LEN_256] = {0};
	int iListLen = 0;//, iId = 0;
	int argvlen = 0;
	
	DEBUGMSG(1,("%s..., argc:%d\r\n", __FUNCTION__, argc));

	if (argc < iStartPos || argc < COMMANDLINE_4)
		return (rtn = E_COMMAND_ERROR_PARAM_NUM);
	
	index = iStartPos;
	iSecndParamLen = strlen(argv[index]);
	iThirdParamLen = strlen(argv[index+1]);
	iFourthParamLen = strlen(argv[index+2]);
	iControlLen = dim(idControlInput);

	//for (m = 0; m < MAX_MINER_COMMAND_NUM; m++)
	//	DEBUGMSG(1,("1 giSendMinerCommTimer[%d]:%ld\r\n", m, giSendMinerCommTimer[m]));
	
	for (i = 0; i < iControlLen; i++)
	{		
		iHeadLen = strlen(idControlInput[i].pstrNodeHead);
		if (strncasecmp(idControlInput[i].pstrNodeHead, argv[index], iHeadLen) == 0 && iHeadLen == iSecndParamLen)
		{
			pthread_mutex_lock(idControlInput[i].pList->ListLenLock);
			iListLen = idControlInput[i].pList->CountUsersStr(*idControlInput[i].pList->StrHUsers);
			pthread_mutex_unlock(idControlInput[i].pList->ListLenLock);

			DEBUGMSG(1,("iListLen:%d\r\n", iListLen));
			
			if (iListLen == 0)
				return E_COMMAND_ERROR_EMPTY;
		
			for (j = 0; j < *idControlInput[i].pCommandLen / sizeof(idControlInput[i].pIdCommandPack[0]); j++)
			{	
				if (j == 0)	//不支持get_stat命令
				{
					++iCountJ[i];
					continue;
				}
				
				iCommandLen = strlen(idControlInput[i].pIdCommandPack[j].pstrControl);
				DEBUGMSG(0,("i:%d, j :%d, iFourthParamLen:%d\r\n", i, j, iFourthParamLen));
				DEBUGMSG(0,("pstrNodeHead:%s, index:%d, argv:%s, iCommandLen:%d\n", 
					idControlInput[i].pIdCommandPack[j].pstrControl, index+2, argv[index+2], iCommandLen));
				if (strncasecmp(idControlInput[i].pIdCommandPack[j].pstrControl, argv[index+2], iCommandLen) == 0 && iCommandLen == iFourthParamLen)
				{
					(rtn = idControlInput[i].pIdCommandPack[j].CommandParseFunc(argc, argv, index+3));
					DEBUGMSG(1,("^^^^^^^^^^^^^^^^^^^^rtn:%d\r\n", rtn));
					if ( rtn != 0)
					{
						break;
					}

					//查看是否是全部发送
					argvlen = strlen(argv[index+1]);
					if (strncmp(argv[index+1], "all", 3) == 0 && argvlen == 3)
					{
						CommShellFlag.bMassFlag = TRUE;
						//iId = 0;
					
						pthread_mutex_lock(idControlInput[i].pList->CommRemLock);
						HASH_ITER(hh, *idControlInput[i].pList->StrHUsers, pNode, tmp)
						{
							pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
							//只对在线的设备进行发送重新登录命令，同时关闭掉了状态查询。
							if (((stCellConn *)pNode->val)->CellData.bCellOnline == TRUE)
							{
								//只有stat、relogin、reboot、resetflash有群发
								if (j <= E_COMMAND_TYPE_RESETFLASH)
									((stCellConn *)pNode->val)->eCellComm = j;
							}
							pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
						}
						pthread_mutex_unlock(idControlInput[i].pList->CommRemLock); 			
					}
					else
					{
						//查找ID
						//iId = strtoul(argv[index+1], 0, 0);
						//pNode = idControlInput[i].pList->FindUserInt(idControlInput[i].pList->HashUsers, iId);
						pNode = idControlInput[i].pList->FindUserStr(idControlInput[i].pList->StrHUsers, argv[index+1]);
						if (pNode == NULL)
						{
							rtn = E_COMMAND_ERROR_ID;
							break;
						}
					}
					
					for (k = 0; k < MAX_CLIENTS; k++)
					{						
						DEBUGMSG(1,("k:%d, bUserFlag:%d\r\n", k, CommShellFlag.bUserFlag[k]));					
						if (CommShellFlag.bUserFlag[k] == TRUE)
						{
							if (iThirdParamLen > BUFFER_LEN_32)
							{
								return E_COMMAND_ERROR_ID;
							}
							pthread_mutex_lock(CommShellFlag.CommShellFlagLock);
							strncpy(idControlInput[i].pIdCommandPack[j].pDevId, argv[index+1], iThirdParamLen);
							pthread_mutex_unlock(CommShellFlag.CommShellFlagLock);

							if (gSendSeq >= SEND_ID_LENGTH)
								gSendSeq = 0;
							pthread_mutex_lock(CommShellFlag.UserSeqLock);
							CommShellFlag.uUserSeq[k] = ++gSendSeq;
							pthread_mutex_unlock(CommShellFlag.UserSeqLock);

							//打包发送序列号、id、command
							//if (CommShellFlag.bMassFlag)
							snprintf((char *)bySrcBuf, BUFFER_LEN_256, "%d", gSendSeq);
							//else								
							//	snprintf((char *)bySrcBuf, BUFFER_LEN_256, "%d %d %s", gSendSeq, iId, argv[index+2]);
							iArgLen = strlen((char *)bySrcBuf);
							DEBUGMSG(0,("i:%d, j:%d, bySrcBuf:%s, iArgLen:%d\r\n", i, j, (char *)bySrcBuf, iArgLen));
							
							//把第五个之后的参数打包到缓冲区里l=4
							for (l = index+3; l < argc; l++)
							{
								snprintf((char *)bySrcBuf + iArgLen, sizeof(bySrcBuf), " %s", argv[l]);
								iArgLen += strlen(argv[l]) + 1;
								DEBUGMSG(1,("argv[%d]:%s\r\n", l, argv[l]));
							}
							//DEBUGMSG(0,("1 -----------i:%d, j:%d, @gMinerSrcBuf[1]:%p, @:%p, pSrcBuf:%s, len:%d\r\n", i, j, gMinerSrcBuf[1], idControlInput[i].pIdCommandPack[j].pSrcBuf, idControlInput[i].pIdCommandPack[j].pSrcBuf, strlen(idControlInput[i].pIdCommandPack[j].pSrcBuf)));

							pthread_mutex_lock(CommShellFlag.CommShellFlagLock);
							memset(idControlInput[i].pIdCommandPack[j].pSrcBuf, 0, BUFFER_LEN_256);
							strncpy(idControlInput[i].pIdCommandPack[j].pSrcBuf, (char *)bySrcBuf, iArgLen);
							idControlInput[i].pShellFlag[j][k] = TRUE;
							pthread_mutex_unlock(CommShellFlag.CommShellFlagLock);
										
							CommShellFlag.bUserFlag[k] = FALSE;
							rtn = 0;
							DEBUGMSG(1,("\r\ni:%d, j:%d, k:%d, iArgLen:%d, bySrcBuf:%s, pSrcBuf:%s\r\n", i, j, k, iArgLen, (char *)bySrcBuf, idControlInput[i].pIdCommandPack[j].pSrcBuf));
							break;
						}
					}
				}
				else
				{
					//DEBUGMSG(0,("2 <<<<i:%d, pCommandLen:%d, %d, iCountJ[%d]:%d\r\n", i, *idControlInput[i].pCommandLen, sizeof(idControlInput[i].pIdCommandPack[0]), i, iCountJ[i]));
					if (++iCountJ[i] == *idControlInput[i].pCommandLen / sizeof(idControlInput[i].pIdCommandPack[0]))
						rtn = E_COMMAND_ERROR_FOURTH_PARAM;
				}
			}
		}
		else
		{
			if (rtn > 0)
				break;
			
			if (++iCountI == iControlLen)
				rtn = E_COMMAND_ERROR_TYPE;
		}
	}
	DEBUGMSG(0,("2 rtn:%d\r\n", rtn));
	
	return rtn;
}

//在PackMonitorToComm()函数中被调用
int ParseMonitorCommand(int argc, char **argv, int iStartPos, stPartData *pParData)
{
	int rtn = -1;
	int i = 0, j = 0, l = 0, index = 0;// k = 0,
	int iControlLen = 0, iHeadLen = 0, iCommandLen = 0, iSecndParamLen = 0, iThirdParamLen = 0, iFourthParamLen = 0, iArgLen = 0;
	int iCountI = 0, iCountJ[MAX_COMMAND_NUM] = {0};
	devlist_st *pNode, *tmp = NULL;
	//int m = 0;
	BYTE bySrcBuf[BUFFER_LEN_256] = {0};
	int iListLen = 0;//, iId = 0;
	int argvlen = 0;
	
	DEBUGMSG(1,("%s..., argc:%d\r\n", __FUNCTION__, argc));

	if (argc < iStartPos || argc < COMMANDLINE_4)
		return (rtn = E_COMMAND_ERROR_PARAM_NUM);
	
	index = iStartPos;
	iSecndParamLen = strlen(argv[index]);
	iThirdParamLen = strlen(argv[index+1]);
	iFourthParamLen = strlen(argv[index+2]);
	iControlLen = dim(idControlInput);

	//for (m = 0; m < MAX_MINER_COMMAND_NUM; m++)
	//	DEBUGMSG(1,("1 giSendMinerCommTimer[%d]:%ld\r\n", m, giSendMinerCommTimer[m]));
	
	for (i = 0; i < iControlLen; i++)
	{		
		iHeadLen = strlen(idControlInput[i].pstrNodeHead);
		if (strncasecmp(idControlInput[i].pstrNodeHead, argv[index], iHeadLen) == 0 && iHeadLen == iSecndParamLen)
		{
			pthread_mutex_lock(idControlInput[i].pList->ListLenLock);
			iListLen = idControlInput[i].pList->CountUsersStr(*idControlInput[i].pList->StrHUsers);
			pthread_mutex_unlock(idControlInput[i].pList->ListLenLock);

			DEBUGMSG(1,("iListLen:%d\r\n", iListLen));
			
			if (iListLen == 0)
				return E_COMMAND_ERROR_EMPTY;
		
			for (j = 0; j < *idControlInput[i].pCommandLen / sizeof(idControlInput[i].pIdCommandPack[0]); j++)
			{	
				if (j == 0)	//不支持get_stat命令
				{
					++iCountJ[i];
					continue;
				}
				
				iCommandLen = strlen(idControlInput[i].pIdCommandPack[j].pstrControl);
				DEBUGMSG(0,("i:%d, j :%d, iFourthParamLen:%d\r\n", i, j, iFourthParamLen));
				DEBUGMSG(0,("pstrNodeHead:%s, index:%d, argv:%s, iCommandLen:%d\n", 
					idControlInput[i].pIdCommandPack[j].pstrControl, index+2, argv[index+2], iCommandLen));
				if (strncasecmp(idControlInput[i].pIdCommandPack[j].pstrControl, argv[index+2], iCommandLen) == 0 && iCommandLen == iFourthParamLen)
				{
					(rtn = idControlInput[i].pIdCommandPack[j].CommandParseFunc(argc, argv, index+3));
					DEBUGMSG(1,("?????????????????????????rtn:%d\r\n", rtn));
					if ( rtn != 0)
					{
						break;
					}

					//查看是否是全部发送
					argvlen = strlen(argv[index+1]);
					if (strncmp(argv[index+1], "all", 3) == 0 && argvlen == 3)
					{
						CommMonitorFlag.bMassFlag = TRUE;
						//iId = 0;
					
						pthread_mutex_lock(idControlInput[i].pList->CommRemLock);
						HASH_ITER(hh, *idControlInput[i].pList->StrHUsers, pNode, tmp)
						{
							pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
							//只对在线的设备进行发送重新登录命令，同时关闭掉了状态查询。
							if (((stCellConn *)pNode->val)->CellData.bCellOnline == TRUE)
							{
								//只有stat、relogin、reboot、resetflash有群发
								if (j <= E_COMMAND_TYPE_SETPOOL)
									((stCellConn *)pNode->val)->eCellComm = j;
							}
							pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
						}
						pthread_mutex_unlock(idControlInput[i].pList->CommRemLock); 			
					}
					//部分设备
					/*else if (strncmp(argv[index+1], "part", 4) == 0 && argvlen == 4)
					{
						CommMonitorFlag.bPartFlag = TRUE;				
						pthread_mutex_lock(idControlInput[i].pList->CommRemLock);
						pNode = idControlInput[i].pList->FindUserInt(idControlInput[i].pList->HashUsers, iId);						
						//HASH_ITER(hh, *idControlInput[i].pList->HashUsers, pNode, tmp)
						if (pNode != NULL)
						{
							pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
							//只对在线的设备进行发送重新登录命令，同时关闭掉了状态查询。
							if (((stCellConn *)pNode->val)->CellData.bCellOnline == TRUE)
							{
								//只有stat、relogin、reboot、resetflash有群发
								if (j <= E_COMMAND_TYPE_RESETFLASH)
									((stCellConn *)pNode->val)->eCellComm = j;
							}
							pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
						}
						else
						{
							spnrintf(CommMonitorFlag.szIdbufRtn+CommMonitorFlag.iBufPos, sizeof(CommMonitorFlag.szIdbufRtn), "%s", iId);
							CommMonitorFlag.iBufPos += strlen(CommMonitorFlag.szIdbufRtn+CommMonitorFlag.iBufPos);
						}
						pthread_mutex_unlock(idControlInput[i].pList->CommRemLock); 			
					}*/
					else
					{
						//查找ID					
						//iId = strtoul(argv[index+1], 0, 0);
						//DEBUGMSG(1,("^^^iId:%d^^^^\n", iId));
						DEBUGMSG(1,("^^^szId:%s^^^\n", argv[index+1]));
						//pNode = idControlInput[i].pList->FindUserInt(idControlInput[i].pList->HashUsers, iId);
						pNode = idControlInput[i].pList->FindUserStr(idControlInput[i].pList->StrHUsers, argv[index+1]);
						if (pNode == NULL)
						{
							if (CommMonitorFlag.iBufPos == 0)
							{
								snprintf((char *)CommMonitorFlag.szIdbufRtn+CommMonitorFlag.iBufPos, sizeof(CommMonitorFlag.szIdbufRtn), "%s %s", szRetBuf[4], argv[index+1]);
								//CommMonitorFlag.iBufPos += strlen((char *)CommMonitorFlag.szIdbufRtn+CommMonitorFlag.iBufPos);
							}
							else
							{
								snprintf((char *)CommMonitorFlag.szIdbufRtn+CommMonitorFlag.iBufPos, sizeof(CommMonitorFlag.szIdbufRtn), " %s", argv[index+1]);
							}	
							CommMonitorFlag.iBufPos += strlen((char *)CommMonitorFlag.szIdbufRtn+CommMonitorFlag.iBufPos);
							rtn = E_COMMAND_ERROR_ID;

							//pParData->iPartLen--;
							//DEBUGMSG(1,("000 pParData->iPartLen:%d\n", pParData->iPartLen));
							DEBUGMSG(1,("1 szIdbufRtn:%s\n", (char *)CommMonitorFlag.szIdbufRtn));
							DEBUGMSG(1,("1 iBufPos:%d\n", CommMonitorFlag.iBufPos));
							//DEBUGMSG(1,("000 iPartLen:%d, iParListLen:%d\n", pParData->iPartLen, pParData->iPartListLen));
							//if (pParData->iPartListLen < pParData->iPartLen)
							//	break;
						}
						else
						{
							pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
							//只对在线的设备进行发送重新登录命令，同时关闭掉了状态查询。
							if (((stCellConn *)pNode->val)->CellData.bCellOnline == TRUE)
							{
								//只有stat、relogin、reboot、resetflash有群发
								if (j <= E_COMMAND_TYPE_SETLED)
									((stCellConn *)pNode->val)->eCellComm = j;
							}
							pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
							DEBUGMSG(1,("init eCellComm:%d\n", ((stCellConn *)pNode->val)->eCellComm));
						}
					}
					
					//for (k = 0; k < MAX_CLIENTS; k++)
				//{						
					DEBUGMSG(1,("bServerCommFlag:%d\r\n", CommMonitorFlag.bServerCommFlag));					
					if (CommMonitorFlag.bServerCommFlag == TRUE)
					{
						if (iThirdParamLen > BUFFER_LEN_32)
						{
							return E_COMMAND_ERROR_ID;
						}
						/*pthread_mutex_lock(CommMonitorFlag.CommMonitorFlagLock);
						strncpy(idControlInput[i].pIdCommandPack[j].pDevId, argv[index+1], iThirdParamLen);
						pthread_mutex_unlock(CommMonitorFlag.CommMonitorFlagLock);*/

						//把id保存下来
						DEBUGMSG(1,("1 iPartListLen:%d\n", pParData->iPartListLen));
						if (rtn == 0)
							//pParData->iPartIdBuf[pParData->iPartListLen++] = strtoul(argv[2], 0, 0);
							strncpy(pParData->iPartIdBuf[pParData->iPartListLen++], argv[2], sizeof(pParData->iPartIdBuf[pParData->iPartListLen++]));	
						else if (rtn == 3)
							pParData->iPartLen--;
						
						DEBUGMSG(1,("1 iPartLen:%d, iParListLen:%d\n", pParData->iPartLen, pParData->iPartListLen));
						if (pParData->iPartLen == pParData->iPartListLen)
						{
							if (gSendSeq >= SEND_ID_LENGTH)
								gSendSeq = 0;
							pthread_mutex_lock(CommMonitorFlag.UserSeqLock);
							CommMonitorFlag.uUserSeq = ++gSendSeq;
							pthread_mutex_unlock(CommMonitorFlag.UserSeqLock);
							
							//打包发送序列号、id、command
							//if (CommMonitorFlag.bMassFlag)
							snprintf((char *)bySrcBuf, BUFFER_LEN_256, "%d", gSendSeq);
							//else								
								//snprintf((char *)bySrcBuf, BUFFER_LEN_256, "%d %d %s", gSendSeq, iId, argv[index+2]);
							iArgLen = strlen((char *)bySrcBuf);
							DEBUGMSG(1,("i:%d, j:%d, bySrcBuf:%s, iArgLen:%d\r\n", i, j, (char *)bySrcBuf, iArgLen));
							
							//把第五个之后的参数打包到缓冲区里l=4
							for (l = index+3; l < argc; l++)
							{
								snprintf((char *)bySrcBuf + iArgLen, sizeof(bySrcBuf), " %s", argv[l]);
								iArgLen += strlen(argv[l]) + 1;
								DEBUGMSG(1,("argv[%d]:%s\r\n", l, argv[l]));
							}
							//DEBUGMSG(0,("1 -----------i:%d, j:%d, @gMinerSrcBuf[1]:%p, @:%p, pSrcBuf:%s, len:%d\r\n", i, j, gMinerSrcBuf[1], idControlInput[i].pIdCommandPack[j].pSrcBuf, idControlInput[i].pIdCommandPack[j].pSrcBuf, strlen(idControlInput[i].pIdCommandPack[j].pSrcBuf)));
							
							pthread_mutex_lock(CommMonitorFlag.CommMonitorFlagLock);
							memset(idControlInput[i].pIdCommandPack[j].pSrcBuf, 0, BUFFER_LEN_256);
							strncpy(idControlInput[i].pIdCommandPack[j].pSrcBuf, (char *)bySrcBuf, iArgLen);
							idControlInput[i].pMonitorFlag[j] = TRUE;
							pthread_mutex_unlock(CommMonitorFlag.CommMonitorFlagLock);
										
							CommMonitorFlag.bServerCommFlag = FALSE;
							rtn = 0;
							DEBUGMSG(1,("1 pMonitorFlag[%d]:%d\n", j, idControlInput[i].pMonitorFlag[j]));
							DEBUGMSG(1,("\r\ni:%d, j:%d, iArgLen:%d, bySrcBuf:%s, pSrcBuf:%s\r\n", i, j, iArgLen, (char *)bySrcBuf, idControlInput[i].pIdCommandPack[j].pSrcBuf));
							break;							
						}
						else
						{
							rtn = E_COMMAND_ERROR_PARTID;
							return rtn;
						}
					}
				//}
				}
				else
				{
					//DEBUGMSG(0,("2 <<<<i:%d, pCommandLen:%d, %d, iCountJ[%d]:%d\r\n", i, *idControlInput[i].pCommandLen, sizeof(idControlInput[i].pIdCommandPack[0]), i, iCountJ[i]));
					if (++iCountJ[i] == *idControlInput[i].pCommandLen / sizeof(idControlInput[i].pIdCommandPack[0]))
						rtn = E_COMMAND_ERROR_FOURTH_PARAM;
				}
			}
		}
		else
		{
			if (rtn > 0)
				break;
			
			if (++iCountI == iControlLen)
				rtn = E_COMMAND_ERROR_TYPE;
		}
	}
	DEBUGMSG(1,("2 rtn:%d\r\n", rtn));
	
	return rtn;
}

/*int ParseRecvMinerListCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;
	devlist_ist *Node;
	BYTE byMinerIp[BUFFER_LEN_16] = {0};	//miner的ip
	int	iMinerPort = 0;						//miner的port
	int iId = 0;
	char bufDest[BUFFER_LEN_512] = {0};
	char bufComm[BUFFER_LEN_32] = {0};
	long iUpdateTime = 0;
		
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	if (argc < iStartPos || argc != COMMANDLINE_10)
		return (rtn = E_RECV_COMMAND_ERROR_PARAM_NUM);
	
	//查找MinerId	
	pthread_mutex_lock(stMinerUtlist.CommRemLock);
	iId = strtoul(argv[2], 0, 0);
	Node = stMinerUtlist.FindUserInt(stMinerUtlist.HashUsers, iId);
	if (Node != NULL)
	{
		pthread_mutex_lock(&((stMinerConn *)Node->val)->MinerDataLock.AlarmTimerLock);
		iUpdateTime = ((stMinerConn *)Node->val)->MinerData.iMinerAlarmTimer = GetTime();
		pthread_mutex_unlock(&((stMinerConn *)Node->val)->MinerDataLock.AlarmTimerLock);
		
		strncpy((char *)byMinerIp, argv[7], sizeof(byMinerIp));
		iMinerPort = atoi(argv[8]); 
		if (strcmp(inet_ntoa(((stMinerConn *)Node->val)->remote.addr4.sin_addr), (char *)byMinerIp) != 0
			|| ((stMinerConn *)Node->val)->remote.addr4.sin_port != htons(iMinerPort))
		{
			DEBUGMSG(0,("------------byMinerIp:%s, iMinerPort:%d\r\n", (char *)byMinerIp, iMinerPort));
			printf("sin_port:%d, net:%d, host:%d\r\n", ((stMinerConn *)Node->val)->remote.addr4.sin_port, htons(iMinerPort), iMinerPort);
			pthread_mutex_lock(&((stMinerConn *)Node->val)->MinerDataLock.IpPortLock);
			SetClientIp4(&(((stMinerConn *)Node->val)->remote.addr4), byMinerIp, iMinerPort);
			pthread_mutex_unlock(&((stMinerConn *)Node->val)->MinerDataLock.IpPortLock);
			//有数据已经插入数据库后，才能进行更新。
			if (((stMinerConn *)Node->val)->eMinerMySql != E_QUERY_TYPE_INSERT)				
				((stMinerConn *)Node->val)->eMinerMySql = E_QUERY_TYPE_UPDATE;
		}

		if (bIniMinerStatus)
		{
			//<elapse><pool_id><pool_stat>	update_time
			snprintf(bufComm, sizeof(bufComm), "%d,%s,%s,%s,%ld", iId, argv[4], argv[5], argv[6], iUpdateTime);

			DEBUGMSG(0,("bufComm:%s\r\n", bufComm));
			MinerStatusQuery[E_QUERY_TYPE_INSERT - 1].QueryPackFunc1(bufComm, &gCfgDataDL.MinerStatusInfo, &gCfgDataDL.MinerStaColName, bufDest, sizeof(bufDest));
			MysqlFunc.dbDoQuery(&myMinerStatus, bufDest, E_QUERY_TYPE_INSERT, NULL);
			memset(bufDest, 0, sizeof(bufDest));
		}
	}
    else
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, cirqueue_create Generated the que!\r\n",  __FILE__, __FUNCTION__, __LINE__);
    }
	pthread_mutex_unlock(stMinerUtlist.CommRemLock);

	return rtn;
}*/
/*************************************************************
	算力计算方法

	挖矿算力即矿池分配给矿机的difficulty

	hashSum = diff1 + diff2 + ... + diffn;

	//不同算法具有不同的share
	BTC：0xFFFFFFFF //4294967295
	LTC: 0xFFFF  //65535

	//假设hashSum是某台矿机1分钟的总算力
	//单位：khash/s
	hashrate = hashSum * share  / 60 / 1000;
*************************************************************/
int ParseRecvCellListCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;
	devlist_st *Node;
	BYTE byCellIp[BUFFER_LEN_16];       //cell的ip	
	int	iCellPort = 0;                  //cell的port	
	//int iId = 0;
	char bufDest[BUFFER_LEN_1024] = {0};
	char bufComm[BUFFER_LEN_512] = {0};
	long iUpdateTime = 0;
	LLONG ullBTCNcsend = 0, ullLTCNcsend = 0;
	int iIndex = 0;

	DEBUGMSG(1,("%s..., argc:%d, iStartPos:%d\r\n", __FUNCTION__, argc, iStartPos));
//#ifdef _DEBUG_MANY_DEV_
	//printf("%s..., argc:%d, iStartPos:%d\r\n", __FUNCTION__, argc, iStartPos);
//#endif
	if (argc < iStartPos || argc != COMMANDLINE_18)
		return (rtn = E_RECV_COMMAND_ERROR_PARAM_NUM);
	
	pthread_mutex_lock(stCellUtlist.CommRemLock);
	//查找CellId
	//iId = strtoul(argv[2], 0, 0);
	//printf("iId:%d\r\n", iId);
	//Node = stCellUtlist.FindUserInt(stCellUtlist.HashUsers, iId);
	Node = stCellUtlist.FindUserStr(stCellUtlist.StrHUsers, argv[2]);
	if (Node != NULL)
	{
		pthread_mutex_lock(&((stCellConn *)Node->val)->CellDataLock.AlarmTimerLock);
		iUpdateTime = ((stCellConn *)Node->val)->CellData.iCellAlarmTimer = GetTime();
		pthread_mutex_unlock(&((stCellConn *)Node->val)->CellDataLock.AlarmTimerLock);

		strncpy((char *)byCellIp, argv[15], sizeof(byCellIp));
		iCellPort = atoi(argv[16]);
		
		if (strcmp(inet_ntoa(((stCellConn *)Node->val)->remote.addr4.sin_addr), (char *)byCellIp) != 0
			|| ((stCellConn *)Node->val)->remote.addr4.sin_port != htons(iCellPort))
		{
			pthread_mutex_lock(&((stCellConn *)Node->val)->CellDataLock.IpPortLock);
			SetClientIp4(&(((stCellConn *)Node->val)->remote.addr4), byCellIp, iCellPort);
			pthread_mutex_unlock(&((stCellConn *)Node->val)->CellDataLock.IpPortLock);
			//有数据已经插入数据库后，才能进行更新。
			if (((stCellConn *)Node->val)->eCellMySql != E_QUERY_TYPE_INSERT)					
				((stCellConn *)Node->val)->eCellMySql = E_QUERY_TYPE_UPDATE;
		}

		//更新对应的数据
		//strncpy((char *)((stCellConn *)Node->val)->CellStat.byTestResult, argv[4], sizeof(((stCellConn *)Node->val)->CellStat.byTestResult));
		iIndex = 4;
		((stCellConn *)Node->val)->CellStat.iEntryTemp = strtoul(argv[iIndex++], 0, 0);
		((stCellConn *)Node->val)->CellStat.iExitTemp = strtoul(argv[iIndex++], 0, 0);
		iIndex++;
		((stCellConn *)Node->val)->CellStat.iElapse = strtoul(argv[iIndex++], 0, 0);	
		ullBTCNcsend = strtoull(argv[iIndex++], 0, 0);
		((stCellConn *)Node->val)->CellStat.ullBTCReject = strtoull(argv[iIndex++], 0, 0);
		((stCellConn *)Node->val)->CellStat.ullBTCReject = strtoull(argv[iIndex++], 0, 0);
		ullLTCNcsend = strtoull(argv[iIndex++], 0, 0);
		((stCellConn *)Node->val)->CellStat.ullLTCReject = strtoull(argv[iIndex++], 0, 0);
		((stCellConn *)Node->val)->CellStat.ullLTCAccept = strtoull(argv[iIndex++], 0, 0);
		DEBUGMSG(0,("iEntryTemp:%d, ullLTCAccept:%llu\n", 
			((stCellConn *)Node->val)->CellStat.iEntryTemp, ((stCellConn *)Node->val)->CellStat.ullLTCAccept));

		//计算实际功率
		CalculateBTCRealKHash(ullBTCNcsend, (stCellConn *)Node->val);
		DEBUGMSG(1,("&&&&&&&&&&&&&\nllBTCRealKHash:%lld\n", ((stCellConn *)Node->val)->CellStat.llBTCRealKHash));
		CalculateLTCRealKHash(ullLTCNcsend, (stCellConn *)Node->val);
		DEBUGMSG(1,("#############\nllLTCRealKHash:%lld\n", ((stCellConn *)Node->val)->CellStat.llLTCRealKHash));	

		//更新30 Min 内的BTCNcsend和LTCNcsend。
		if (bIniCellBase)
		{
			CellSqlQuery[E_QUERY_TYPE_UPDATE - 1].QueryPackFunc3(
															Node->val, 
															&gCfgDataDL.CellBaseInfo, 
															&gCfgDataDL.CellColName, 
															bufDest, sizeof(bufDest));
			DEBUGMSG(0,("bufDest:%s\r\n", bufDest));
			pthread_mutex_lock(&gCellBaseLock);
			MysqlFunc.dbDoQuery(&myCellBase, bufDest, E_QUERY_TYPE_UPDATE, NULL);
			pthread_mutex_unlock(&gCellBaseLock);
			memset(bufDest, 0, sizeof(bufDest));	
		}	

#if 1
		DEBUGMSG(0,("bIniCellStatus:%d\r\n", bIniCellStatus));
		if (bIniCellStatus)
		{
			//<entry_temp><exit_temp><led_status><elapse><btc_wb_lost><btc_nc_send><btc_nc_hw>
			//<btc_nc_accept><btc_nc_reject><ltc_wb_lost><ltc_nc_send><ltc_nc_hw><ltc_nc_accept>
			//<ltc_nc_reject><chip_temp_array>  update_time
			DEBUGMSG(0,("argv[4]:%s, argv[17]:%s\r\n", argv[4], argv[14]));
			snprintf(bufComm, sizeof(bufComm), "'%s',%d,%d,%s,%d,%llu,%llu,%llu,%llu,%llu,%llu,'%s',%ld", 
				((stCellConn *)Node->val)->CellData.szCellId, 
				((stCellConn *)Node->val)->CellStat.iEntryTemp, 
				((stCellConn *)Node->val)->CellStat.iExitTemp, 
				argv[6], //led_status
				((stCellConn *)Node->val)->CellStat.iElapse, 
				ullBTCNcsend, 
				((stCellConn *)Node->val)->CellStat.ullBTCReject,
				((stCellConn *)Node->val)->CellStat.ullBTCReject,
				ullLTCNcsend, 
				((stCellConn *)Node->val)->CellStat.ullBTCReject,
				((stCellConn *)Node->val)->CellStat.ullBTCReject,
				argv[14], //chips_temp 
				iUpdateTime);	//, argv[15], argv[16], argv[17]

			DEBUGMSG(0,("bufComm:%s\r\n", bufComm));
			CellStatusQuery[E_QUERY_TYPE_INSERT - 1].QueryPackFunc1(bufComm, &gCfgDataDL.CellStatusInfo, &gCfgDataDL.CellStaColName, bufDest, sizeof(bufDest));

			pthread_mutex_lock(&gCellStatusLock);
			MysqlFunc.dbDoQuery(&myCellStatus, bufDest, E_QUERY_TYPE_INSERT, NULL);
			pthread_mutex_unlock(&gCellStatusLock);
			
			memset(bufDest, 0, sizeof(bufDest));	
		}
		
#endif			
		//数据已更新，可以发给dataserver
		if (((stCellConn *)Node->val)->eCellDataSer == E_DEV_DATAUPDATE_NEWCELL)
			gNewCellSendLen++;
			
		((stCellConn *)Node->val)->eCellDataSer = E_DEV_DATAUPDATE_CELLNEWDATA;
	}
	else
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, cirqueue_create Generated the que!\r\n",  
							__FILE__, __FUNCTION__, __LINE__);
	}

	pthread_mutex_unlock(stCellUtlist.CommRemLock);

	return rtn;
}

int ParseRecvShellMonitorCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;
	int i = 0;
	unsigned int uId = 0;
	devlist_st *Node;
	BYTE byCellIp[BUFFER_LEN_16];       //cell的ip	
	int	iCellPort = 0;                  //cell的port	
	//int iCellId = 0;

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	if (argc < iStartPos || argc != COMMANDLINE_7)
		return (rtn = E_RECV_COMMAND_ERROR_PARAM_NUM);

	uId = strtoul(argv[1], 0, 0);
	//iCellId = strtoul(argv[2], 0, 0);

	//DEBUGMSG(1,("uId:%d, iCellId:%d, gCommandType:%d\r\n", uId, iCellId, gCommandType));
	DEBUGMSG(1,("uId:%d, szCellId:%s, gCommandType:%d\r\n", uId, argv[2], gCommandType));
	//临时处理
	if (gCommandType == E_COMM_TYPE_SHELL)
	{
		pthread_mutex_lock(CommShellFlag.UserSeqLock);
		for (i = 0; i < MAX_CLIENTS; i++)
		{
			if (CommShellFlag.uUserSeq[i] == uId)
				CommShellFlag.bRecvSeqFlag[i] = TRUE;
		}	
		pthread_mutex_unlock(CommShellFlag.UserSeqLock);
	}
	else if (gCommandType == E_COMM_TYPE_MONITOR)
	{
		//Node = stCellUtlist.FindUserInt(stCellUtlist.HashUsers, iCellId);
		Node = stCellUtlist.FindUserStr(stCellUtlist.StrHUsers, argv[2]);	
		if (Node != NULL)
		{		
			//DEBUGMSG(0,("uSeqId:%d, uId:%d, iCellId:%d\r\n", ((stCellConn *)Node->val)->uSeqId, uId, iCellId));
			if (((stCellConn *)Node->val)->uSeqId == uId)
			{
				//复位
				((stCellConn *)Node->val)->uSeqId = 0;
			}
			else
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, ParseRecvCommand() %s\r\n",  
									__FILE__, __FUNCTION__, __LINE__, gCommParseErr[E_RECV_COMMAND_ERROR_USER_SEQ - 1].cmd);
				pthread_mutex_unlock(stCellUtlist.CommRemLock);
				return (rtn = E_RECV_COMMAND_ERROR_USER_SEQ);
			}
		
			strncpy((char *)byCellIp, argv[4], sizeof(byCellIp));
			iCellPort = atoi(argv[5]);
			
			if (strcmp(inet_ntoa(((stCellConn *)Node->val)->remote.addr4.sin_addr), (char *)byCellIp) != 0
				|| ((stCellConn *)Node->val)->remote.addr4.sin_port != htons(iCellPort))
			{
				pthread_mutex_lock(&((stCellConn *)Node->val)->CellDataLock.IpPortLock);
				SetClientIp4(&(((stCellConn *)Node->val)->remote.addr4), byCellIp, iCellPort);
				pthread_mutex_unlock(&((stCellConn *)Node->val)->CellDataLock.IpPortLock);
				//有数据已经插入数据库后，才能进行更新。
				if (((stCellConn *)Node->val)->eCellMySql != E_QUERY_TYPE_INSERT)				
					((stCellConn *)Node->val)->eCellMySql = E_QUERY_TYPE_UPDATE;
			}
			if (((stCellConn *)Node->val)->eCellComm)
				((stCellConn *)Node->val)->eCellComm = E_COMMAND_TYPE_STAT;
			DEBUGMSG(1,("-----------eCellComm:%d\r\n", ((stCellConn *)Node->val)->eCellComm));

			CommMonitorFlag.iRecvLen++;
		}
		else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, cirqueue_create Generated the que!\r\n",	
								__FILE__, __FUNCTION__, __LINE__);
		}
		pthread_mutex_unlock(stCellUtlist.CommRemLock);
	}

	return rtn;	
}

/*int ParseRecvMinerSetCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;
	devlist_ist *Node;
	BYTE byMinerIp[BUFFER_LEN_16] = {0};	//miner的ip
	int	iMinerPort = 0;						//miner的port
	int iId = 0;
		
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	if (argc < iStartPos || argc != COMMANDLINE_7)
		return (rtn = E_RECV_COMMAND_ERROR_PARAM_NUM);

	//查找MinerId	
	pthread_mutex_lock(stMinerUtlist.CommRemLock);
	iId = strtoul(argv[2], 0, 0);
	Node = stMinerUtlist.FindUserInt(stMinerUtlist.HashUsers, iId);
	if (Node != NULL)
	{
		strncpy((char *)byMinerIp, argv[4], sizeof(byMinerIp));
		iMinerPort = atoi(argv[5]); 
		if (strcmp(inet_ntoa(((stMinerConn *)Node->val)->remote.addr4.sin_addr), (char *)byMinerIp) != 0
			|| ((stMinerConn *)Node->val)->remote.addr4.sin_port != htons(iMinerPort))
		{
			DEBUGMSG(0,("------------byMinerIp:%s, iMinerPort:%d\r\n", (char *)byMinerIp, iMinerPort));
			pthread_mutex_lock(&((stMinerConn *)Node->val)->MinerDataLock.IpPortLock);
			SetClientIp4(&(((stMinerConn *)Node->val)->remote.addr4), byMinerIp, iMinerPort);
			pthread_mutex_unlock(&((stMinerConn *)Node->val)->MinerDataLock.IpPortLock);
			//有数据已经插入数据库后，才能进行更新。
			if (((stMinerConn *)Node->val)->eMinerMySql != E_QUERY_TYPE_INSERT)				
				((stMinerConn *)Node->val)->eMinerMySql = E_QUERY_TYPE_UPDATE;
		}
		if (((stMinerConn *)Node->val)->eMinerComm)
			((stMinerConn *)Node->val)->eMinerComm = E_COMMAND_TYPE_STAT;		
	}
    else
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, cirqueue_create Generated the que!\r\n",  __FILE__, __FUNCTION__, __LINE__);
    }
	pthread_mutex_unlock(stMinerUtlist.CommRemLock);

	return rtn;
}
*/

int ParseRecvCellSetCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;
	//int i = 0;
	devlist_st *Node;
	BYTE byCellIp[BUFFER_LEN_16];       //cell的ip	
	int	iCellPort = 0;                  //cell的port	
	//int iId = 0;
	//int i = 0;
	unsigned int uId = 0;

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	if (argc < iStartPos || argc != COMMANDLINE_7)
		return (rtn = E_RECV_COMMAND_ERROR_PARAM_NUM);

	uId = atoi(argv[1]);

	pthread_mutex_lock(stCellUtlist.CommRemLock);
	//查找CellId
	//iId = strtoul(argv[2], 0, 0);
	//printf("iId:%d\r\n", iId);
	//Node = stCellUtlist.pList.FindUserInt(stCellUtlist.HashUsers, iId);
	Node = stCellUtlist.FindUserStr(stCellUtlist.StrHUsers, argv[2]);		
	if (Node != NULL)
	{
		//pthread_mutex_lock(&((stCellConn *)Node->val)->CellDataLock.AlarmTimerLock);
		//iUpdateTime = ((stCellConn *)Node->val)->CellData.iCellAlarmTimer = GetTime();
		//pthread_mutex_unlock(&((stCellConn *)Node->val)->CellDataLock.AlarmTimerLock);

		DEBUGMSG(1,("uSeqId:%d, uId:%d\r\n", ((stCellConn *)Node->val)->uSeqId, uId));
		if (((stCellConn *)Node->val)->uSeqId == uId)
		{
			//复位
			((stCellConn *)Node->val)->uSeqId = 0;
		}
		else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, ParseRecvCommand() %s\r\n",  
								__FILE__, __FUNCTION__, __LINE__, gCommParseErr[E_RECV_COMMAND_ERROR_USER_SEQ - 1].cmd);
			pthread_mutex_unlock(stCellUtlist.CommRemLock);
			return (rtn = E_RECV_COMMAND_ERROR_USER_SEQ);
		}

		strncpy((char *)byCellIp, argv[4], sizeof(byCellIp));
		iCellPort = atoi(argv[5]);
		
		if (strcmp(inet_ntoa(((stCellConn *)Node->val)->remote.addr4.sin_addr), (char *)byCellIp) != 0
			|| ((stCellConn *)Node->val)->remote.addr4.sin_port != htons(iCellPort))
		{
			pthread_mutex_lock(&((stCellConn *)Node->val)->CellDataLock.IpPortLock);
			SetClientIp4(&(((stCellConn *)Node->val)->remote.addr4), byCellIp, iCellPort);
			pthread_mutex_unlock(&((stCellConn *)Node->val)->CellDataLock.IpPortLock);
			//有数据已经插入数据库后，才能进行更新。
			if (((stCellConn *)Node->val)->eCellMySql != E_QUERY_TYPE_INSERT)				
				((stCellConn *)Node->val)->eCellMySql = E_QUERY_TYPE_UPDATE;
		}
		if (((stCellConn *)Node->val)->eCellComm)
			((stCellConn *)Node->val)->eCellComm = E_COMMAND_TYPE_STAT;
		DEBUGMSG(1,("-----------eCellComm:%d\r\n", ((stCellConn *)Node->val)->eCellComm));

		//Shell或者Monitor
		if (gCommandType == E_COMM_TYPE_SHELL)
		{
			//CommShellFlag.iRecvLen++;
		}
		else if (gCommandType == E_COMM_TYPE_MONITOR)
		{
			CommMonitorFlag.iRecvLen++;
		}
	}
	else
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, cirqueue_create Generated the que!\r\n",  
							__FILE__, __FUNCTION__, __LINE__);
	}
	pthread_mutex_unlock(stCellUtlist.CommRemLock);
	
	return rtn; 
}

int ParseRecvCommand(int argc, char **argv, int iStartPos)
{
	int rtn = 0;

	DEBUGMSG(1,("1 %s...argc:%d\r\n", __FUNCTION__, argc));

	if (argc < iStartPos || argc < COMMANDLINE_6)
		return (rtn = E_RECV_COMMAND_ERROR_PARAM_NUM);

	if (strncmp(argv[0], szHead[0], 4) == 0)
	{
		/*if (strncmp(argv[1], "miner", 5) == 0)
		{
			rtn = ParseRecvMinerListCommand(argc, argv, 2);
		}
		else*/
		if (strncmp(argv[1], "cell", 4) == 0)
		{

			rtn = ParseRecvCellListCommand(argc, argv, 2);
		}
		else
		{
			rtn = E_RECV_COMMAND_ERROR_IDSECOND_PARAM;
		}
	}
	else if (strncmp(argv[0], szHead[1], 4) == 0)
	{
		rtn = ParseRecvShellMonitorCommand(argc, argv, 1);
	}
	else if (strncmp(argv[0], szHead[2], 7) == 0)
	{
		/*if (strncmp(argv[1], "miner", 5) == 0)
		{
			rtn = ParseRecvMinerSetCommand(argc, argv, 1);
		}
		else*/ 
		//if (strncmp(argv[1], "cell", 4) == 0)
		//{
		
		rtn = ParseRecvCellSetCommand(argc, argv, 1);
		//}
		//else
		//{
		//	rtn = E_RECV_COMMAND_ERROR_IDSECOND_PARAM;
		//}	
	}
	else
	{
		rtn = E_RECV_COMMAND_ERROR_IDFIRST_PARAM;
	}
	
	return rtn;
}

int CommandShell(int i, int j, int k)
{
	pthread_mutex_lock(CommShellFlag.CommShellFlagLock);
	for (k = 0; k < MAX_CLIENTS; k++)
	{	
		if (idControlInput[i].pShellFlag[j][k] == TRUE)
		{
			idControlInput[i].pShellFlag[j][k] = FALSE;
			DEBUGMSG(1,("k:%d, pShellFlag:%d\r\n", k, idControlInput[i].pShellFlag[j][k]));
	
			if (CommShellFlag.bMassFlag)
			{
				//当有用户开启群发，立即开启全部发送
				//idControlInput[i].pSendCommTimer[j] -= gCfgDataDL.SwitchInfo.iStatsInter;
					
				if (idControlInput[i].pIdCommandPack[j].CommandSendAllFunc(idControlInput[i].pList, 
												  idControlInput[i].pApiNet, 
												  idControlInput[i].pIdCommandPack,
												  j,
												  gpCommandPool->listener.sock
												 ) != 0)
				{
					return 1;
				}
				/*else
				{
					CommShellFlag.bMassFlag = FALSE;
				}*/
			}
			else
			{
				if (idControlInput[i].pIdCommandPack[j].CommandSendSingleFunc(idControlInput[i].pList, 
					idControlInput[i].pApiNet, 
					idControlInput[i].pIdCommandPack,
					j,
					gpCommandPool->listener.sock
					) != 0)
				{
					return 1;
				}
			}
		}
	}
	pthread_mutex_unlock(CommShellFlag.CommShellFlagLock);

	return 0;
}

int CommandMonitor(int i, int j)
{
	int iPartIndex = 0;

	//usleep(1000);
	//DEBUGMSG(1,("2 pMonitorFlag[%d]:%d\n", j, idControlInput[i].pMonitorFlag[j]));
	//monitor
	pthread_mutex_lock(CommMonitorFlag.CommMonitorFlagLock);		
	if (idControlInput[i].pMonitorFlag[j] == TRUE)
	{
		DEBUGMSG(1,("3 pMonitorFlag:%d\r\n", idControlInput[i].pMonitorFlag[j]));
		idControlInput[i].pMonitorFlag[j] = FALSE;
		DEBUGMSG(1,("---bMassFlag:%d\n", CommMonitorFlag.bMassFlag));
		if (CommMonitorFlag.bMassFlag)
		{
			//当有用户开启群发，立即开启全部发送
			//idControlInput[i].pSendCommTimer[j] -= gCfgDataDL.SwitchInfo.iStatsInter;
				
			if (idControlInput[i].pIdCommandPack[j].CommandSendAllFunc(idControlInput[i].pList, 
											  idControlInput[i].pApiNet, 
											  idControlInput[i].pIdCommandPack,
											  j,
											  gpCommandPool->listener.sock
											 ) != 0)
			{
				return 1;
			}
			/*else
			{
				CommShellFlag.bMassFlag = FALSE;
			}*/
		}
		else
		{
			DEBUGMSG(1,("CommMonitorFlag.iSendLen:%d, iPartListLen:%d\n", CommMonitorFlag.iSendLen, ParData.iPartListLen));
			CommMonitorFlag.iSendLen = 0;
			for (iPartIndex = 0; iPartIndex < ParData.iPartListLen; iPartIndex++)
			{
				//pthread_mutex_lock(CommMonitorFlag.CommMonitorFlagLock);
				snprintf(idControlInput[i].pIdCommandPack[j].pDevId, BUFFER_LEN_32, "%s", ParData.iPartIdBuf[iPartIndex]);
				//pthread_mutex_unlock(CommMonitorFlag.CommMonitorFlagLock);
								
				if (idControlInput[i].pIdCommandPack[j].CommandSendSingleFunc(idControlInput[i].pList, 
					idControlInput[i].pApiNet, 
					idControlInput[i].pIdCommandPack,
					j,
					gpCommandPool->listener.sock
					) != 0)
				{
					return 1;
				}
				else
				{
					CommMonitorFlag.iSendLen++;
					DEBUGMSG(1,("----------CommMonitorFlag.iSendLen:%d\n", CommMonitorFlag.iSendLen));					
				}
			}
		}
	}
	pthread_mutex_unlock(CommMonitorFlag.CommMonitorFlagLock);				

	return 0;
}

void *CommandPackProcess(void *arg)
{
	int i = 0, j = 0, k = 0;
	int iListLen = 0;
	
	DEBUGMSG(1,("3 %s...\r\n", __FUNCTION__));
	//DEBUGMSG(0,("^^^^^%d\r\n",  *idControlInput[0].pCommandLen));
	//DEBUGMSG(0,("Command sock:%d\r\n", gpCommandPool->listener.sock));
	while(1)
	{
		for (i = 0; i < dim(idControlInput); i++)
		{
			pthread_mutex_lock(idControlInput[i].pList->ListLenLock);
			iListLen = idControlInput[i].pList->CountUsersStr(*idControlInput[i].pList->StrHUsers);
			pthread_mutex_unlock(idControlInput[i].pList->ListLenLock);

			//如果链表为空则返回
			if (*idControlInput[i].pList->StrHUsers == NULL || iListLen == 0)
			{
				MSleep(100);
				idControlInput[i].pSendCommTimer[j] = GetTime();
				continue;
			}

			DEBUGMSG(0,("iStatsInter:%d\r\n", gCfgDataDL.SwitchInfo.iStatsInter));
	
			for (j = 0; j < *idControlInput[i].pCommandLen / sizeof(idControlInput[i].pIdCommandPack[0]); j++)
			{
				if (j <= geCommLoopType)
				{
					if ((GetTime() - idControlInput[i].pSendCommTimer[j]) >= gCfgDataDL.SwitchInfo.iStatsInter)
					//if ((GetTime() - idControlInput[i].pSendCommTimer[j]) >= 25)//测试用
					{
						DEBUGMSG(1,("j:%d, geCommLoopType:%d\r\n", j, geCommLoopType));
						DEBUGMSG(0,("GetTime():%ld, i:%d, j:%d, Timer:%ld\r\n", GetTime(), i, j, idControlInput[i].pSendCommTimer[j]));
						idControlInput[i].pSendCommTimer[j] = GetTime();
						if (bCommStaTime)
						{
							gCommStartTime = GetTime();
						}
						if (idControlInput[i].pIdCommandPack[j].CommandSendAllFunc(idControlInput[i].pList, 
														  idControlInput[i].pApiNet, 
														  idControlInput[i].pIdCommandPack,
														  j,
														  gpCommandPool->listener.sock
														 ) != 0)
						{
							continue;
						}
						
						dbgdumpCommSend();
					}
				}
				if (j > E_COMMAND_TYPE_STAT)
				{
					//Shell或者Monitor
					DEBUGMSG(0,("1 gCommandType:%d\n", gCommandType));
					if (gCommandType == E_COMM_TYPE_SHELL)
					{
						if (CommandShell(i, j, k) != 0)
							continue;
					}
					else if (gCommandType == E_COMM_TYPE_MONITOR)
					{
						if (CommandMonitor(i, j) != 0)
							continue;
					}
				}
			}
			/*if ((++(*idControlInput[i].pCommandIndex)) >= 
				*idControlInput[i].pCommandLen / sizeof(idControlInput[i].pIdCommandPack[0]))
			{
				*idControlInput[i].pCommandIndex = 0;	
			}*/			
			//DEBUGMSG(0,("1 pCommandIndex:%d\r\n", *idControlInput[i].pCommandIndex));
		}
		DEBUGMSG(0,("2 @@@@@@@@@@@\r\n"));
		//MSleep(60000);
		MSleep(1000);
		//usleep(1000);
	}

}

/*void *CommandRecvProcess(void *arg)
{
	//void *data;
	//char * string = NULL;
	//BOOL bDely = TRUE;
	DEBUGMSG(1,("4 %s...\r\n", __FUNCTION__));

	while(1)
	{	
		//if (bDely)
		//{
		//	MSleep(1000);
		//	bDely = FALSE;
		//}
		if (gpCommandPool == NULL)
		{
			MSleep(1000);
			DEBUGMSG(0,("4 %s...1111\r\n", __FUNCTION__));	
			continue;
		}

		DEBUGMSG(0,("4 %s...2222\r\n", __FUNCTION__));
		if (stApiNetWork.ApiNetConnPoolPoll(gpCommandPool) != 0)
		{
			//MSleep(1000);
			//usleep(100);
			continue;	
		}
		DEBUGMSG(0,("4 %s...3333\r\n", __FUNCTION__));

		if (CommandCirFifoQueue.QuePush(gpCommandQue, (void *)gpCommandPool->recv_buf) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, cirqueue_push error!\r\n", __FILE__, __FUNCTION__, __LINE__);
		}
		
		memset(gpCommandPool->recv_buf, 0, sizeof(gpCommandPool->recv_buf));
	}
}*/

void *PollRecvProcess(void *arg)
{
	int event_idx = 0, i = 0;

	DEBUGMSG(1, ("1 %s...\r\n", __FUNCTION__));

	while(1)
	{
		if (stApiNetWork.ApiNetPoolPollWait(gpPoller) != 0)
		{
			//MSleep(1);
			usleep(150);
			continue;	
		}

		for (i = 0; i < dim(gWorkProc); i++)
		{
			//if (gWorkProc.pPool[i].listener.sock != -1 )
			//{
			for ( event_idx = 0; event_idx < gpPoller->events_count; ++event_idx)
			{	//*(argv+i) 
				//DEBUGMSG(0,("sock:%d, fd:%d\r\n", gWorkProc.pPool[i].listener.sock, gpPoller->events[event_idx].data.fd));
				if ((*gWorkProc[i].pPool)->listener.sock == gpPoller->events[event_idx].data.fd)
				{
					if (stApiNetWork.ApiNetConnPoolRecv(*gWorkProc[i].pPool) == 0)
					{
						if (CirFifoQueue.QuePush(*gWorkProc[i].pCirQue, (void *)(*gWorkProc[i].pPool)->recv_buf) != 0)
						{
							GetLocalTimeLog();
							ap_log_debug_log("\t\t%s, %s, %d, cirqueue_push error!\r\n", __FILE__, __FUNCTION__, __LINE__);
						}
					}
					memset((*gWorkProc[i].pPool)->recv_buf, 0, sizeof((*gWorkProc[i].pPool)->recv_buf));
				}
				/*else
				{
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, fd:[%d] error!\r\n", 
						__FILE__, __FUNCTION__, __LINE__, (*gWorkProc[i].pPool)->listener.sock);
				}*/
			}
			//}
		}
	}
}

void *CommandParseProcess(void *arg)
{
	int i = 0, rtnPar = 0;
	void *data;
	char *pStr = NULL;						//逗号替换成空格用
	char * string = NULL;					//空格替换成'\0'用	
	int argc = 0;
	char *argv[BUFFER_LEN_256] = {0};
	char delimA[] = " ";
	int delimB = ',';
	int rtn = 0;
	int iIndex = 0;
	char dest[BUFFER_LEN_4096] = {0};
	int destLen = sizeof(dest), iPos = 0;

	DEBUGMSG(1,("4 %s...\r\n", __FUNCTION__));

	while(1)
	{
		if (gpCommandPool == NULL)
		{
			MSleep(1000);
			continue;
		}

		if (gbCommRecvOK)
		{
			dbgdumpCommRecv();
		}
				
		if ((data = CirFifoQueue.QuePop(gpCommandQue)) == NULL)
		{
			DEBUGMSG(0,("1 >>>\r\n"));
			MSleep(500);
			//CommandCirFifoQueue.QueFree(data);
			continue;
		}
			
		pStr = string = (char *)data;
		DEBUGMSG(0,("1 recv:%s, pStr@:%p\r\n", string, pStr));

		if (Parse(pStr,&argc,argv, delimA) != 0)
		{
			continue;
		}

		if (argc < COMMANDLINE_3)
			continue;
		
		//for (i = 0; i < argc; i++)
		//	DEBUGMSG(0,("###111 [%d] %s\r\n", i, argv[i]));

		rtnPar = ParseJsonCommand(argv[0], (void *)&CommandSrc, &iIndex, dest, destLen);

		if (rtnPar != 0)
		{
			//GetLocalTimeLog();
			//ap_log_debug_log("\t\t%s, %s, %d, \"Parse JsonLogin\" is error!rtnPar:%d\r\n",  
			//	__FILE__, __FUNCTION__, __LINE__, rtnPar);		
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

		DEBUGMSG(0,("3 dest:%s\n", dest));

		//continue;

		for (i = 0; i < argc; i++)
		{
			DEBUGMSG(0,("***%p, %s\r\n", *(argv+i), *(argv+i)));
			memset(argv+i, 0, sizeof(*argv));
		}
		argc = iPos = 0;

		while((pStr = strchr(dest, delimB)) != NULL)
		{
			DEBUGMSG(0,("pStr:%s\r\n", pStr));
			strncpy(pStr, delimA, sizeof(char));
		}

		DEBUGMSG(1,("4 dest:%s\n", dest));

		DEBUGMSG(0,("2 recv:%s, pStr@:%p\r\n", string, pStr));
		
		if (Parse(dest, &argc, argv, delimA) != 0)
		{
			memset(data, 0, BUFFER_LEN_256);
			continue;
		}
		
		rtn = ParseRecvCommand(argc, argv, 0);
		if (rtn == E_RECV_COMMAND_ERROR_PARAM_NUM)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, ParseRecvCommand() %s\r\n",  
								__FILE__, __FUNCTION__, __LINE__, gCommParseErr[E_RECV_COMMAND_ERROR_PARAM_NUM - 1].cmd);
		}
				
		for (i = 0; i < argc; i++)
		{
			DEBUGMSG(1,("###222 %p, %s\r\n", *(argv+i), *(argv+i)));
			memset(argv+i, 0, sizeof(*argv));
		}
		argc = 0;
		memset(data, 0, BUFFER_LEN_256);

		DEBUGMSG(0,("3 recv:%s, pStr@:%p\r\n", string, pStr));
	}	
}

void dbgdumpCommRecv(void)
{

	printf("++++++++++++\r\n");
	timeprintf();

	printf("CommandRecv+++Count:%d, Error:%d\r\n", gpCommandPool->Recv.Count, gpCommandPool->Recv.Error);

	gpCommandPool->Recv.Count = gpCommandPool->Recv.Error = 0;
	
	gbCommRecvOK = FALSE;
}

void dbgdumpCommSend(void)
{
	long Interval = 0;                 //秒

	printf("++++++++++++\r\n");
	timeprintf();

	Interval = GetTime()- gCommStartTime;
	printf("CommandSend+++Count:%d, Error:%d, Error1:%d\r\n", gpCommandPool->Send.Count, gpCommandPool->Send.Error, gpCommandPool->Send.Error1);

	printf("%d num took time %ld S\r\n",  gpCommandPool->Send.Count, Interval);

	gpCommandPool->Send.Count = gpCommandPool->Send.Error = gpCommandPool->Send.Error1 = 0;

	MSleep(5000);
	gbCommRecvOK = TRUE;
}

