/***********************************************************************
* $Id$
* Project:	 switch_network  
* File:db_op/db_mysql.c		 
* Description: brief Part of mysql api. 
*
* @version:		V1.0.0
* @date:		19th November 2014
*
* Copyright (C) 2014 SFARDS Co., Ltd. All rights reserved.
*-----------------------------------------------------------------
*
* Revise:  
* 		 date		 version 		author 		description
*  		2014/11/19	 1.0.0		zhengchao	create mysql
*
*
*
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "db_mysql.h"
#include "../ap_log.h"
#include "../api_net/api_net.h"
#include "../list/uthash/my_uthash.h"
#include "../fifo_queue/circular_queue.h"
//#include "../api_comm/api_assign_resource.h"

//int gInitDbCpm = 0;                                         //从databses初始化后，获得的cpm值
E_COMMAND_TYPE geCommLoopType = E_COMMAND_TYPE_STAT;
int gCrrMaxCellId = 0; 
int gCrrMaxPoolId = 0;

pthread_mutex_t gCellBaseLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCellStatusLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gSwitchBaseLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gPoolConfigLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gAlarmLogLock = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t gDelCellIdBaseLock = PTHREAD_MUTEX_INITIALIZER;

extern stType SetPoolPara[3];
extern int ParseSetPoolAlgo(int argc, char **argv, int *pPos, void *vPoolIdTemp);


/*stMysqlQuery MinerSqlQuery[] = 
{
	{"replace", PackMinerBaseInsert},
	{"select", PackSelectAll, NULL},
	{"update", PackMinerBaseUpdate},
	{"alter", PackMinerBaseAlter},
	{"delete", PackMinerDeleteOneRow},
	//{"create database", PackCreateDb},
	//{"create table", PackMinerBaseCreate}
};*/

stMysqlQuery CellSqlQuery[] = 
{
	{"replace", PackCellBaseInsert},
	{"select", PackSelectAll, PackCellBaseSelectCellMax},
	{"update", PackCellBaseUpdate, PackCellBaseUpdatePool, PackCellBaseUpdateRealKHash, PackCellBaseUpdateFreq},
	{"alter", PackCellBaseAlter},
	{"delete", PackCellDeleteOneRow},
	{"create table", PackCellBaseCreate}
	//{"select max()", PackCellBaseCellMax}
	//{"create database", PackCreateDb},
};

/*stMysqlQuery MinerStatusQuery[] =
{
	{"insert", PackMinerStatusInsert},
	{"select", PackMinerStatusSelectSingle},
	{"update", PackMinerStatusUpdate},
	{"alter", PackMinerStatusAlter},
	{"deleteall", PackDeleteAll}
	//{"create database", PackCreateDb},
	//{"create table", PackMinerStatusCreate}
};*/

stMysqlQuery CellStatusQuery[] =
{
	{"insert", PackCellStatusInsert},
	{"select", PackCellStatusSelectSingle},
	{"update", PackCellStatusUpdate},
	{"alter", PackCellStatusAlter},
	{"deleteall", PackDeleteAll},
	{"create table", PackCellStatusCreate}
	//{"create database", PackCreateDb},
};

stMysqlQuery SwitchBaseQuery[] =
{
	{"insert", PackSwitchBaseInsert},
	{"select", PackSelectAll, NULL},
	{"update", PackSwitchBaseUpdate},
	{"alter", PackSwitchBaseAlter},
	{"deleteall", PackDeleteAll},
	{"create table", PackSwitchBaseCreate}
	//{"create database", PackCreateDb},
};

stMysqlQuery PoolConfigQuery[] =
{
	{"insert", PackPoolConfigInsert},
	{"select", PackSelectAll, PackPoolConfigSelectPoolMax},
	{"update", PackPoolConfigUpdate},
	{"alter", PackPoolConfigAlter},
	{"delete", PackPoolDeleteOneRow, PackDeleteAll},
	{"create table", PackPoolConfigCreate}
	//{"create database", PackCreateDb},
};

stMysqlQuery AlarmLogQuery[] =
{
	{"insert", PackAlarmLogInsert},
	{"select", PackAlarmLogSelectSingle},
	{"update", PackAlarmLogUpdate},
	{"alter", PackAlarmLogAlter},
	{"deleteall", PackDeleteAll},
	{"create table", PackAlarmLogCreate}
	//{"create database", PackCreateDb},
};

stMysqlQuery DelCellIdBaseQuery[] = 
{
	{"replace", PackDelCellIdBaseInsert},
	{"select", PackSelectAll, NULL},
	{"update", PackDelCellIdBaseUpdate},
	{"alter", PackDelCellIdBaseAlter},
	{"delete", PackDelCellIdDeleteOneRow, PackDeleteAll},
	{"create table", PackDelCellIdBaseCreate}
};

stConnectDataBase gConnIniDb[] =
{
	{&myPoolConfig, &bIniPoolConfig, &gCfgDataDL.PoolInfo, &gCfgDataDL.PoolConfigColName, PoolConfigQuery, InitDbToDefaultPool, NULL, &gPoolConfigLock},//InitDbToMaxPoolId
	{&mySwitchBase, &bIniSwitchBase, &gCfgDataDL.SwitchBaseInfo, &gCfgDataDL.SwitchBaseColName, SwitchBaseQuery, InitDbToSwitch, NULL, &gSwitchBaseLock},
	//{&myCellBase, &bIniCellBase, &gCfgDataDL.CellBaseInfo, &gCfgDataDL.CellColName, CellSqlQuery, InitDbToMaxCellId},	
	//{&myDelCellIdBase, &bIniDelCellIdBase, &gCfgDataDL.DelCellIdBaseInfo, &gCfgDataDL.DelCellIdBaColName, DelCellIdBaseQuery, InitDbToDelCellIdBase, NULL},
	//{&myMinerBase, &bIniMinerBase, &gCfgDataDL.MinerBaseInfo, &gCfgDataDL.MinerColName, MinerSqlQuery, InitDbToMiner, NULL},
	{&myCellBase, &bIniCellBase, &gCfgDataDL.CellBaseInfo, &gCfgDataDL.CellColName, CellSqlQuery, InitDbToCell, InitDbToMaxCellId, &gCellBaseLock},
	//{&myMinerStatus, &bIniMinerStatus, &gCfgDataDL.MinerStatusInfo, &gCfgDataDL.MinerStaColName, MinerStatusQuery, NotInitDb, NULL},
	{&myCellStatus, &bIniCellStatus, &gCfgDataDL.CellStatusInfo, &gCfgDataDL.CellStaColName, CellStatusQuery, NotInitDb, NULL, &gCellStatusLock},
	{&myAlarmLog, &bIniAlarmLog, &gCfgDataDL.AlarmInfo, &gCfgDataDL.AlarmLogColName, AlarmLogQuery, NotInitDb, NULL, &gAlarmLogLock}
};

static int UserEqual(PTR pa, PTR pb)
{
  return 0 == strcmp(pa, pb);
}

int InitDbAndDev()
{
	int rtn = 0;	
	char bufDest[BUFFER_LEN_1024] = {0};
	//char paraDest[BUFFER_LEN_32] = {0};
	int i = 0;
	int mySqlError = 0, iQueryRtn = 0;
	int rtnPack = 0;

	//初始化PoolConfige结点
	stBTCPoolConfList.List = stBTCPoolConfList.ListNew();
	stBTCPoolConfList.List->match = UserEqual;
	stLTCPoolConfList.List = stLTCPoolConfList.ListNew();
	stLTCPoolConfList.List->match = UserEqual;

	for (i = 0; i < dim(gConnIniDb); i++)
	{
		DEBUGMSG(0,("i:%d, myDbName:%s, myTableName:%s\r\n", i, gConnIniDb[i].pConnInfo->myDbName, 
			gConnIniDb[i].pConnInfo->myTableName));
		
		//if (i > 1)		//调试用
		//	continue;	
		
		mySqlError = MysqlFunc.dbConnect(gConnIniDb[i].pMySql, gConnIniDb[i].pConnInfo);

		//if (i == 4)
		//{
		memset(bufDest, 0, sizeof(bufDest));
		rtnPack = gConnIniDb[i].pQuery[E_QUERY_TYPE_CREATETABLE - 1].QueryPackFunc1(NULL, gConnIniDb[i].pConnInfo, gConnIniDb[i].pColName, bufDest, sizeof(bufDest));
		if (rtnPack > sizeof(bufDest))
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, More than 1024!\r\n", __FILE__, __FUNCTION__, __LINE__);		
			continue;
		}
		iQueryRtn = MysqlFunc.dbDoQuery(gConnIniDb[i].pMySql, bufDest, E_QUERY_TYPE_CREATETABLE, NULL);

		DEBUGMSG(0,("<<<<<<<<<<<<<<<rtnPack:%d\n", rtnPack));
		DEBUGMSG(0,("bufDest:%s\n", bufDest));
		//}
#if 1
		switch(i)
		{
		case 0:
		case 1:	
		case 2:
		//case 3:
		//case 4:	
			//DEBUGMSG(0,("i:%d, bySqlType2:%s\r\n", i, (char *)gConnIniDb[i].pQuery[E_QUERY_TYPE_SELECT - 1].bySqlType2));
			//strcpy(paraDest, (char *)gConnIniDb[i].pQuery[E_QUERY_TYPE_SELECT - 1].bySqlType1) ;
			gConnIniDb[i].pQuery[E_QUERY_TYPE_SELECT - 1].QueryPackFunc1(NULL, gConnIniDb[i].pConnInfo, gConnIniDb[i].pColName, bufDest, sizeof(bufDest));
			MysqlFunc.dbDoQuery(gConnIniDb[i].pMySql, bufDest, E_QUERY_TYPE_SELECT, gConnIniDb[i].InitDbToDevFunc1);			

			//if (strcmp((char *)gConnIniDb[i].pQuery[E_QUERY_TYPE_SELECT - 1].bySqlType2, "select max") == 0)
			//{
			//	strcpy(paraDest, (char *)gConnIniDb[i].pQuery[E_QUERY_TYPE_SELECT - 1].bySqlType2);

			if (gConnIniDb[i].pQuery[E_QUERY_TYPE_SELECT - 1].QueryPackFunc2 != NULL)
			{
				gConnIniDb[i].pQuery[E_QUERY_TYPE_SELECT - 1].QueryPackFunc2(NULL, gConnIniDb[i].pConnInfo, gConnIniDb[i].pColName, bufDest, sizeof(bufDest));
				if (gConnIniDb[i].InitDbToDevFunc2 != NULL)
				{				
					MysqlFunc.dbDoQuery(gConnIniDb[i].pMySql, bufDest, E_QUERY_TYPE_SELECT, gConnIniDb[i].InitDbToDevFunc2);
				}
			}
			break;

		//case 1:
		//	gConnIniDb[i].pQuery[E_QUERY_TYPE_SELECT_MAX - 1].QueryPackFunc1(NULL, gConnIniDb[i].pConnInfo, gConnIniDb[i].pColName, bufDest, sizeof(bufDest));
		//	MysqlFunc.dbDoQuery(gConnIniDb[i].pMySql, bufDest, E_QUERY_TYPE_SELECT);
		//	break;

		default:
			break;
		}
#endif
		//DEBUGMSG(0,("here-----------------\r\n"));
		//数据库连接成功，表也创建成功。
		if (mySqlError == 0 && iQueryRtn == 0)
		{
			*gConnIniDb[i].pbInitOK = TRUE;

			//如果表不存在则创建
#if 0//_DB_DEL_ENABLE_	
			gConnIniDb[i].pQuery[E_QUERY_TYPE_DELETE - 1].QueryPackFunc1(NULL, gConnIniDb[i].pConnInfo, NULL, bufDest, sizeof(bufDest));
			MysqlFunc.dbDoQuery(gConnIniDb[i].pMySql, bufDest, E_QUERY_TYPE_DELETE);
#endif		
		}
	}


	//初始化池配置
	//PoolToFixediBuffer();

	return rtn;
}

int CalculateRatedKHash(stCellConn *pCellConn)
{
	//int rtn = 0;
	pCellConn->CellData.lBTCRatedKHash = 
		(pCellConn->CellData.iChipBTCHash * pCellConn->CellData.iChipCnt) / VALUES_1000;

	pCellConn->CellData.lLTCRatedKHash = 
		(pCellConn->CellData.iChipLTCHash * pCellConn->CellData.iChipCnt) / VALUES_1000;	

	return 0;
}

#if 0
int ParseAlgoToPoolBuf(int argc, char **argv, void *vPoolIdTemp, void *poolBuf)
{
	int rtn = 0, index = 0, i = 0, j = 0;
	E_ALGO eAlgo;
	int iPoolIdTemp = 0;
	structList *pList = NULL;
	list_node_t *NodeTemp = NULL;
	//int iPos = 0;

	DEBUGMSG(0,("%s..., argc:%d\r\n", __FUNCTION__, argc));

	//iPos = *pPos;

	//btc 或者ltc后面的pool_id 最多3个，最少1个
	if (argc > 4 && argc < 2)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, too many algorithm parameters!(>1 && <4)!\r\n", __FILE__, __FUNCTION__, __LINE__);
		return 1;
	}

	index = ParseAlgo(argc, argv, 0, &eAlgo);
	if (index < 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, algorithm does not exist!(algo:BTC/LTC)!\r\n", __FILE__, __FUNCTION__, __LINE__);
		return 2;
	}

	switch(eAlgo)
	{
	case E_ALGO_BTC:
		pList = &stBTCPoolConfList;
		for (i = 1; i < argc;i++,j++)
		{
			//查找pool_id	
			iPoolIdTemp = strtoul(argv[i], 0, 0);
			NodeTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
			if (NodeTemp != NULL)
			{
				((stPoolFixedConf*)poolBuf)->stBTCPoolConf[j].iPoolId = iPoolIdTemp;

				DEBUGMSG(0,("111 stBTCPoolConf@%p, iPoolId[%d]:%d\r\n", ((stPoolFixedConf*)poolBuf)->stBTCPoolConf, j, ((stPoolFixedConf*)poolBuf)->stBTCPoolConf[j].iPoolId));
				strcpy((char *)((stPoolFixedConf*)poolBuf)->stBTCPoolConf[j].szUrl, (char *)((stPoolConfInfo *)NodeTemp->val)->szUrl);
				strcpy((char *)((stPoolFixedConf*)poolBuf)->stBTCPoolConf[j].szUser, (char *)((stPoolConfInfo *)NodeTemp->val)->szUser);
				strcpy((char *)((stPoolFixedConf*)poolBuf)->stBTCPoolConf[j].szPasswd, (char *)((stPoolConfInfo *)NodeTemp->val)->szPasswd);
				((stPoolFixedConf*)poolBuf)->stBTCPoolConf[j].eAlgo = ((stPoolConfInfo *)NodeTemp->val)->eAlgo;
					
				((stPoolFixedConf*)poolBuf)->iBTCPoolCount++;
			}
			else
			{
				//*(int *)vPoolIdTemp = ((stPoolConfInfo *)NodeTemp->val)->iPoolId;				
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, BTC, this algorithms have not this id(%d)!\r\n", 
					__FILE__, __FUNCTION__, __LINE__, iPoolIdTemp);
				
				return 3;
			}
		}
		break;
		
	case E_ALGO_LTC:
		pList = &stLTCPoolConfList;
		for (i = 1; i < argc;i++,j++)
		{
			//查找pool_id	
			iPoolIdTemp = strtoul(argv[i], 0, 0);
			NodeTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
			if (NodeTemp != NULL)
			{		
				((stPoolFixedConf*)poolBuf)->stLTCPoolConf[j].iPoolId = iPoolIdTemp;
				DEBUGMSG(0,("111 stLTCPoolConf@%p, iPoolId[%d]:%d\r\n",((stPoolFixedConf*)poolBuf)->stLTCPoolConf, j, ((stPoolFixedConf*)poolBuf)->stLTCPoolConf[j].iPoolId));
				strcpy((char *)((stPoolFixedConf*)poolBuf)->stLTCPoolConf[j].szUrl, (char *)((stPoolConfInfo *)NodeTemp->val)->szUrl);
				strcpy((char *)((stPoolFixedConf*)poolBuf)->stLTCPoolConf[j].szUser, (char *)((stPoolConfInfo *)NodeTemp->val)->szUser);
				strcpy((char *)((stPoolFixedConf*)poolBuf)->stLTCPoolConf[j].szPasswd, (char *)((stPoolConfInfo *)NodeTemp->val)->szPasswd);
				((stPoolFixedConf*)poolBuf)->stLTCPoolConf[j].eAlgo = ((stPoolConfInfo *)NodeTemp->val)->eAlgo;

				((stPoolFixedConf*)poolBuf)->iLTCPoolCount++;
			}
			else
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, BTC, this algorithms have not this id(%d)!\r\n", 
					__FILE__, __FUNCTION__, __LINE__, iPoolIdTemp);

				return 3;
			}
		}
		break;
		
	case E_ALGO_DUAL:
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, BTC, nothing has set up a temporary dual!!\r\n", __FILE__, __FUNCTION__, __LINE__);
		rtn = 4;
		break;

	default:
		break;
	}

	//*pPos = iPos;
		
	return rtn;
}
#endif

int ParseAlgoToPoolBuf(int argc, char **argv, void *vPoolIdTemp, void *poolBuf)
{
	int rtn = 0, index = 0, i = 0;//, j = 0;
	E_ALGO eAlgo;
	//int iPoolIdTemp = 0;
	//structList *pList = NULL;
	//list_node_t *NodeTemp = NULL;
	//int iPos = 0;
	int argcSec = 0;
	char *argvSec[BUFFER_LEN_64] = {0};
	char delim[] = "&";
	int iBTCCount = 0, iLTCCount = 0;
	int iIndex = 0;

	DEBUGMSG(0,("%s..., argc:%d\r\n", __FUNCTION__, argc));

	//iPos = *pPos;

	//btc 或者ltc后面的pool_id 最多3个，最少1个
	if (argc > COMMANDLINE_4 && argc < COMMANDLINE_2)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, too many algorithm parameters!(>1 && <4)!\r\n", __FILE__, __FUNCTION__, __LINE__);
		return 1;
	}

	index = ParseAlgo(argc, argv, COMMANDLINE_0, &eAlgo);
	if (index < 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, algorithm does not exist!(algo:BTC/LTC)!\r\n", __FILE__, __FUNCTION__, __LINE__);
		return 2;
	}

	//algo/pool_id&url&worker&pass/pool_id&url&worker&pass...

	for (i = COMMANDLINE_1; i < argc; i++)
	{	
		if (Parse(argv[i], &argcSec, argvSec, delim) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, PoolIdPri,Secondary parse error!\r\n", __FILE__, __FUNCTION__, __LINE__);
			return -1;
		}

		if (argcSec != COMMANDLINE_4)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, parameters error!( != 4)!\r\n", __FILE__, __FUNCTION__, __LINE__);
			return 1;
		}


		switch(eAlgo)
		{
		case E_ALGO_BTC:
			iBTCCount = ((stPoolFixedConf*)poolBuf)->iBTCPoolCount;
			iIndex = 0;
			((stPoolFixedConf*)poolBuf)->stBTCPoolConf[iBTCCount].eAlgo = eAlgo;
			((stPoolFixedConf*)poolBuf)->stBTCPoolConf[iBTCCount].iPoolId = strtoul(argvSec[iIndex++], 0, 0);
			strcpy((char *)((stPoolFixedConf*)poolBuf)->stBTCPoolConf[iBTCCount].szUrl, argvSec[iIndex++]);
			strcpy((char *)((stPoolFixedConf*)poolBuf)->stBTCPoolConf[iBTCCount].szUser, argvSec[iIndex++]);
			strcpy((char *)((stPoolFixedConf*)poolBuf)->stBTCPoolConf[iBTCCount].szPasswd, argvSec[iIndex++]);
			((stPoolFixedConf*)poolBuf)->iBTCPoolCount++;
			if (((stPoolFixedConf*)poolBuf)->iBTCPoolCount > MAX_POOL_COUNT)
			{
				((stPoolFixedConf*)poolBuf)->iBTCPoolCount = MAX_POOL_COUNT;
			}
			break;
			
		case E_ALGO_LTC:
			iLTCCount = ((stPoolFixedConf*)poolBuf)->iLTCPoolCount;
			iIndex = 0;
			((stPoolFixedConf*)poolBuf)->stLTCPoolConf[iLTCCount].eAlgo = eAlgo;
			((stPoolFixedConf*)poolBuf)->stLTCPoolConf[iLTCCount].iPoolId = strtoul(argvSec[iIndex++], 0, 0);
			strcpy((char *)((stPoolFixedConf*)poolBuf)->stLTCPoolConf[iLTCCount].szUrl, argvSec[iIndex++]);
			strcpy((char *)((stPoolFixedConf*)poolBuf)->stLTCPoolConf[iLTCCount].szUser, argvSec[iIndex++]);
			strcpy((char *)((stPoolFixedConf*)poolBuf)->stLTCPoolConf[iLTCCount].szPasswd, argvSec[iIndex++]);
			((stPoolFixedConf*)poolBuf)->iLTCPoolCount++;
			if (((stPoolFixedConf*)poolBuf)->iLTCPoolCount > MAX_POOL_COUNT)
			{
				((stPoolFixedConf*)poolBuf)->iLTCPoolCount = MAX_POOL_COUNT;
			}
			break;
			
		case E_ALGO_DUAL:
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, BTC/LTC, nothing has set up a temporary dual!!\r\n", __FILE__, __FUNCTION__, __LINE__);
			rtn = 4;
			break;

		default:
			break;
		}

	}
	//*pPos = iPos;
		
	return rtn;
}

int ParseToPoolBuf(int argc, char **argv, void *vIdTemp, void *poolBuf)
{
	int rtn = 0, i = 0, j = 0, k = 0;
	int argcSec = 0;
	char *argvSec[BUFFER_LEN_64] = {0};
	char delim[] = "#";
	int iAlgLen = 0, iParamLen = 0;
	//int iPos = 0;

	DEBUGMSG(0,("%s..., argc:%d\r\n", __FUNCTION__, argc));

	for (i = 0; i < argc; i++)
	{
		DEBUGMSG(0,("argv[%d]:%s\r\n", i, argv[i]));
		if (Parse(argv[i], &argcSec, argvSec, delim) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, PoolIdPri,Secondary parse error!\r\n", __FILE__, __FUNCTION__, __LINE__);
			return -1;
		}
		if (argcSec < 2 || argcSec >4)
		{


		
			//GetLocalTimeLog();
			//ap_log_debug_log("\t\t%s, %s, %d, PoolIdPri,parameter error or not set cell or pool,num=%d!(>=2&&<=4)\r\n", 
			//	__FILE__, __FUNCTION__, __LINE__, argcSec);





			return -1;
		}

		DEBUGMSG(0,("argcSec:%d, argvSec[0]:%s\n", argcSec, argvSec[0]));
		iAlgLen = strlen((char *)SetPoolPara[j].type);
		iParamLen = strlen(argvSec[0]);
		if (strncasecmp((char *)SetPoolPara[j++].type, argvSec[0], iAlgLen) == 0 && iAlgLen == iParamLen)
		{
		
			rtn = ParseAlgoToPoolBuf(argcSec, argvSec, vIdTemp, poolBuf);
			if (rtn != 0)
			{
				return rtn;
			}
			for (k = 0; k < argcSec; k++)
			{
				memset(argvSec+k, 0, sizeof(*argvSec));
			}
			argcSec = 0;
		}
	}

	return rtn;
}

int InitCalculateBTCRealKHash(char *pBufBTCHash, stCellConn *pCellConn)
{
	int iBTCIndex = 0;
	//int iIndex = 0;
	int i = 0;
	int argcA = 0;
	char *argvA[BUFFER_LEN_64] = {0};
	char delimA[] = "|";	

	DEBUGMSG(0,("%s...\n", __FUNCTION__));

	//解析出30Min内的BTCNcsend
	/*for (i = 0; i < argc; i++)
	{
		memset(argvA+i, 0, sizeof(*argvA));
	}*/
	//argcA = 0;
	if (Parse(pBufBTCHash, &argcA, argvA, delimA) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, cell_base, BTCNcsend parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		//memset(bufPool, 0, sizeof(bufPool));
		return 1;
	}
	else
	{
		
		if (argcA <= 1)
			return 2;
	
		for (i = 0; i < argcA; i++)
		{
			if (i == 0)
				pCellConn->CellStat.iBTCNcIndex = strtoll(argvA[i], 0, 0);
			else
				pCellConn->CellStat.llBTCNcsend[i] = strtoll(argvA[i], 0, 0); 		
		}
	}

	//iIndex = iStartPos;
	//实际算力计算方法，单位：khash/s，iBTCRealKHash=(|lBTCNcsend[Crr] - lBTCNcsend[30M before]| * BTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000)

	if (pCellConn->CellStat.iBTCNcIndex == 0 && pCellConn->CellStat.llBTCNcsend[INTERVAL_30_SECOND-1] != 0)
		iBTCIndex = INTERVAL_30_SECOND - 1;
	else
		iBTCIndex = pCellConn->CellStat.iBTCNcIndex - 1;
	if (iBTCIndex < 0)
		iBTCIndex = 0;

	if (iBTCIndex < INTERVAL_30_SECOND)
	{
		//iIndex = 8
		//pCellConn->CellStat.llBTCNcsend[iBTCIndex] = strtoll(argv[iIndex], 0, 0);
		
		if (pCellConn->CellStat.llBTCNcsend[iBTCIndex] != 0 && iBTCIndex != INTERVAL_30_SECOND - 1)
		{	
			pCellConn->CellStat.llBTCRealKHash = 
				(abs(pCellConn->CellStat.llBTCNcsend[iBTCIndex] - pCellConn->CellStat.llBTCNcsend[iBTCIndex+1]) * BTC_SHARE) 
				/ (INTERVAL_30_SECOND * VALUES_1000);
		}
		else
		{
			pCellConn->CellStat.llBTCRealKHash =
			(abs(pCellConn->CellStat.llBTCNcsend[iBTCIndex] - pCellConn->CellStat.llBTCNcsend[0]) * BTC_SHARE) 
			/ (INTERVAL_30_SECOND * VALUES_1000);
		}
		//pCellConn->CellStat.iBTCNcIndex++;
	}
	else
	{
		//iIndex = 8
		//pCellConn->CellStat.llBTCNcsend[0] = strtoll(argv[iIndex], 0, 0);
		pCellConn->CellStat.llBTCRealKHash =
			(abs(pCellConn->CellStat.llBTCNcsend[INTERVAL_30_SECOND - 1] - pCellConn->CellStat.llBTCNcsend[0]) * BTC_SHARE) 
			/ (INTERVAL_30_SECOND * VALUES_1000);
	
		//pCellConn->CellStat.iBTCNcIndex = 0;
	}
	
	return 0;
}

int InitCalculateLTCRealKHash(char *pBufLTCHash, stCellConn *pCellConn)
{
	int iLTCIndex = 0;
	//int iIndex = 0;
	int i = 0;
	int argcA = 0;
	char *argvA[BUFFER_LEN_64] = {0};
	char delimA[] = "|";	

	DEBUGMSG(0,("%s...\n", __FUNCTION__));


	//解析出30Min内的LTCNcsend
	if (Parse(pBufLTCHash, &argcA, argvA, delimA) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, cell_base, LTCNcsend parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		//memset(bufPool, 0, sizeof(bufPool));
		return 1;
	}
	else
	{
		if (argcA <= 1)
			return 2;
		
		for (i = 0; i < argcA; i++)
		{
			if (i == 0)
				pCellConn->CellStat.iLTCNcIndex = strtoll(argvA[i], 0, 0);
			else
				pCellConn->CellStat.llLTCNcsend[i] = strtoll(argvA[i], 0, 0); 		
		}
	}

	//iIndex = iStartPos;
	//实际算力计算方法，单位：khash/s，iLTCRealKHash=(|lLTCNcsend[Crr] - lLTCNcsend[30M before]| * LTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000)

	if (pCellConn->CellStat.iLTCNcIndex == 0 && pCellConn->CellStat.llLTCNcsend[INTERVAL_30_SECOND-1] != 0)
		iLTCIndex = INTERVAL_30_SECOND - 1;
	else
		iLTCIndex = pCellConn->CellStat.iLTCNcIndex - 1;
	if (iLTCIndex < 0)
		iLTCIndex = 0;

	if (iLTCIndex < INTERVAL_30_SECOND)
	{
		//iIndex = 8
		//pCellConn->CellStat.llLTCNcsend[iLTCIndex] = strtoll(argv[iIndex], 0, 0);
		
		if (pCellConn->CellStat.llLTCNcsend[iLTCIndex] != 0 && iLTCIndex != INTERVAL_30_SECOND - 1)
		{	
			pCellConn->CellStat.llLTCRealKHash = 
				(abs(pCellConn->CellStat.llLTCNcsend[iLTCIndex] - pCellConn->CellStat.llLTCNcsend[iLTCIndex+1]) * LTC_SHARE) 
				/ (INTERVAL_30_SECOND * VALUES_1000);
		}
		else
		{
			pCellConn->CellStat.llLTCRealKHash =
			(abs(pCellConn->CellStat.llLTCNcsend[iLTCIndex] - pCellConn->CellStat.llLTCNcsend[0]) * LTC_SHARE) 
			/ (INTERVAL_30_SECOND * VALUES_1000);
		}
		//pCellConn->CellStat.iLTCNcIndex++;
	}
	else
	{
		//iIndex = 8
		//pCellConn->CellStat.llLTCNcsend[0] = strtoll(argv[iIndex], 0, 0);
		pCellConn->CellStat.llLTCRealKHash =
			(abs(pCellConn->CellStat.llLTCNcsend[INTERVAL_30_SECOND - 1] - pCellConn->CellStat.llLTCNcsend[0]) * LTC_SHARE) 
			/ (INTERVAL_30_SECOND * VALUES_1000);
	
		//pCellConn->CellStat.iLTCNcIndex = 0;
	}
	
	return 0;
}

int InitDbToSwitch(int argc, char**argv, int iStartPos)
{
	//int rtn = 0;
	//int argcA = 0;
	//char *argvA[BUFFER_LEN_256] = {0};
	//char delimA[] = " ";
	int index = 0;//i = 0, 
	//int iPoolIdtemp[MAX_POOL_COUNT*2] = {0};
	//BYTE szPoolIdPriTemp[BUFFER_LEN_64] = {0};

	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));
	
	if (argc < iStartPos || argc != SWITCH_DB_COMMANDLINE)
		return 1;
	
	index = iStartPos;
	//gpSwitchDbInfo->DbSwi.iSwiId =  strtoul(argv[index++], 0, 0);
	strncpy((char *)gpSwitchDbInfo->DbSwi.szSwiId, argv[index++], sizeof(gpSwitchDbInfo->DbSwi.szSwiId));
	DEBUGMSG(1,("###############szSwiId:%s\r\n", gpSwitchDbInfo->DbSwi.szSwiId));
	gpSwitchDbInfo->DbSwi.iStatsInter =	strtoul(argv[index++], 0, 0);
	strncpy((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri, argv[index], sizeof(gpSwitchDbInfo->DbSwi.szPoolIdPri));
	DEBUGMSG(0,("szPoolIdPri:%s\r\n", (char *)gpSwitchDbInfo->DbSwi.szPoolIdPri));

	//初始化gPoolDefaultConf
	//memset(&gPoolDefaultConf, 0, sizeof(stPoolFixedConf));
	//初始化gPoolSpecifyConf
	memset(&gPoolSpecifyConf, 0, sizeof(stPoolFixedConf));
	//memset(&gPoolDefaultConf.iBTCPoolIdPri, -1, sizeof(int));
	//memset(&gPoolDefaultConf.iLTCPoolIdPri, -1, sizeof(int));


	//解析出poolid_pri
	/*strcpy((char *)szPoolIdPriTemp, (char *)gpSwitchDbInfo->DbSwi.szPoolIdPri);
	if (Parse((char *)szPoolIdPriTemp, &argcA, argvA, delimA) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, poolid_pri, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return 2;
	}
	DEBUGMSG(0,("argcA:%d, argvA[0]:%s\r\n", argcA, argvA[0]));

	if (ParseToPoolBuf(argcA, argvA, NULL, (void *)&gPoolDefaultConf) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, inside the parameters of poolid_pri!\r\n",  __FILE__, __FUNCTION__, __LINE__);
	}*/

	/*if (argcA > MAX_POOL_COUNT*2)
		return 3;
	else
	{
		for (i = 0; i < argcA; i++)
		{
			iPoolIdtemp[i] = strtoul(argvA[index++], 0, 0);
		}
	}*/
	
	/*for (i = 0; i < argcA; i++)
	{
		if (iPoolIdtemp[i] < 0)
			continue;



		//if (iPoolIdtemp[i] / 2 == 0)
		{
			gPoolDefaultConf.iBTCPoolIdPri[i] = iPoolIdtemp[i];
			if (gPoolDefaultConf.iBTCPoolCount <= MAX_POOL_COUNT)
				gPoolDefaultConf.iBTCPoolCount++;
		}	
		else
		{
			gPoolDefaultConf.iLTCPoolIdPri[i] = iPoolIdtemp[i];
			if (gPoolDefaultConf.iLTCPoolCount <= MAX_POOL_COUNT)
				gPoolDefaultConf.iLTCPoolCount++;			
		}
	}*/
	
	/*for (i = 0; i < gPoolDefaultConf.iBTCPoolCount; i++)
	{
		DEBUGMSG(0,("BTCPoolIdPri:[%d]:%d\r\n", i, gPoolDefaultConf.iBTCPoolIdPri[i]));
	}

	for (i = 0; i < gPoolDefaultConf.iLTCPoolCount; i++)
	{
		DEBUGMSG(0,("LTCPoolIdPri:[%d]:%d\r\n", i, gPoolDefaultConf.iLTCPoolIdPri[i]));
	}*/

	//gInitDbCpm = strtoul(argv[2], 0, 0);

	//if (gInitDbCpm != gCfgDataDL.SwitchInfo.iCellNum)
	//	geCommLoopType = E_COMMAND_TYPE_REBOOT;

	return 0;
}

int InitDbToMaxCellId(int argc, char**argv, int iStartPos)
{
	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));

	if (argc < iStartPos || argc != DELCELLID_DB_COMMANDLINE)
		return 1;	

	gCrrMaxCellId = strtoul(argv[0], 0, 0);

	DEBUGMSG(0,("gCrrMaxCellId:%d, argv[0]:%s\r\n", gCrrMaxCellId, argv[0]));

	return 0;
}

//初始化Database的数据到DelCellId节点中 
//此链表是保存删除CellNode时，保留CellId的链表，方便其它设备继续使用这个id。
int InitDbToDelCellIdBase(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int iDelCellIdTemp = 0;
	structList *pList = NULL;
	
	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));
	
	if (argc < iStartPos || argc != DELCELLID_DB_COMMANDLINE)
		return 1;

	pList = &stDelCellIdList;
	iDelCellIdTemp = strtoul(argv[0], 0, 0);

	//查找DelDevId	
	//pthread_mutex_lock(pList->RemoveLock);
	list_node_t *DelCellIdNode = pList->ListFind(pList->List, (char *)&iDelCellIdTemp);
	if (DelCellIdNode != NULL)
	{		
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Get the same DelCellId [%d] from the database!\r\n",	
			__FILE__, __FUNCTION__, __LINE__, iDelCellIdTemp);		
	}
	else
	{
		stDelIdAllocTab *val = malloc(sizeof(stDelIdAllocTab));
		memset(val, 0, sizeof(stDelIdAllocTab));
		val->iDelCellId = iDelCellIdTemp;
		val->eDelCellIdMySql = 0;
		//初始化
		//val->eDelCellIdMySql = 0;

		//无重复DelDevId，插入链表
		list_node_t *node = pList->ListNodeNew(val);
		assert(node->val == val);
		pList->ListRpush(pList->List, node, pList->ListLenLock);
		rtn = 0;
	}
	//pthread_mutex_unlock(pList->RemoveLock);

	return rtn;
}

//初始化Database的数据到Miner节点中 
/*int InitDbToMiner(int argc, char**argv, int iStartPos)
{
	int rtn = -1;
	int index = 0;
	//BYTE byMinerIp[BUFFER_LEN_16] = {0};	//miner的ip
	unsigned long iMinerIp = 0;
	int iMinerPort = 0; 					//miner的port
	int iListLen = 0;
	//BYTE destBuf[BUFFER_LEN_256] = {0};
	int i = 0;
	stMinerConn *val = NULL;
	E_LOGIN_ERROR eLoginErr;
	
	if (argc < iStartPos || argc != MINER_DB_COMMANDLINE)
		return 1;
	
	index = iStartPos;
	DEBUGMSG(0, ("%s..., argc:%d\r\n", __FUNCTION__, argc));
	val = malloc(sizeof(stMinerConn));
	memset(val, 0, sizeof(stMinerConn));

	val->MinerData.iMinerId = strtoul(argv[index++], 0, 0);
	iMinerIp = strtoul(argv[index++], 0, 0);
	iMinerPort = strtoul(argv[index++], 0, 0);
	val->MinerData.eMinerAlgo = strtoul(argv[index++], 0, 0);
	val->MinerData.bMinerOnline = strtoul(argv[index++], 0, 0);
	
	DEBUGMSG(0,("eMinerAlgo:%d, bMinerOnline:%d\r\n", val->MinerData.eMinerAlgo, val->MinerData.bMinerOnline));
	iStartPos = index;
	//分配资源空间
	if ((index = AllocDbToMinerRes(argc, argv, iStartPos, val->MinerData.eMinerAlgo, &val->pMinerRes)) < 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, AllocDbToRes error!\r\n",  __FILE__, __FUNCTION__, __LINE__);	
		return 3;
	}

	//DEBUGMSG(0,("1 iWbsId:%d, iConnCellNum:%d, iWbIndex:%d\r\n", 
	// val->pMinerRes->iWbsId,  val->pMinerRes->iConnCellNum,  val->pMinerRes->iWbIndex));
	//DEBUGMSG(0,("index:%d, @pMinerRes:%p\r\n", index, val->pMinerRes));

	//DEBUGMSG(0,("MinerIp:%s, MinerPort:%d, %d\r\n", (char *)byMinerIp, iMinerPort, val->MinerData.iMinerId));
		

	val->MinerData.iRegtime = strtoul(argv[index++], 0, 0);
	val->MinerData.iLoginTime = strtoul(argv[index++], 0, 0);
	val->MinerData.iLogoutTime = strtoul(argv[index++], 0, 0);
	
	//SetClientIp4(&val->remote.addr4, byMinerIp, iMinerPort);
	val->remote.addr4.sin_family = AF_INET;
	val->remote.addr4.sin_port = iMinerPort;
	val->remote.addr4.sin_addr.s_addr = htonl(iMinerIp);
	val->MinerParent.sock = gpLoginPool->listener.sock;

	//DEBUGMSG(0,("---%s ---%d, sock:%d, eMinerAlgo:%d\r\n", 
	//	inet_ntoa(val->remote.addr4.sin_addr), val->remote.addr4.sin_port, val->MinerParent.sock, val->MinerData.eMinerAlgo));



//#if 1
	//DEBUGMSG(0,("@Miner LoginRemLock:%p\r\n", stMinerUtlist.LoginRemLock));
	//pthread_mutex_lock(stMinerUtlist.LoginRemLock);

	pthread_mutex_lock(stMinerUtlist.InitNodeLock);
	//查找MinerId	
	devlist_ist *MinerNode = stMinerUtlist.FindUserInt(stMinerUtlist.HashUsers, val->MinerData.iMinerId);
	if (MinerNode != NULL)
	{
		free(val);
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Get the same MinerId [%d] from the database!\r\n",	
			__FILE__, __FUNCTION__, __LINE__, ((stMinerConn*)MinerNode->val)->MinerData.iMinerId); 		
	}
	else
	{
		pthread_mutex_lock(stMinerUtlist.ListLenLock);
		iListLen = stMinerUtlist.CountUsersInt(*stMinerUtlist.HashUsers);
		pthread_mutex_unlock(stMinerUtlist.ListLenLock);
		
		DEBUGMSG(0,("^^^^^^^^iListLen:%d, iMinerNum:%d\r\n", iListLen, gLoginResIndex.iMinerNum));
		if (iListLen < gLoginResIndex.iMinerNum)
		{
			DEBUGMSG(0, ("eMinerAlgo:%d, len:%d\r\n", val->MinerData.eMinerAlgo, sizeof(gMinerAssi)/sizeof(gMinerAssi[0])));
			if ((i = val->MinerData.eMinerAlgo) < dim(gMinerAssi))
			{
				DEBUGMSG(0,("i:%d, pCrrAlgoLen:%d, pSumAlgoLen:%d\r\n", i, *gMinerAssi[i].pCrrAlgoLen, *gMinerAssi[i].pSumAlgoLen));
				if (*gMinerAssi[i].pCrrAlgoLen < *gMinerAssi[i].pSumAlgoLen)
				{

					pthread_mutex_lock(gResourceLock.CrrMinerAlgoLenLock);
					eLoginErr = IncrCrrentAlgoLen(i, gMinerAssi);
					pthread_mutex_unlock(gResourceLock.CrrMinerAlgoLenLock);	
					//DEBUGMSG(0,("++++++++++++++++++++++++++++++++++++++++++++\r\n"));
					if (eLoginErr == 0)
					{
						DEBUGMSG(0,("2 iWbsId:%d, iConnCellNum:%d, iWbIndex:%d @:%p\r\n", 
						val->pMinerRes->iWbsId,  val->pMinerRes->iConnCellNum,  val->pMinerRes->iWbIndex, &val->pMinerRes->iWbIndex));

						//是否重新按照新的cpm重新分配
						if (gInitDbCpm != gpResource->iConnCellNum && gInitDbCpm != 0)
						{
							gMinerAssi[i].ResToAlgoFunc(val, gpResource, gLoginResIndex.iMinerNum);
							val->eMinerComm = E_COMMAND_TYPE_REBOOT;
						}	
						else
						{
							gMinerAssi[i].DbResToAlgoFunc(val, gpResource, gLoginResIndex.iMinerNum); 
							val->eMinerComm = E_COMMAND_TYPE_STAT;
						}

						DEBUGMSG(0,("*********eMinerComm:%d\r\n", val->eMinerComm));
						
						//初始化
						pthread_mutex_init(&val->MinerDataLock.OnlineLock, NULL);
						pthread_mutex_init(&val->MinerDataLock.AlarmTimerLock, NULL);
						pthread_mutex_init(&val->MinerDataLock.IpPortLock, NULL);
						val->eMinerMySql  = 0;

						if (val->MinerData.bMinerOnline)
							val->MinerData.iMinerAlarmTimer = GetTime();
						else
							val->MinerData.iMinerAlarmTimer = 0;

						//无重复MinerId，插入链表
						//DEBUGMSG(0,("1 @Miner HashUsers:%p\r\n", stMinerUtlist.HashUsers));
						stMinerUtlist.AddUserInt(stMinerUtlist.HashUsers, val->MinerData.iMinerId, (void**)&val);
						rtn = 0;

						DEBUGMSG(0,("2 @Miner HashUsers:%p\r\n", stMinerUtlist.HashUsers));
					}
					else
					{
						GetLocalTimeLog();
						ap_log_debug_log("\t\t%s, %s, %d, MinerId [%d] BL has exceeded the maximum length set!\r\n",	__FILE__, __FUNCTION__, __LINE__, val->MinerData.iMinerId); 		
						//PackMinerError(val, (char *)destBuf, &iLen, sizeof(destBuf), eLoginErr - 1);
						rtn = eLoginErr;
					}
				}
				else
				{
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, MinerId [%d] algo has exceeded the maximum length set!\r\n",  __FILE__, __FUNCTION__, __LINE__, val->MinerData.iMinerId);			
					//PackMinerError(val, (char *)destBuf, &iLen, sizeof(destBuf), E_MINER_LOGIN_ERROR_OVERALGO - 1);
				}
			}

		}
		else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, MinerId [%d] has exceeded the maximum length set!\r\n",  __FILE__, __FUNCTION__, __LINE__, val->MinerData.iMinerId);			
			//PackMinerError(val, (char *)destBuf, &iLen, sizeof(destBuf), E_MINER_LOGIN_ERROR_OVERNUM - 1);
		}
	}
	pthread_mutex_unlock(stMinerUtlist.InitNodeLock);

	return rtn;
}*/

//初始化Database的数据到CellIdBase节点中 
int InitDbToCellIdBase(int argc, char**argv, int iStartPos)
{
	int rtn = -1;
	int index = 0;
	//long lDevIdTemp = 0;
	BYTE szDevIdTemp[BUFFER_LEN_28] = {0};
	int iCellIdTemp = 0;
	//BYTE destBuf[BUFFER_LEN_256] = {0};
	
	iCellIdTemp = strtoul(argv[index++], 0, 0);
	strncpy((char *)szDevIdTemp, argv[index++], sizeof(szDevIdTemp));

	//查找DevId	
	devlist_st *CellIdNode = stCellIdUtlist.FindUserStr(stCellIdUtlist.StrHUsers, (char *)szDevIdTemp);
	if (CellIdNode != NULL)
	{		
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Get the same DevId [%s], CellId [%d] from the database!\r\n",	
			__FILE__, __FUNCTION__, __LINE__, (char *)((stIdAllocTab*)CellIdNode->val)->szDevId, ((stIdAllocTab*)CellIdNode->val)->iId);		
	}
	else
	{
		stIdAllocTab *val = malloc(sizeof(stIdAllocTab));
		memset(val, 0, sizeof(stIdAllocTab));	
		strncpy((char *)val->szDevId, (char *)szDevIdTemp, sizeof(val->szDevId));
		val->iId = iCellIdTemp;

		//无重复CellId，插入链表
		stCellIdUtlist.AddUserStr(stCellIdUtlist.StrHUsers, (char *)szDevIdTemp, sizeof(szDevIdTemp), (void**)&val);
		rtn = 0;
	}

	return rtn;
}

/*int ParseCellPoolList(int argc, char**argv, stCellConn *pval)
{
	int rtn = 0,j = 0;
	int argcSec = 0;
	char *argvSec[BUFFER_LEN_64] = {0};
	char delimSec[] = "/";

	if (argc > 9 && argc < 2)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, too many parameters!(>1 && <9)!\r\n", __FILE__, __FUNCTION__, __LINE__);
	}
	else
	{
		DEBUGMSG(0,("argv:%s\r\n", argv));
		
		if (Parse(argv, &argcSec, argvSec, delimSec) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, PoolBuf,Secondary parse error!\r\n", __FILE__, __FUNCTION__, __LINE__);
			return -1;
		}		
			
		if (argc != COMMANDLINE_6)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, too many algorithm parameters!(>1 && <4)!\r\n", __FILE__, __FUNCTION__, __LINE__);
			return 1;
		}

	
		//结点中的pool信息修改完成后，在进行打包发送结点信息
		pthread_mutex_lock(&val->CellDataLock.PoolConfLock);
		val->CellPoolConf.iBTCPoolCount = 0;
		for(j = 0;j < gPoolDefaultConf.iBTCPoolCount && gPoolDefaultConf.iBTCPoolCount <= MAX_POOL_COUNT;j++)
		{	
			val->CellPoolConf.stBTCPoolConf[j].iPoolId = gPoolDefaultConf.stBTCPoolConf[j].iPoolId;
			DEBUGMSG(0,("BTC iPoolId[%d]:%d\r\n", j, val->CellPoolConf.stBTCPoolConf[j].iPoolId));
			strcpy((char *)((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].szUrl, (char *)gPoolDefaultConf.stBTCPoolConf[j].szUrl);
			strcpy((char *)((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].szUser, (char *)gPoolDefaultConf.stBTCPoolConf[j].szUser);
			strcpy((char *)((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].szPasswd, (char *)gPoolDefaultConf.stBTCPoolConf[j].szPasswd);
			((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].eAlgo = gPoolDefaultConf.stBTCPoolConf[j].eAlgo;
			((stCellConn *)pNode->val)->CellPoolConf.iBTCPoolCount++;
		}
		
		((stCellConn *)pNode->val)->CellPoolConf.iLTCPoolCount = 0; 		
		for(j = 0;j < gPoolDefaultConf.iLTCPoolCount && gPoolDefaultConf.iLTCPoolCount <= MAX_POOL_COUNT;j++)
		{	
			((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].iPoolId = gPoolDefaultConf.stLTCPoolConf[j].iPoolId;
			DEBUGMSG(0,("LTC iPoolId[%d]:%d\r\n", j, ((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].iPoolId));
			strcpy((char *)((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].szUrl, (char *)gPoolDefaultConf.stLTCPoolConf[j].szUrl);
			strcpy((char *)((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].szUser, (char *)gPoolDefaultConf.stLTCPoolConf[j].szUser);
			strcpy((char *)((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].szPasswd, (char *)gPoolDefaultConf.stLTCPoolConf[j].szPasswd);
			((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].eAlgo = gPoolDefaultConf.stLTCPoolConf[j].eAlgo;
			((stCellConn *)pNode->val)->CellPoolConf.iLTCPoolCount++;
		}
		pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.PoolConfLock);




	}

	return rtn;
}*/

//初始化Database的数据到Cell节点中 
int InitDbToCell(int argc, char**argv, int iStartPos)
{
	int rtn = -1;
	int index = 0;
	//BYTE byCellIp[BUFFER_LEN_16];		//cell的ip	
	int iCellIp = 0, iCellPort = 0;						//cell的port	
	//int iLen = 0;
	//BYTE destBuf[BUFFER_LEN_256] = {0};
	//int iListLen = 0;
	//int i = 0;
	//E_LOGIN_ERROR eLoginErr;
	//BOOL bUpdateDb = FALSE;
	char bufPool[BUFFER_LEN_512] = {0};
	int argcA = 0;
	char *argvA[BUFFER_LEN_64] = {0};
	char delimA[] = " ";//, delimNcSend[] = "|";	
	//int i = 0;
	char bufBTCHash[BUFFER_LEN_512] = {0}, bufLTCHash[BUFFER_LEN_512] = {0};
	
	if (argc < iStartPos || argc != CELL_DB_COMMANDLINE)
		return 1;

	//if (InitDbToCellIdBase(argc, argv, iStartPos) != 0)
	//	return 2;

	index = iStartPos;
	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	stCellConn *val = malloc(sizeof(stCellConn));
	memset(val, 0, sizeof(stCellConn));
	
	//val->CellData.iCellId = strtoul(argv[index++], 0, 0);
	//strncpy((char *)val->CellData.szDevId, argv[index++], sizeof(val->CellData.szDevId));
	strncpy((char *)val->CellData.szCellId, argv[index++], sizeof(val->CellData.szCellId));
	iCellIp = strtoul(argv[index++], 0, 0);
	iCellPort = strtoul(argv[index++], 0, 0);
	//val->CellData.iSwVer = strtoul(argv[index++], 0, 0);
	//val->CellData.iHwVer = strtoul(argv[index++], 0, 0);
	strncpy((char *)val->CellData.szSwVer, argv[index++], sizeof(val->CellData.szSwVer));
	strncpy((char *)val->CellData.szHwVer, argv[index++], sizeof(val->CellData.szHwVer));
	val->CellData.eCellAlgo = strtoul(argv[index++], 0, 0);
	val->CellData.iChipCnt = strtoul(argv[index++], 0, 0);
	val->CellData.iChipBTCHash = strtoll(argv[index++], 0, 0);
	val->CellData.iChipLTCHash = strtoll(argv[index++], 0, 0);	
	val->CellData.iChipBTCFreq = strtoul(argv[index++], 0, 0);
	val->CellData.iChipLTCFreq = strtoul(argv[index++], 0, 0);
	val->CellData.bCellOnline = strtoul(argv[index++], 0, 0);
	strncpy(bufPool, argv[index++], sizeof(bufPool));
	strncpy((char *)val->CellData.szTestResult, argv[index++], sizeof(val->CellData.szTestResult));

	//解析pool_list
	DEBUGMSG(0,("bufPool:%s\r\n", bufPool));
	
	if (Parse(bufPool, &argcA, argvA, delimA) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, cell_base, pool_list parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		memset(bufPool, 0, sizeof(bufPool));
		//return 3;
	}
	else
	{
		if (ParseToPoolBuf(argcA, argvA, NULL, (void *)&val->CellPoolConf) != 0)
		{
			//GetLocalTimeLog();
			//ap_log_debug_log("\t\t%s, %s, %d, inside the parameters of poolid_pri!\r\n",	__FILE__, __FUNCTION__, __LINE__);
			memset(bufPool, 0, sizeof(bufPool));
			//return 4;
		}		
	}

	DEBUGMSG(0,("szCellId:%s, SwVer:%d, HwVer:%d, \r\n-----bufPool:%s\r\n", 
		val->CellData.szCellId, val->CellData.iSwVer, val->CellData.iHwVer, bufPool));

	/*iStartPos = index;
	//分配资源空间
	if ((index = AllocDbToCellRes(argc, argv, iStartPos, val->CellData.eCellAlgo, &val->pCellRes)) < 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, AllocDbToRes error!\r\n",  __FILE__, __FUNCTION__, __LINE__);	
		return 3;
	}*/
	
	val->CellData.iRegtime = strtoul(argv[index++], 0, 0);
	val->CellData.iLoginTime = strtoul(argv[index++], 0, 0);
	val->CellData.iLogoutTime = strtoul(argv[index++], 0, 0);
	strncpy(bufBTCHash, argv[index++], sizeof(bufBTCHash));
	strncpy(bufLTCHash, argv[index++], sizeof(bufLTCHash));
	val->remote.addr4.sin_family = AF_INET;
	val->remote.addr4.sin_port = iCellPort;
	val->remote.addr4.sin_addr.s_addr = htonl(iCellIp);
	val->CellParent.sock = gpLoginPool->listener.sock;

	DEBUGMSG(0,("+++%s +++%d, sock:%d\r\n", inet_ntoa(val->remote.addr4.sin_addr), val->remote.addr4.sin_port, val->CellParent.sock));	

	pthread_mutex_lock(stCellUtlist.InitNodeLock);
	//查找CellId	
	//devlist_st *CellNode = stCellUtlist.FindUserInt(stCellUtlist.HashUsers, val->CellData.iCellId);
	devlist_st *CellNode = stCellUtlist.FindUserStr(stCellUtlist.StrHUsers, (char *)val->CellData.szCellId);

	if (CellNode != NULL)
	{
		free(val);
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Get the same CellId [%s] from the database!\r\n",	
			__FILE__, __FUNCTION__, __LINE__, ((stCellConn *)CellNode->val)->CellData.szCellId);
	}
	else
	{		
		/*pthread_mutex_lock(stCellUtlist.ListLenLock);
		iListLen = stCellUtlist.CountUsersInt(*stCellUtlist.HashUsers);
		pthread_mutex_unlock(stCellUtlist.ListLenLock);*/
		//DEBUGMSG(0,("gLoginResIndex.iCellSum:%d\r\n", gLoginResIndex.iCellSum));
		//if (iListLen < gLoginResIndex.iCellSum)
	//{
		//if ((i = val->CellData.eCellAlgo) < sizeof(gCellAssi)/sizeof(gCellAssi[0]))
	//{
		//DEBUGMSG(0,("i:%d, pCrrAlgoLen:%d, pSumAlgoLen:%d\r\n", i, *gCellAssi[i].pCrrAlgoLen, *gCellAssi[i].pSumAlgoLen));
		//if (*gCellAssi[i].pCrrAlgoLen < *gCellAssi[i].pSumAlgoLen)
	//{
		/*pthread_mutex_lock(gResourceLock.CrrCellAlgoLenLock);
		eLoginErr = IncrCrrentAlgoLen(i, gCellAssi);
		pthread_mutex_unlock(gResourceLock.CrrCellAlgoLenLock);*/

		//if (eLoginErr == 0)
	//{
		//是否重新按照新的cpm重新分配
		/*if (gInitDbCpm != gpResource->iConnCellNum && gInitDbCpm != 0)
		{
			gCellAssi[i].ResToAlgoFunc(val, gpResource, gLoginResIndex.iMinerNum);
			val->eCellComm = E_COMMAND_TYPE_REBOOT;
		}	
		else
		{
			gCellAssi[i].DbResToAlgoFunc(val, gpResource, gLoginResIndex.iMinerNum);
			val->eCellComm = E_COMMAND_TYPE_STAT;
		}*/

		val->eCellComm = E_COMMAND_TYPE_STAT;
		DEBUGMSG(0,("*********eCellComm:%d\r\n", val->eCellComm));

		//初始化
		pthread_mutex_init(&val->CellDataLock.OnlineLock, NULL);
		pthread_mutex_init(&val->CellDataLock.AlarmTimerLock, NULL);
		pthread_mutex_init(&val->CellDataLock.IpPortLock, NULL);
		val->eCellMySql = 0;
		if (val->CellData.bCellOnline)
			val->CellData.iCellAlarmTimer = GetTime();
		else
			val->CellData.iCellAlarmTimer = 0;

		//计算额定功率
		CalculateRatedKHash(val);

		//计算实际功率
		InitCalculateBTCRealKHash(bufBTCHash, val);
		InitCalculateLTCRealKHash(bufBTCHash, val);

		//无重复CellId，插入链表
		//stCellUtlist.AddUserInt(stCellUtlist.HashUsers, val->CellData.iCellId, (void**)&val);
		stCellUtlist.AddUserStr(stCellUtlist.StrHUsers, (char *)val->CellData.szCellId, sizeof(val->CellData.szCellId), (void**)&val);

		//Init Monitor New Cell
		gNewCellSendLen++;
		rtn = 0;
	//}
	/*else
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, CellId [%d] BL has exceeded the maximum length set!\r\n",  __FILE__, __FUNCTION__, __LINE__, val->CellData.iCellId);
	}*/
	//}
		/*else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, CellId [%d] algo has exceeded the maximum length set!\r\n",  __FILE__, __FUNCTION__, __LINE__, val->CellData.iCellId);
		}*/
	//}
	//}
		/*else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, CellId [%d] has exceeded the maximum length set!\r\n",	__FILE__, __FUNCTION__, __LINE__, val->CellData.iCellId);
		}*/
	}
	pthread_mutex_unlock(stCellUtlist.InitNodeLock);

	return rtn;
}

//初始化Database的数据到Pool configuration节点中 
int InitDbToPool(int argc, char**argv, int iStartPos)
{
	int rtn = -1;
	int index = 0;
	//int iListLen = 0;
	//int i = 0;
	stPoolConfInfo *val = NULL;
	E_ALGO eAlgo;
	structList *pList = NULL;
	list_node_t *node = NULL;
	
	if (argc < iStartPos || argc != POOL_DB_COMMANDLINE)
		return 1;
	
	index = iStartPos;
	DEBUGMSG(0, ("%s..., argc:%d\r\n", __FUNCTION__, argc));
	val = malloc(sizeof(stPoolConfInfo));
	memset(val, 0, sizeof(stPoolConfInfo));

	//init
	val->iPoolId = strtoul(argv[index++], 0, 0);
	strncpy((char *)val->szUrl, argv[index++],sizeof(val->szUrl));
	strncpy((char *)val->szUser, argv[index++], sizeof(val->szUser));
	strncpy((char *)val->szPasswd, argv[index++], sizeof(val->szPasswd));
	eAlgo = strtoul(argv[index++], 0, 0);

	DEBUGMSG(0,("++++++++++++++++++++++eAlgo:%d\r\n", eAlgo));

	switch(eAlgo)
	{
	case E_ALGO_BTC:
		val->eAlgo = eAlgo;
		pList = &stBTCPoolConfList;
		//插入链表
		node = pList->ListNodeNew(val);
		assert(node->val == val);
		pList->ListRpush(pList->List, node, pList->ListLenLock);
		rtn = 0;
		break;

	case E_ALGO_LTC:
		val->eAlgo = eAlgo;
		pList = &stLTCPoolConfList;
		//插入链表
		node = pList->ListNodeNew(val);
		assert(node->val == val);
		pList->ListRpush(pList->List, node, pList->ListLenLock);
		rtn = 0;
		break;

	default:
		free(val);
		break;
	}

	return rtn;
}


//初始化Database的数据到DefaultPool configuration节点中 
int InitDbToDefaultPool(int argc, char**argv, int iStartPos)
{
	int rtn = -1;
	int index = 0;
	//int iListLen = 0;
	//int i = 0;
	//stPoolConfInfo *val = NULL;
	E_ALGO eAlgo;
	//structList *pList = NULL;
	//list_node_t *node = NULL;
	int j = 0;

	DEBUGMSG(0, ("%s..., argc:%d\r\n", __FUNCTION__, argc));
	
	if (argc < iStartPos || argc != POOL_DB_COMMANDLINE)
		return 1;
	
	index = iStartPos;
	DEBUGMSG(0,("index:%d\n", index));
	//val = malloc(sizeof(stPoolConfInfo));
	//memset(val, 0, sizeof(stPoolConfInfo));

	//init
	/*val->iPoolId = strtoul(argv[index++], 0, 0);
	strncpy((char *)val->szUrl, argv[index++],sizeof(val->szUrl));
	strncpy((char *)val->szUser, argv[index++], sizeof(val->szUser));
	strncpy((char *)val->szPasswd, argv[index++], sizeof(val->szPasswd));*/
	eAlgo = strtoul(argv[COMMANDLINE_4], 0, 0);

	DEBUGMSG(0,("++++++++++++++++++++++eAlgo:%d\r\n", eAlgo));

	switch(eAlgo)
	{
	case E_ALGO_BTC:
		/*val->eAlgo = eAlgo;
		pList = &stBTCPoolConfList;
		//插入链表
		node = pList->ListNodeNew(val);
		assert(node->val == val);
		pList->ListRpush(pList->List, node, pList->ListLenLock);
		rtn = 0;*/

		j = gPoolDefaultConf.iBTCPoolCount;
		DEBUGMSG(0,("j:%d, iBTCPoolCount:%d\n", j, gPoolDefaultConf.iBTCPoolCount));
		gPoolDefaultConf.stBTCPoolConf[j].iPoolId = strtoul(argv[index++], 0, 0);
		strcpy((char *)gPoolDefaultConf.stBTCPoolConf[j].szUrl, argv[index++]);
		strcpy((char *)gPoolDefaultConf.stBTCPoolConf[j].szUser, argv[index++]);
		strcpy((char *)gPoolDefaultConf.stBTCPoolConf[j].szPasswd, argv[index++]);
		gPoolDefaultConf.stBTCPoolConf[j].eAlgo = eAlgo;
		DEBUGMSG(0,("BTC szUrl:%s\n", (char *)gPoolDefaultConf.stBTCPoolConf[j].szUrl));
		
		if (gPoolDefaultConf.iBTCPoolCount++ > MAX_POOL_COUNT)
			gPoolDefaultConf.iBTCPoolCount = MAX_POOL_COUNT;
		rtn = 0;	
		break;

	case E_ALGO_LTC:
		/*val->eAlgo = eAlgo;
		pList = &stLTCPoolConfList;
		//插入链表
		node = pList->ListNodeNew(val);
		assert(node->val == val);
		pList->ListRpush(pList->List, node, pList->ListLenLock);*/

		j = gPoolDefaultConf.iLTCPoolCount;
		DEBUGMSG(0,("j:%d, iLTCPoolCount:%d\n", j, gPoolDefaultConf.iLTCPoolCount));
		gPoolDefaultConf.stLTCPoolConf[j].iPoolId = strtoul(argv[index++], 0, 0);
		strcpy((char *)gPoolDefaultConf.stLTCPoolConf[j].szUrl, argv[index++]);
		strcpy((char *)gPoolDefaultConf.stLTCPoolConf[j].szUser, argv[index++]);
		strcpy((char *)gPoolDefaultConf.stLTCPoolConf[j].szPasswd, argv[index++]);
		gPoolDefaultConf.stLTCPoolConf[j].eAlgo = eAlgo;
		DEBUGMSG(0,("LTC szUrl:%s\n", (char *)gPoolDefaultConf.stLTCPoolConf[j].szUrl));

		if (gPoolDefaultConf.iLTCPoolCount++ > MAX_POOL_COUNT)
			gPoolDefaultConf.iLTCPoolCount = MAX_POOL_COUNT;		
		rtn = 0;
		break;

	default:
		//free(val);
		break;
	}

	return rtn;
}


int InitDbToMaxPoolId(int argc, char**argv, int iStartPos)
{
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	if (argc < iStartPos || argc != DELCELLID_DB_COMMANDLINE)
		return 1;	

	gCrrMaxPoolId = strtoul(argv[0], 0, 0);

	DEBUGMSG(1,("gCrrMaxCellId:%d, argv[0]:%s\r\n", gCrrMaxPoolId, argv[0]));

	return 0;
}

int NotInitDb(int argc, char**argv, int iStartPos)
{	
	ap_log_debug_log("\t\tDo not need to initialize!\r\n");

	return 0;
}

//连接数据库
int MysqlConnect(MYSQL *db, stMyConnInfo *pInfo)
{
	int rtn = 0;
	int error = 0;
	char value = 1;

	mysql_init(db);
	
	if (mysql_real_connect(db, "localhost", (char *)pInfo->myUserName, (char *)pInfo->myUserPw, (char *)pInfo->myDbName, 0, NULL, CLIENT_FOUND_ROWS))  
	{
		mysql_options(db, MYSQL_OPT_RECONNECT, &value);
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s.%s database connection is successful!\r\n", pInfo->myDbName, pInfo->myTableName);
	}
	else
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s database connection failed\r\n", pInfo->myDbName);
		error = mysql_errno(db);
		if (error) 
		{
			ap_log_debug_log("Connection error %d: %s\r\n",error,mysql_error(db));
		}

		rtn = error;
	}

	return rtn;
}

//重新连接数据库
int MysqlReconnect(MYSQL *db, stMyConnInfo *pInfo)
{
	int rtn = 0;
	int error = 0;
	int rtnPing = 0;

	rtnPing = mysql_ping(db);
	DEBUGMSG(0,("rtnPing:%d\r\n", rtnPing));
	if(rtnPing)
	{
		if (mysql_real_connect(db, "localhost", (char *)pInfo->myUserName, (char *)pInfo->myUserPw, (char *)pInfo->myDbName, 0, NULL, CLIENT_FOUND_ROWS))  
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s.%s database connection is successful!\r\n", pInfo->myDbName, pInfo->myTableName);
		}
		else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s database connection failed\r\n", pInfo->myDbName);
			error = mysql_errno(db);
			if (error) 
			{
				ap_log_debug_log("Connection error %d: %s\r\n",error,mysql_error(db));
			}
		
			rtn = error;
		}
	}

	return rtn;
}


//关闭连接
int MysqlDisconnect(MYSQL *db)
{
	mysql_close(db);

	return 0;
}

#if 0
//向MinerBase数据库插入数据
int PackMinerBaseInsert(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;
	//BYTE AlgoBuf[BUFFER_LEN_16] = {0};
	//BYTE OnLineBuf[BUFFER_LEN_8] = {0};

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	//AlgoEnumToString(((stMinerConn *)pCoon)->MinerData.eMinerAlgo, (char *)AlgoBuf, sizeof(AlgoBuf));

	/*if (((stMinerConn *)pCoon)->MinerData.bMinerOnline == FALSE)
		strcpy((char *)OnLineBuf, "FALSE");
	else
		strcpy((char *)OnLineBuf, "TRUE");*/

	DEBUGMSG(0,("MinerAlgo:%d\r\n", ((stMinerConn *)pCoon)->MinerData.eMinerAlgo));
	switch(((stMinerConn *)pCoon)->MinerData.eMinerAlgo)
	{
	case E_ALGO_BTC://miner_id;miner_ip;miner_port;algo;is_online;wb_sid;cpm;wb_ip;wb_port;reg_time;login_time;logout_time;pool1_id;pool2_id;pool3_id;update_time;
		rtn = snprintf(dest, destLen, "replace into %s		 \
				(%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)values \
				(%d, %lu, %d, %d, %d, %d, %ld, %ld, %ld, %d, %d, %d, \'%s\')", 
			    (char *)((stMyConnInfo*)pInfo)->myTableName,
			    (char *)((stMinerDbColumnName*)pColName)->byKeyName,
				(char *)((stMinerDbColumnName*)pColName)->byMinerIp,
				(char *)((stMinerDbColumnName*)pColName)->byMinerPort,
				(char *)((stMinerDbColumnName*)pColName)->byAlgo,                                              
				(char *)((stMinerDbColumnName*)pColName)->byOnline,
				(char *)((stMinerDbColumnName*)pColName)->byBTCWbsId,
				//(char *)((stMinerDbColumnName*)pColName)->byBTCCellNum,
				//(char *)((stMinerDbColumnName*)pColName)->byBTCWbIp,
				//(char *)((stMinerDbColumnName*)pColName)->byBTCWbPort,				
				(char *)((stMinerDbColumnName*)pColName)->byRegTime,
				(char *)((stMinerDbColumnName*)pColName)->byLoginTime, 
				(char *)((stMinerDbColumnName*)pColName)->byLogoutTime,
				(char *)((stMinerDbColumnName*)pColName)->byPool1Id, 		
				(char *)((stMinerDbColumnName*)pColName)->byPool2Id, 
				(char *)((stMinerDbColumnName*)pColName)->byPool3Id,	
				(char *)((stMinerDbColumnName*)pColName)->byPath,
		
				((stMinerConn *)pCoon)->MinerData.iMinerId,
				(unsigned long)ntohl(((stMinerConn *)pCoon)->remote.addr4.sin_addr.s_addr),
				((stMinerConn *)pCoon)->remote.addr4.sin_port,
				((stMinerConn *)pCoon)->MinerData.eMinerAlgo, 
				((stMinerConn *)pCoon)->MinerData.bMinerOnline,
				((stMinerConn *)pCoon)->pMinerRes->iWbsId,
				//((stMinerConn *)pCoon)->pMinerRes->iConnCellNum,
				//ip2long((char *)((stMinerConn *)pCoon)->pMinerRes->WbIpPort.byWbIp),
				//(char *)((stMinerConn *)pCoon)->pMinerRes->WbIpPort.byWbPort,
				((stMinerConn *)pCoon)->MinerData.iRegtime,
				((stMinerConn *)pCoon)->MinerData.iLoginTime,
				((stMinerConn *)pCoon)->MinerData.iLogoutTime,
				0,
				1,
				2,
				"/home/zc/bfgminer"
				);
		break;

	case E_ALGO_LTC:
		rtn = snprintf(dest, destLen, "replace into %s		 \
				(%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)values \
				(%d, %lu, %d, %d, %d, %d, %ld, %ld, %ld, %d, %d, %d, \'%s\')", 
			    (char *)((stMyConnInfo*)pInfo)->myTableName,
			    (char *)((stMinerDbColumnName*)pColName)->byKeyName,
				(char *)((stMinerDbColumnName*)pColName)->byMinerIp,
				(char *)((stMinerDbColumnName*)pColName)->byMinerPort,
				(char *)((stMinerDbColumnName*)pColName)->byAlgo,                                              
				(char *)((stMinerDbColumnName*)pColName)->byOnline,
				(char *)((stMinerDbColumnName*)pColName)->byBTCWbsId,
				//(char *)((stMinerDbColumnName*)pColName)->byBTCCellNum,			
				(char *)((stMinerDbColumnName*)pColName)->byRegTime,
				(char *)((stMinerDbColumnName*)pColName)->byLoginTime, 
				(char *)((stMinerDbColumnName*)pColName)->byLogoutTime,
				(char *)((stMinerDbColumnName*)pColName)->byPool1Id, 		
				(char *)((stMinerDbColumnName*)pColName)->byPool2Id, 
				(char *)((stMinerDbColumnName*)pColName)->byPool3Id,	
				(char *)((stMinerDbColumnName*)pColName)->byPath,
		
				((stMinerConn *)pCoon)->MinerData.iMinerId,
				(unsigned long)ntohl(((stMinerConn *)pCoon)->remote.addr4.sin_addr.s_addr),
				((stMinerConn *)pCoon)->remote.addr4.sin_port,
				((stMinerConn *)pCoon)->MinerData.eMinerAlgo, 
				((stMinerConn *)pCoon)->MinerData.bMinerOnline,
				((stMinerConn *)pCoon)->pMinerRes->iWbsId,
				//((stMinerConn *)pCoon)->pMinerRes->iConnCellNum,
				((stMinerConn *)pCoon)->MinerData.iRegtime,
				((stMinerConn *)pCoon)->MinerData.iLoginTime,
				((stMinerConn *)pCoon)->MinerData.iLogoutTime,
				0,
				1,
				2,
				"/home/zc/bfgminer"
				);		
		break;
		
	case E_ALGO_DUAL://miner_id;miner_ip;miner_port;algo;is_online;btc_wbsid;btc_cpm;ltc_wbsid;ltc_cpm;reg_time;login_time;logout_time;pool1_id;pool2_id;pool3_id;path
		rtn = snprintf(dest, destLen, "replace into %s		 \
				(%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)values \
				(%d, %lu, %d, %d, %d, %d, %d, %ld, %ld, %ld, %d, %d, %d, \'%s\')", 
			    (char *)((stMyConnInfo*)pInfo)->myTableName,
			    (char *)((stMinerDbColumnName*)pColName)->byKeyName,
				(char *)((stMinerDbColumnName*)pColName)->byMinerIp,
				(char *)((stMinerDbColumnName*)pColName)->byMinerPort,
				(char *)((stMinerDbColumnName*)pColName)->byAlgo,                                              
				(char *)((stMinerDbColumnName*)pColName)->byOnline,
				(char *)((stMinerDbColumnName*)pColName)->byBTCWbsId,
				//(char *)((stMinerDbColumnName*)pColName)->byBTCCellNum,					
				(char *)((stMinerDbColumnName*)pColName)->byLTCWbsId,
				//(char *)((stMinerDbColumnName*)pColName)->byLTCCellNum,			
				(char *)((stMinerDbColumnName*)pColName)->byRegTime,
				(char *)((stMinerDbColumnName*)pColName)->byLoginTime, 
				(char *)((stMinerDbColumnName*)pColName)->byLogoutTime,
				(char *)((stMinerDbColumnName*)pColName)->byPool1Id, 		
				(char *)((stMinerDbColumnName*)pColName)->byPool2Id, 
				(char *)((stMinerDbColumnName*)pColName)->byPool3Id,	
				(char *)((stMinerDbColumnName*)pColName)->byPath,
		
				((stMinerConn *)pCoon)->MinerData.iMinerId,
				(unsigned long)ntohl(((stMinerConn *)pCoon)->remote.addr4.sin_addr.s_addr),
				((stMinerConn *)pCoon)->remote.addr4.sin_port,
				((stMinerConn *)pCoon)->MinerData.eMinerAlgo, 
				((stMinerConn *)pCoon)->MinerData.bMinerOnline,
				((stMinerConn *)pCoon)->pMinerRes[0].iWbsId,
				//((stMinerConn *)pCoon)->pMinerRes[0].iConnCellNum,
				((stMinerConn *)pCoon)->pMinerRes[1].iWbsId,
				//((stMinerConn *)pCoon)->pMinerRes[1].iConnCellNum,				
				((stMinerConn *)pCoon)->MinerData.iRegtime,
				((stMinerConn *)pCoon)->MinerData.iLoginTime,
				((stMinerConn *)pCoon)->MinerData.iLogoutTime,
				0,
				1,
				2,
				"/home/zc/bfgminer"
				);
		break;

	default:
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Algorithm[%d] does not exist!\r\n", __FILE__, __FUNCTION__, __LINE__, ((stMinerConn *)pCoon)->eClinetType);
		break;
	}

	//if (((stMinerConn *)pCoon)->MinerData.iMinerId > 255 || ((stMinerConn *)pCoon)->MinerData.iMinerId == 0)
	//	printf("iMinerId:%d, iLoginTime:%d\r\n", ((stMinerConn *)pCoon)->MinerData.iMinerId, ((stMinerConn *)pCoon)->MinerData.iLoginTime);

	return rtn;
}

int PackMinerBaseSelectSingle(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	rtn = snprintf(dest, destLen, "select %s from %s where %s = %d", 
		     (char *)((stMinerDbColumnName*)pColName)->byKeyName, 
		     (char *)((stMyConnInfo*)pInfo)->myDbName, 
		     (char *)((stMinerDbColumnName*)pColName)->byKeyName, 
		     ((stMinerConn *)pCoon)->MinerData.iMinerId);

	return rtn;
}

int PackMinerBaseUpdate(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));
	rtn = snprintf(dest, destLen, "update %s set %s=%lu, %s=%u, %s=%d, %s=%ld, %s=%ld, %s=%ld where %s = %d", 
			(char *)((stMyConnInfo*)pInfo)->myTableName,
			(char *)((stMinerDbColumnName*)pColName)->byMinerIp,
			(unsigned long)ntohl(((stMinerConn *)pCoon)->remote.addr4.sin_addr.s_addr),
			(char *)((stMinerDbColumnName*)pColName)->byMinerPort,
			((stMinerConn *)pCoon)->remote.addr4.sin_port,
			(char *)((stMinerDbColumnName*)pColName)->byOnline,
			((stMinerConn *)pCoon)->MinerData.bMinerOnline,
			(char *)((stMinerDbColumnName*)pColName)->byRegTime,
			((stMinerConn *)pCoon)->MinerData.iRegtime,		
			(char *)((stMinerDbColumnName*)pColName)->byLoginTime, 
			((stMinerConn *)pCoon)->MinerData.iLoginTime,	
			(char *)((stMinerDbColumnName*)pColName)->byLogoutTime,
			((stMinerConn *)pCoon)->MinerData.iLogoutTime,			
			(char *)((stMinerDbColumnName*)pColName)->byKeyName, 
			((stMinerConn *)pCoon)->MinerData.iMinerId);

	//printf("%s..., dest:%s\r\n", __FUNCTION__, dest);
	return rtn;
}

int PackMinerBaseAlter(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackMinerDeleteOneRow(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));

	rtn = snprintf(dest, destLen, "delete from %s.%s where %s = %d", 
			(char *)((stMyConnInfo*)pInfo)->myDbName, 
			(char *)((stMyConnInfo*)pInfo)->myTableName,
			((stMinerDbColumnName*)pColName)->byKeyName,
			((stMinerConn *)pCoon)->MinerData.iMinerId);

	return rtn;
}
#endif
/*int PackMinerBaseCreate(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	rtn = snprintf(dest, destLen, "create table %s.%s	\
		(%s int not null primary key auto_increment, %s bigint unsigned not null, 				\
		%s int not null, %s enum('btc','ltc','dual') default null, %s bool not null, 			\
		%s int not null, %s int not null, %s bigint unsigned not null, %s int not null, 		\
		%s int unsigned not null, %s int unsigned not null, %s int unsigned not null, %s int not null,	\
		%s int not null, %s int not null)", 
		(char *)((stMyConnInfo*)pInfo)->myDbName, (char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stMinerDbColumnName*)pColName)->byKeyName,
		(char *)((stMinerDbColumnName*)pColName)->byMinerIp,
		(char *)((stMinerDbColumnName*)pColName)->byMinerPort,
		(char *)((stMinerDbColumnName*)pColName)->byAlgo,                                              
		(char *)((stMinerDbColumnName*)pColName)->byOnline,
		(char *)((stMinerDbColumnName*)pColName)->byBTCWbsId,
		(char *)((stMinerDbColumnName*)pColName)->byBTCCellNum,
		(char *)((stMinerDbColumnName*)pColName)->byBTCWbIp,
		(char *)((stMinerDbColumnName*)pColName)->byBTCWbPort,				
		(char *)((stMinerDbColumnName*)pColName)->byRegTime,
		(char *)((stMinerDbColumnName*)pColName)->byLoginTime, 
		(char *)((stMinerDbColumnName*)pColName)->byLogoutTime,
		(char *)((stMinerDbColumnName*)pColName)->byPool1Id, 		
		(char *)((stMinerDbColumnName*)pColName)->byPool2Id, 
		(char *)((stMinerDbColumnName*)pColName)->byPool3Id
		);

	return rtn;
}*/

int PackCellBaseInsert(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;
	int j = 0;
	//BYTE AlgoBuf[BUFFER_LEN_16] = {0};
	//BYTE OnLineBuf[BUFFER_LEN_8] = {0};
	char bufPool[BUFFER_LEN_512] = {0};
	int iPosPool = 0;
	BYTE AlgoBuf[BUFFER_LEN_16] = {0};

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	//AlgoEnumToString(((stCellConn *)pCoon)->CellData.eCellAlgo, (char *)AlgoBuf, sizeof(AlgoBuf));

	/*if (((stCellConn *)pCoon)->CellData.bCellOnline == FALSE)
		strcpy((char *)OnLineBuf, "FALSE");
	else
		strcpy((char *)OnLineBuf, "TRUE");*/

	//BTC,其实保存一个算法，就可以了。
	if (((stCellConn *)pCoon)->CellPoolConf.iBTCPoolCount != 0)
	{
		AlgoEnumToString(((stCellConn *)pCoon)->CellPoolConf.stBTCPoolConf[0].eAlgo, (char *)AlgoBuf, sizeof(AlgoBuf));
		snprintf(bufPool + iPosPool, sizeof(bufPool), "%s", AlgoBuf);
		iPosPool += strlen(bufPool + iPosPool);
		memset(AlgoBuf, 0, sizeof(AlgoBuf));
	}
		
	for(j = 0;j < ((stCellConn *)pCoon)->CellPoolConf.iBTCPoolCount && ((stCellConn *)pCoon)->CellPoolConf.iBTCPoolCount <= MAX_POOL_COUNT;j++)
	{	
		//数据库中值，目前只保存pool_id吧。
		snprintf(bufPool + iPosPool, sizeof(bufPool), "/%d&%s&%s&%s", 
			((stCellConn *)pCoon)->CellPoolConf.stBTCPoolConf[j].iPoolId,
			(char *)((stCellConn *)pCoon)->CellPoolConf.stBTCPoolConf[j].szUrl,
			(char *)((stCellConn *)pCoon)->CellPoolConf.stBTCPoolConf[j].szUser,
			(char *)((stCellConn *)pCoon)->CellPoolConf.stBTCPoolConf[j].szPasswd
			);
		iPosPool += strlen(bufPool + iPosPool);
	}
	
	//LTC,其实保存一个算法，就可以了。
	if ( ((stCellConn *)pCoon)->CellPoolConf.iLTCPoolCount  != 0)
	{
		AlgoEnumToString(((stCellConn *)pCoon)->CellPoolConf.stLTCPoolConf[0].eAlgo, (char *)AlgoBuf, sizeof(AlgoBuf));
		snprintf(bufPool + iPosPool, sizeof(bufPool), " %s", AlgoBuf);
		iPosPool += strlen(bufPool + iPosPool);
		memset(AlgoBuf, 0, sizeof(AlgoBuf));
	}

	for(j = 0;j < ((stCellConn *)pCoon)->CellPoolConf.iLTCPoolCount && ((stCellConn *)pCoon)->CellPoolConf.iLTCPoolCount <= MAX_POOL_COUNT;j++)
	{	
		snprintf(bufPool + iPosPool, sizeof(bufPool), "/%d&%s&%s&%s", 
			((stCellConn *)pCoon)->CellPoolConf.stLTCPoolConf[j].iPoolId,
			(char *)((stCellConn *)pCoon)->CellPoolConf.stLTCPoolConf[j].szUrl,
			(char *)((stCellConn *)pCoon)->CellPoolConf.stLTCPoolConf[j].szUser,
			(char *)((stCellConn *)pCoon)->CellPoolConf.stLTCPoolConf[j].szPasswd
			//((stCellConn *)pCoon)->CellPoolConf.stLTCPoolConf[j].eAlgo
			);
		iPosPool += strlen(bufPool + iPosPool);
	}				

	DEBUGMSG(0,("111111111111 @pCoon:%p, sin_port:%d\n", (stCellConn *)pCoon, ((stCellConn *)pCoon)->remote.addr4.sin_port));
	DEBUGMSG(0,("CellAlgo:%d\r\n", ((stCellConn *)pCoon)->CellData.eCellAlgo));
	rtn =snprintf(dest, destLen, "replace into %s		 \
		(%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)values \
		('%s', %lu, %d, '%s', '%s', %u, %u, %lld, %lld, %u, %u, %d, '%s', '%s', %ld, %ld, %ld)", 
		(char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stCellDbColumnName *)pColName)->byKeyName,
		//(char *)((stCellDbColumnName *)pColName)->byDevId,//%s, 
		(char *)((stCellDbColumnName *)pColName)->byCellIp,
		(char *)((stCellDbColumnName *)pColName)->byCellPort,
		(char *)((stCellDbColumnName *)pColName)->bySwVer,
		(char *)((stCellDbColumnName *)pColName)->byHwVer,
		(char *)((stCellDbColumnName *)pColName)->byAlgo,
		(char *)((stCellDbColumnName *)pColName)->byChipCnt,
		(char *)((stCellDbColumnName *)pColName)->byChipBTCHash,	
		(char *)((stCellDbColumnName *)pColName)->byChipLTCHash,
		(char *)((stCellDbColumnName *)pColName)->byBTCFreq,
		(char *)((stCellDbColumnName *)pColName)->byLTCFreq,	
		(char *)((stCellDbColumnName *)pColName)->byOnline,
		(char *)((stCellDbColumnName *)pColName)->szPoolList,
		(char *)((stCellDbColumnName *)pColName)->byTestResult,
		//(char *)((stCellDbColumnName *)pColName)->byBTCWbsId,
		//(char *)((stCellDbColumnName *)pColName)->byBTCWbcId,
		//(char *)((stCellDbColumnName *)pColName)->byBTCCpm,
		//(char *)((stCellDbColumnName *)pColName)->byBTCWbIp,
		//(char *)((stCellDbColumnName *)pColName)->byBTCWbPort,
		(char *)((stCellDbColumnName *)pColName)->byRegTime,
		(char *)((stCellDbColumnName *)pColName)->byLoginTime,
		(char *)((stCellDbColumnName *)pColName)->byLogoutTime,
		
		((stCellConn *)pCoon)->CellData.szCellId,
		//((stCellConn *)pCoon)->CellData.szDevId,//%d, 
		(unsigned long)ntohl(((stCellConn *)pCoon)->remote.addr4.sin_addr.s_addr),
		((stCellConn *)pCoon)->remote.addr4.sin_port,				
		//((stCellConn *)pCoon)->CellData.iSwVer,	//%u
		//((stCellConn *)pCoon)->CellData.iHwVer,	//%u
		((stCellConn *)pCoon)->CellData.szSwVer,
		((stCellConn *)pCoon)->CellData.szHwVer,		
		((stCellConn *)pCoon)->CellData.eCellAlgo,
		((stCellConn *)pCoon)->CellData.iChipCnt,
		((stCellConn *)pCoon)->CellData.iChipBTCHash,
		((stCellConn *)pCoon)->CellData.iChipLTCHash,
		((stCellConn *)pCoon)->CellData.iChipBTCFreq,
		((stCellConn *)pCoon)->CellData.iChipLTCFreq,
		((stCellConn *)pCoon)->CellData.bCellOnline,
		bufPool,		
		((stCellConn *)pCoon)->CellData.szTestResult,
		//((stCellConn *)pCoon)->pCellRes->iWbNo,
		//((stCellConn *)pCoon)->pCellRes->iWbcId,
		//((stCellConn *)pCoon)->pCellRes->iConnCellNum,
		//ip2long((char *)((stCellConn *)pCoon)->pCellRes->WbIpPort.byWbIp),
		//(char *)((stCellConn *)pCoon)->pCellRes->WbIpPort.byWbPort,
		((stCellConn *)pCoon)->CellData.iRegtime,
		((stCellConn *)pCoon)->CellData.iLoginTime,
		((stCellConn *)pCoon)->CellData.iLogoutTime
		);
	DEBUGMSG(0,("2222222222222 @pCoon:%p, sin_port:%d\n", (stCellConn *)pCoon, ((stCellConn *)pCoon)->remote.addr4.sin_port));
	/*switch(((stCellConn *)pCoon)->CellData.eCellAlgo)
	{
	case E_ALGO_BTC://CellId, CellIp, CellPort, SwVer, HwVer, CalcPower, TestResult, Algo, Online, BTCWbNo, BTCWbcId, BTCCellNum, BTCWbIp, BTCWbPort
		rtn =snprintf(dest, destLen, "replace into %s        \
	        (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)values \
	        (%d, %ld, %lu, %d, %u, %u, %u, %u, %u, %u, %d, %ld, %ld, %ld)", 
	        (char *)((stMyConnInfo*)pInfo)->myTableName,
		    (char *)((stCellDbColumnName *)pColName)->byKeyName,
		    (char *)((stCellDbColumnName *)pColName)->byDevId,
			(char *)((stCellDbColumnName *)pColName)->byCellIp,
			(char *)((stCellDbColumnName *)pColName)->byCellPort,
			(char *)((stCellDbColumnName *)pColName)->bySwVer,
			(char *)((stCellDbColumnName *)pColName)->byHwVer,
			(char *)((stCellDbColumnName *)pColName)->byChipBTCHash,
			(char *)((stCellDbColumnName *)pColName)->byChipCnt,
			(char *)((stCellDbColumnName *)pColName)->byChipFreq,
			(char *)((stCellDbColumnName *)pColName)->byAlgo,											   
			(char *)((stCellDbColumnName *)pColName)->byOnline,
			//(char *)((stCellDbColumnName *)pColName)->byBTCWbsId,
			//(char *)((stCellDbColumnName *)pColName)->byBTCWbcId,
			//(char *)((stCellDbColumnName *)pColName)->byBTCCpm,
			//(char *)((stCellDbColumnName *)pColName)->byBTCWbIp,
			//(char *)((stCellDbColumnName *)pColName)->byBTCWbPort,			
			(char *)((stCellDbColumnName *)pColName)->byRegTime,
			(char *)((stCellDbColumnName *)pColName)->byLoginTime,
			(char *)((stCellDbColumnName *)pColName)->byLogoutTime,
			
	        ((stCellConn *)pCoon)->CellData.iCellId,
	        ((stCellConn *)pCoon)->CellData.lDevId,
			(unsigned long)ntohl(((stCellConn *)pCoon)->remote.addr4.sin_addr.s_addr),
			((stCellConn *)pCoon)->remote.addr4.sin_port,		        
	        ((stCellConn *)pCoon)->CellData.iSwVer,
	        ((stCellConn *)pCoon)->CellData.iHwVer,
	        ((stCellConn *)pCoon)->CellData.iChipBTCHash,
	        ((stCellConn *)pCoon)->CellData.iChipCnt,
	        ((stCellConn *)pCoon)->CellData.iChipBTCFreq,
	        ((stCellConn *)pCoon)->CellData.eCellAlgo,
	        ((stCellConn *)pCoon)->CellData.bCellOnline,
	        //((stCellConn *)pCoon)->pCellRes->iWbNo,
	        //((stCellConn *)pCoon)->pCellRes->iWbcId,
	        //((stCellConn *)pCoon)->pCellRes->iConnCellNum,
	        //ip2long((char *)((stCellConn *)pCoon)->pCellRes->WbIpPort.byWbIp),
	        //(char *)((stCellConn *)pCoon)->pCellRes->WbIpPort.byWbPort,
	        ((stCellConn *)pCoon)->CellData.iRegtime,
			((stCellConn *)pCoon)->CellData.iLoginTime,
			((stCellConn *)pCoon)->CellData.iLogoutTime
			);
		break;

	case E_ALGO_LTC://CellId, CellIp, CellPort, SwVer, HwVer, CalcPower, TestResult, Algo, Online, LTCWbNo, LTCWbcId, LTCCellNum, LTCWbIp, LTCWbPort
			rtn =snprintf(dest, destLen, "replace into %s        \
	        (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)values \
	        (%d, %ld, %lu, %d, %u, %u, %u, %u, %u, %u, %d, %d, %d, %ld, %ld, %ld)", 
	        (char *)((stMyConnInfo*)pInfo)->myTableName,     
			(char *)((stCellDbColumnName *)pColName)->byKeyName,
			(char *)((stCellDbColumnName *)pColName)->byDevId,
			(char *)((stCellDbColumnName *)pColName)->byCellIp,
			(char *)((stCellDbColumnName *)pColName)->byCellPort,
			(char *)((stCellDbColumnName *)pColName)->bySwVer,
			(char *)((stCellDbColumnName *)pColName)->byHwVer,
			(char *)((stCellDbColumnName *)pColName)->byChipBTCHash,
			(char *)((stCellDbColumnName *)pColName)->byChipCnt,
			(char *)((stCellDbColumnName *)pColName)->byChipFreq,
			(char *)((stCellDbColumnName *)pColName)->byAlgo, 
			(char *)((stCellDbColumnName *)pColName)->byOnline,
			(char *)((stCellDbColumnName *)pColName)->byLTCWbsId,
			(char *)((stCellDbColumnName *)pColName)->byLTCWbcId,
			//(char *)((stCellDbColumnName *)pColName)->byLTCCpm,
			//(char *)((stCellDbColumnName *)pColName)->byLTCWbIp,
			//(char *)((stCellDbColumnName *)pColName)->byLTCWbPort,
			(char *)((stCellDbColumnName *)pColName)->byRegTime,
			(char *)((stCellDbColumnName *)pColName)->byLoginTime,
			(char *)((stCellDbColumnName *)pColName)->byLogoutTime,
			
	        ((stCellConn *)pCoon)->CellData.iCellId,
	        ((stCellConn *)pCoon)->CellData.lDevId,
			(unsigned long)ntohl(((stCellConn *)pCoon)->remote.addr4.sin_addr.s_addr),
			((stCellConn *)pCoon)->remote.addr4.sin_port,	
	        ((stCellConn *)pCoon)->CellData.iSwVer,
	        ((stCellConn *)pCoon)->CellData.iHwVer,
	        ((stCellConn *)pCoon)->CellData.iChipBTCHash,
	        ((stCellConn *)pCoon)->CellData.iChipCnt,
	        ((stCellConn *)pCoon)->CellData.iChipBTCFreq,
	        ((stCellConn *)pCoon)->CellData.eCellAlgo,
	        ((stCellConn *)pCoon)->CellData.bCellOnline,
	        ((stCellConn *)pCoon)->pCellRes->iWbNo,
	        ((stCellConn *)pCoon)->pCellRes->iWbcId,
	        //((stCellConn *)pCoon)->pCellRes->iConnCellNum,
	        //ip2long((char *)((stCellConn *)pCoon)->pCellRes->WbIpPort.byWbIp),
	        //(char *)((stCellConn *)pCoon)->pCellRes->WbIpPort.byWbPort,
			((stCellConn *)pCoon)->CellData.iRegtime,
			((stCellConn *)pCoon)->CellData.iLoginTime,
			((stCellConn *)pCoon)->CellData.iLogoutTime
			);
		break;
		
	case E_ALGO_DUAL://CellId, CellIp, CellPort, SwVer, HwVer, CalcPower, TestResult, Algo, Online, BTCWbNo, BTCWbcId, BTCCellNum, BTCWbIp, BTCWbPort, LTCWbNo, LTCWbcId, LTCCellNum, LTCWbIp, LTCWbPort
		rtn = snprintf(dest, destLen, "replace into %s        \
	        (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)values \
	        (%d, %ld, %lu, %d, %u, %u, %u, %u, %u, %u, %d, %d, %d, %d, %d, %ld, %ld, %ld)", 
		    (char *)((stMyConnInfo*)pInfo)->myTableName,
		    (char *)((stCellDbColumnName *)pColName)->byKeyName,
		    (char *)((stCellDbColumnName *)pColName)->byDevId,
			(char *)((stCellDbColumnName *)pColName)->byCellIp,
			(char *)((stCellDbColumnName *)pColName)->byCellPort,
			(char *)((stCellDbColumnName *)pColName)->bySwVer,
			(char *)((stCellDbColumnName *)pColName)->byHwVer,
			(char *)((stCellDbColumnName *)pColName)->byChipBTCHash,
			(char *)((stCellDbColumnName *)pColName)->byChipCnt,
			(char *)((stCellDbColumnName *)pColName)->byChipFreq,
			(char *)((stCellDbColumnName *)pColName)->byAlgo,
			(char *)((stCellDbColumnName *)pColName)->byOnline,
			(char *)((stCellDbColumnName *)pColName)->byBTCWbsId,
			(char *)((stCellDbColumnName *)pColName)->byBTCWbcId,
			//(char *)((stCellDbColumnName *)pColName)->byBTCCpm,
			//(char *)((stCellDbColumnName *)pColName)->byBTCWbIp,
			//(char *)((stCellDbColumnName *)pColName)->byBTCWbPort,
			(char *)((stCellDbColumnName *)pColName)->byLTCWbsId,
			(char *)((stCellDbColumnName *)pColName)->byLTCWbcId,
			//(char *)((stCellDbColumnName *)pColName)->byLTCCpm,
			//(char *)((stCellDbColumnName *)pColName)->byLTCWbIp,
			//(char *)((stCellDbColumnName *)pColName)->byLTCWbPort,
			(char *)((stCellDbColumnName *)pColName)->byRegTime,
			(char *)((stCellDbColumnName *)pColName)->byLoginTime,
			(char *)((stCellDbColumnName *)pColName)->byLogoutTime,
			
			((stCellConn *)pCoon)->CellData.iCellId,
			((stCellConn *)pCoon)->CellData.lDevId,
			(unsigned long)ntohl(((stCellConn *)pCoon)->remote.addr4.sin_addr.s_addr),
			((stCellConn *)pCoon)->remote.addr4.sin_port,			
	        ((stCellConn *)pCoon)->CellData.iSwVer,
	        ((stCellConn *)pCoon)->CellData.iHwVer,
	        ((stCellConn *)pCoon)->CellData.iChipBTCHash,
	        ((stCellConn *)pCoon)->CellData.iChipCnt,
	        ((stCellConn *)pCoon)->CellData.iChipBTCFreq,
	        ((stCellConn *)pCoon)->CellData.eCellAlgo,
	        ((stCellConn *)pCoon)->CellData.bCellOnline,
	        ((stCellConn *)pCoon)->pCellRes[0].iWbNo,
	        ((stCellConn *)pCoon)->pCellRes[0].iWbcId,
	        //((stCellConn *)pCoon)->pCellRes[0].iConnCellNum,
	        //ip2long((char *)((stCellConn *)pCoon)->pCellRes[0].WbIpPort.byWbIp),
	        //(char *)((stCellConn *)pCoon)->pCellRes[0].WbIpPort.byWbPort,
	        ((stCellConn *)pCoon)->pCellRes[1].iWbNo,
	        ((stCellConn *)pCoon)->pCellRes[1].iWbcId,
	        //((stCellConn *)pCoon)->pCellRes[1].iConnCellNum,
	        //ip2long((char *)((stCellConn *)pCoon)->pCellRes[1].WbIpPort.byWbIp),
	        //(char *)((stCellConn *)pCoon)->pCellRes[1].WbIpPort.byWbPort,
	        ((stCellConn *)pCoon)->CellData.iRegtime,
			((stCellConn *)pCoon)->CellData.iLoginTime,
			((stCellConn *)pCoon)->CellData.iLogoutTime
			);			
		break;
		
	default:
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Algorithm[%d] does not exist!\r\n", __FILE__, __FUNCTION__, __LINE__, ((stCellConn *)pCoon)->eClinetType);
		break;
	}*/

	return rtn;
}

int PackCellBaseSelectSingle(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	rtn = snprintf(dest, destLen, "select %s from %s where %s = \'%s\'", 
		     (char *)((stCellDbColumnName *)pColName)->byKeyName, 
		     (char *)((stMyConnInfo*)pInfo)->myDbName, 
		     (char *)((stCellDbColumnName *)pColName)->byKeyName, 
		     (char *)((stCellConn *)pCoon)->CellData.szCellId);

	return rtn;
}

int PackCellBaseUpdate(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;
	
	rtn = snprintf(dest, destLen, "update %s set %s=%lu, %s=%d, %s=\'%s\', %s=\'%s\', %s=%u, %s=%u, %s=%d, %s=%ld, %s=%ld, %s=%ld where %s = \'%s\'", 
			(char *)((stMyConnInfo*)pInfo)->myTableName,
			(char *)((stCellDbColumnName *)pColName)->byCellIp,
			(unsigned long)ntohl(((stCellConn *)pCoon)->remote.addr4.sin_addr.s_addr),
			(char *)((stCellDbColumnName *)pColName)->byCellPort,
			((stCellConn *)pCoon)->remote.addr4.sin_port,
			(char *)((stCellDbColumnName *)pColName)->bySwVer,
			((stCellConn *)pCoon)->CellData.szSwVer,
			(char *)((stCellDbColumnName *)pColName)->byHwVer,
			((stCellConn *)pCoon)->CellData.szHwVer,	
			(char *)((stCellDbColumnName *)pColName)->byBTCFreq,
			((stCellConn *)pCoon)->CellData.iChipBTCFreq,
			(char *)((stCellDbColumnName *)pColName)->byLTCFreq,
			((stCellConn *)pCoon)->CellData.iChipLTCFreq,
			(char *)((stCellDbColumnName *)pColName)->byOnline,
			((stCellConn *)pCoon)->CellData.bCellOnline,
			(char *)((stCellDbColumnName*)pColName)->byRegTime,
			((stCellConn *)pCoon)->CellData.iRegtime, 	
			(char *)((stCellDbColumnName*)pColName)->byLoginTime, 
			((stCellConn *)pCoon)->CellData.iLoginTime,
			(char *)((stCellDbColumnName*)pColName)->byLogoutTime,
			((stCellConn *)pCoon)->CellData.iLogoutTime,	 
			(char *)((stCellDbColumnName *)pColName)->byKeyName, 
			(char *)((stCellConn *)pCoon)->CellData.szCellId);

	return rtn;
}

int PackCellBaseUpdatePool(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0, j = 0;
	//long iUpdateTime = 0;
	char bufPool[BUFFER_LEN_512] = {0};
	int iPosPool = 0;
	BYTE AlgoBuf[BUFFER_LEN_16] = {0};

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	//iUpdateTime = GetTime();
	//snprintf(bufPool + iPosPool, sizeof(bufPool), "%d", ((stCellConn *)pCoon)->CellPoolConf.iBTCPoolCount);

	//BTC,其实保存一个算法，就可以了。

	if (((stCellConn *)pCoon)->CellPoolConf.iBTCPoolCount != 0)
	{
		AlgoEnumToString(((stCellConn *)pCoon)->CellPoolConf.stBTCPoolConf[0].eAlgo, (char *)AlgoBuf, sizeof(AlgoBuf));
		snprintf(bufPool + iPosPool, sizeof(bufPool), "%s", AlgoBuf);
		iPosPool += strlen(bufPool + iPosPool);
		memset(AlgoBuf, 0, sizeof(AlgoBuf));
	}
		
	for(j = 0;j < ((stCellConn *)pCoon)->CellPoolConf.iBTCPoolCount && ((stCellConn *)pCoon)->CellPoolConf.iBTCPoolCount <= MAX_POOL_COUNT;j++)
	{	
		//数据库中值，目前只保存pool_id吧。
		snprintf(bufPool + iPosPool, sizeof(bufPool), "#%d&%s&%s&%s", 
			((stCellConn *)pCoon)->CellPoolConf.stBTCPoolConf[j].iPoolId,
			(char *)((stCellConn *)pCoon)->CellPoolConf.stBTCPoolConf[j].szUrl,
			(char *)((stCellConn *)pCoon)->CellPoolConf.stBTCPoolConf[j].szUser,
			(char *)((stCellConn *)pCoon)->CellPoolConf.stBTCPoolConf[j].szPasswd
			);
		iPosPool += strlen(bufPool + iPosPool);
	}
	
	//LTC,其实保存一个算法，就可以了。
	if ( ((stCellConn *)pCoon)->CellPoolConf.iLTCPoolCount  != 0)
	{
		AlgoEnumToString(((stCellConn *)pCoon)->CellPoolConf.stLTCPoolConf[0].eAlgo, (char *)AlgoBuf, sizeof(AlgoBuf));
		snprintf(bufPool + iPosPool, sizeof(bufPool), " %s", AlgoBuf);
		iPosPool += strlen(bufPool + iPosPool);
		memset(AlgoBuf, 0, sizeof(AlgoBuf));
	}

	for(j = 0;j < ((stCellConn *)pCoon)->CellPoolConf.iLTCPoolCount && ((stCellConn *)pCoon)->CellPoolConf.iLTCPoolCount <= MAX_POOL_COUNT;j++)
	{	
		snprintf(bufPool + iPosPool, sizeof(bufPool), "#%d&%s&%s&%s", 
			((stCellConn *)pCoon)->CellPoolConf.stLTCPoolConf[j].iPoolId,
			(char *)((stCellConn *)pCoon)->CellPoolConf.stLTCPoolConf[j].szUrl,
			(char *)((stCellConn *)pCoon)->CellPoolConf.stLTCPoolConf[j].szUser,
			(char *)((stCellConn *)pCoon)->CellPoolConf.stLTCPoolConf[j].szPasswd
			//((stCellConn *)pCoon)->CellPoolConf.stLTCPoolConf[j].eAlgo
			);
		iPosPool += strlen(bufPool + iPosPool);
	}				

	DEBUGMSG(0,("2 szPoolList:%s\r\n", (char *)((stCellDbColumnName *)pColName)->szPoolList));
	DEBUGMSG(0,("CellAlgo:%d\r\n", ((stCellConn *)pCoon)->CellData.eCellAlgo));
	rtn =snprintf(dest, destLen, "update %s set %s='%s' where %s = \'%s\'", 
		(char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stCellDbColumnName *)pColName)->szPoolList,
		bufPool,
		(char *)((stCellDbColumnName *)pColName)->byKeyName, 
		(char *)((stCellConn *)pCoon)->CellData.szCellId);
	
	return rtn;
}

int PackCellBaseUpdateRealKHash(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;//, j = 0;
	//long iUpdateTime = 0;
	char bufBTCHash[BUFFER_LEN_512] = {0},  bufLTCHash[BUFFER_LEN_512] = {0};
	int iPosBTCHash = 0, iPosLTCHash = 0;
	//BYTE AlgoBuf[BUFFER_LEN_16] = {0};
	int iIndex = 0, iBTCIndex = 0, iBTCLoop = 0, iLTCIndex = 0, iLTCLoop = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	iBTCIndex = ((stCellConn *)pCoon)->CellStat.iBTCNcIndex;
	if (iBTCIndex < INTERVAL_30_SECOND)
	{
		if (((stCellConn *)pCoon)->CellStat.llBTCNcsend[iBTCIndex+1] != 0 && iBTCIndex != INTERVAL_30_SECOND - 1)
		{	
			iBTCLoop = INTERVAL_30_SECOND;
		}
		else
		{
			iBTCLoop = iBTCIndex;
		}
	}
	else
	{
		iBTCLoop = INTERVAL_30_SECOND;
	}
	
	for (iIndex = 0; iIndex < iBTCLoop; iIndex++)
	{
		if (iIndex == 0)
		{
			snprintf(bufBTCHash + iPosBTCHash, sizeof(bufBTCHash), "%d|%lld",	
				((stCellConn *)pCoon)->CellStat.iBTCNcIndex,
				((stCellConn *)pCoon)->CellStat.llBTCNcsend[iIndex]);
		}
		else
		{
			snprintf(bufBTCHash + iPosBTCHash, sizeof(bufBTCHash), "|%lld",	
				((stCellConn *)pCoon)->CellStat.llBTCNcsend[iIndex]);
		}
		iPosBTCHash += strlen(bufBTCHash + iPosBTCHash);
	}

	iLTCIndex = ((stCellConn *)pCoon)->CellStat.iLTCNcIndex;
	if (iLTCIndex < INTERVAL_30_SECOND)
	{
		if (((stCellConn *)pCoon)->CellStat.llLTCNcsend[iLTCIndex+1] != 0 && iLTCIndex != INTERVAL_30_SECOND - 1)
		{	
			iLTCLoop = INTERVAL_30_SECOND;
		}
		else
		{
			iLTCLoop = iLTCIndex;
		}
	}
	else
	{
		iLTCLoop = INTERVAL_30_SECOND;
	}

	for (iIndex = 0; iIndex < iLTCLoop; iIndex++)
	{
		if (iIndex == 0)
		{
			snprintf(bufLTCHash + iPosLTCHash, sizeof(bufLTCHash), "%d|%lld",
				((stCellConn *)pCoon)->CellStat.iLTCNcIndex,
				((stCellConn *)pCoon)->CellStat.llLTCNcsend[iIndex]);
		}
		else
		{
			snprintf(bufLTCHash + iPosLTCHash, sizeof(bufLTCHash), "|%lld",	
				((stCellConn *)pCoon)->CellStat.llLTCNcsend[iIndex]);
		}
		iPosLTCHash += strlen(bufLTCHash + iPosLTCHash);
	}

	DEBUGMSG(0,("2 szPoolList:%s\r\n", (char *)((stCellDbColumnName *)pColName)->szPoolList));
	DEBUGMSG(0,("CellAlgo:%d\r\n", ((stCellConn *)pCoon)->CellData.eCellAlgo));
	rtn =snprintf(dest, destLen, "update %s set %s='%s', %s='%s' where %s = \'%s\'", 
		(char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stCellDbColumnName *)pColName)->szBTC30MNcsend,
		bufBTCHash,
		(char *)((stCellDbColumnName *)pColName)->szLTC30MNcsend,
		bufLTCHash,
		(char *)((stCellDbColumnName *)pColName)->byKeyName, 
		(char *)((stCellConn *)pCoon)->CellData.szCellId);
	
	return rtn;
}

int PackCellBaseUpdateFreq(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;
	
	rtn = snprintf(dest, destLen, "update %s set %s=%u, %s=%u where %s = \'%s\'", 
			(char *)((stMyConnInfo*)pInfo)->myTableName,
			
			(char *)((stCellDbColumnName *)pColName)->byBTCFreq,
			((stCellConn *)pCoon)->CellData.iChipBTCFreq,
			(char *)((stCellDbColumnName *)pColName)->byLTCFreq,
			((stCellConn *)pCoon)->CellData.iChipLTCFreq,
			
			(char *)((stCellDbColumnName *)pColName)->byKeyName, 
			(char *)((stCellConn *)pCoon)->CellData.szCellId);

	return rtn;
}

int PackCellBaseAlter(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackCellDeleteOneRow(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	rtn = snprintf(dest, destLen, "delete from %s.%s where %s = \'%s\'", 
			(char *)((stMyConnInfo*)pInfo)->myDbName, 
			(char *)((stMyConnInfo*)pInfo)->myTableName,
			((stCellDbColumnName*)pColName)->byKeyName,
			(char *)((stCellConn *)pCoon)->CellData.szCellId);

	return rtn;
}

int PackCellBaseSelectCellMax(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0,("%s..., %s\r\n", __FUNCTION__, (char *)((stCellDbColumnName*)pColName)->byKeyName));
	
	rtn = snprintf(dest, destLen, "select max(%s) as max_id from %s.%s having max_id>=0", 
		     (char *)((stCellDbColumnName*)pColName)->byKeyName, 
		     (char *)((stMyConnInfo*)pInfo)->myDbName,
		     (char *)((stMyConnInfo*)pInfo)->myTableName
		     );

	DEBUGMSG(0,("dest:%s\r\n", dest));

	return rtn;
}

int PackCellBaseCreate(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	rtn = snprintf(dest, destLen, "create table if not exists %s.%s	\
		(%s varchar (32) not null primary key default 0,%s bigint unsigned not null default 0,	\
		%s int unsigned not null default 0,%s varchar (16) not null default 0,%s varchar (16) not null default 0,			\
		%s int unsigned not null default 0,%s tinyint unsigned not null default 0,%s bigint unsigned not null default 0,	\
		%s bigint unsigned not null default 0,%s int unsigned not null default 0,%s int unsigned not null default 0,					\
		%s bool not null default 0,%s varchar (2048) not null default 0,%s varbinary (64) default 0,%s bigint not null default 0,%s bigint not null default 0,			\
		%s bigint not null default 0,%s varchar (512) not null default 0,%s varchar (512) not null default 0)engine myisam", 
	
		(char *)((stMyConnInfo*)pInfo)->myDbName, (char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stCellDbColumnName *)pColName)->byKeyName,//%s int not null primary key default 0,
		//(char *)((stCellDbColumnName *)pColName)->byDevId,
		(char *)((stCellDbColumnName *)pColName)->byCellIp,
		(char *)((stCellDbColumnName *)pColName)->byCellPort,
		(char *)((stCellDbColumnName *)pColName)->bySwVer,
		(char *)((stCellDbColumnName *)pColName)->byHwVer,
		(char *)((stCellDbColumnName *)pColName)->byAlgo,
		(char *)((stCellDbColumnName *)pColName)->byChipCnt,
		(char *)((stCellDbColumnName *)pColName)->byChipBTCHash,	
		(char *)((stCellDbColumnName *)pColName)->byChipLTCHash,
		(char *)((stCellDbColumnName *)pColName)->byBTCFreq,
		(char *)((stCellDbColumnName *)pColName)->byLTCFreq,	
		(char *)((stCellDbColumnName *)pColName)->byOnline,	
		(char *)((stCellDbColumnName *)pColName)->szPoolList,
		(char *)((stCellDbColumnName *)pColName)->byTestResult,	//%s varbinary (64) default 0,
		(char *)((stCellDbColumnName *)pColName)->byRegTime,
		(char *)((stCellDbColumnName *)pColName)->byLoginTime,
		(char *)((stCellDbColumnName *)pColName)->byLogoutTime,
		(char *)((stCellDbColumnName *)pColName)->szBTC30MNcsend,
		(char *)((stCellDbColumnName *)pColName)->szLTC30MNcsend
		);
	
	return rtn;
}

#if 0
//向MinerStatus数据库插入数据
int PackMinerStatusInsert(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	snprintf(dest, destLen, "insert into %s.%s      \
		(%s, %s, %s, %s, %s)values(%s)", 
		(char *)((stMyConnInfo*)pInfo)->myDbName, (char *)((stMyConnInfo*)pInfo)->myTableName,
		//(char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stMinerStatusDbColumnName *)pColName)->byMinerId,
		(char *)((stMinerStatusDbColumnName *)pColName)->byElapse,
		(char *)((stMinerStatusDbColumnName *)pColName)->byCurPoolId,
		(char *)((stMinerStatusDbColumnName *)pColName)->byCurPoolStat,
		(char *)((stMinerStatusDbColumnName *)pColName)->byUpdateTime,
		
		(char *)pComm);

	DEBUGMSG(0,("dest:%s\r\n", dest));

	return rtn;
}

int PackMinerStatusSelectSingle(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackMinerStatusUpdate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackMinerStatusAlter(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}
#endif
/*int PackMinerStatusCreate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	snprintf(dest, destLen, "create table %s.%s       \
		(%s int not null auto_increment, %s int unsigned not null, %s int, %s int,  primary key (%s,%s))", 
		(char *)((stMyConnInfo*)pInfo)->myDbName, (char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stMinerStatusDbColumnName *)pColName)->byMinerId,
		(char *)((stMinerStatusDbColumnName *)pColName)->byUpdateTime,		
		(char *)((stMinerStatusDbColumnName *)pColName)->byElapse,
		(char *)((stMinerStatusDbColumnName *)pColName)->byCurPoolId,

		(char *)((stMinerStatusDbColumnName *)pColName)->byMinerId,
		(char *)((stMinerStatusDbColumnName *)pColName)->byUpdateTime	
		);

	return rtn;
}*/

//向CellStatus数据库插入数据
int PackCellStatusInsert(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	snprintf(dest, destLen, "insert into %s.%s      \
		(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)values		\
		(%s)", 
		(char *)((stMyConnInfo*)pInfo)->myDbName, (char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stCellStatusDbColumnName *)pColName)->byCellId,
		//(char *)((stCellStatusDbColumnName *)pColName)->byTestResult,
		(char *)((stCellStatusDbColumnName *)pColName)->byEntryTemp,
		(char *)((stCellStatusDbColumnName *)pColName)->byExitTemp,
		(char *)((stCellStatusDbColumnName *)pColName)->byLedStatus,
		(char *)((stCellStatusDbColumnName *)pColName)->byElapse,
		//(char *)((stCellStatusDbColumnName *)pColName)->byBtcWblost,
		(char *)((stCellStatusDbColumnName *)pColName)->byBtcNcsend,
		//(char *)((stCellStatusDbColumnName *)pColName)->byBtcHwerr,
		(char *)((stCellStatusDbColumnName *)pColName)->byBtcAccept,
		(char *)((stCellStatusDbColumnName *)pColName)->byBtcReject,
		//(char *)((stCellStatusDbColumnName *)pColName)->byBtcDiff,
		//(char *)((stCellStatusDbColumnName *)pColName)->byLtcWblost,
		(char *)((stCellStatusDbColumnName *)pColName)->byLtcNcsend,
		//(char *)((stCellStatusDbColumnName *)pColName)->byLtcHwerr,
		(char *)((stCellStatusDbColumnName *)pColName)->byLtcAccept,
		(char *)((stCellStatusDbColumnName *)pColName)->byLtcReject,
		//(char *)((stCellStatusDbColumnName *)pColName)->byLtcDiff,
		(char *)((stCellStatusDbColumnName *)pColName)->byChipsTemp,
		(char *)((stCellStatusDbColumnName *)pColName)->byUpdateTime,
		(char *)pComm);

	return rtn;
}

int PackCellStatusSelectSingle(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackCellStatusUpdate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackCellStatusAlter(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackCellStatusCreate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	snprintf(dest, destLen, "create table if not exists %s.%s      \
		(%s varbinary (32) not null,%s tinyint unsigned not null,%s tinyint unsigned not null,		\
		%s int unsigned not null,%s int unsigned not null,%s bigint unsigned,		\
		%s bigint unsigned,%s bigint unsigned,%s bigint unsigned,%s bigint unsigned,		\
		%s bigint unsigned,%s varbinary (64) not null,%s int not null,	\
		primary key (cell_id,update_time))engine myisam",
		(char *)((stMyConnInfo*)pInfo)->myDbName, (char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stCellStatusDbColumnName *)pColName)->byCellId,
		//(char *)((stCellStatusDbColumnName *)pColName)->byTestResult,	//%s varbinary (64) not null,
		(char *)((stCellStatusDbColumnName *)pColName)->byEntryTemp,
		(char *)((stCellStatusDbColumnName *)pColName)->byExitTemp,
		(char *)((stCellStatusDbColumnName *)pColName)->byLedStatus,
		(char *)((stCellStatusDbColumnName *)pColName)->byElapse,
		//(char *)((stCellStatusDbColumnName *)pColName)->byBtcWblost,
		(char *)((stCellStatusDbColumnName *)pColName)->byBtcNcsend,
		//(char *)((stCellStatusDbColumnName *)pColName)->byBtcHwerr,
		(char *)((stCellStatusDbColumnName *)pColName)->byBtcAccept,
		(char *)((stCellStatusDbColumnName *)pColName)->byBtcReject,
		//(char *)((stCellStatusDbColumnName *)pColName)->byBtcDiff,	//%s double unsigned,
		//(char *)((stCellStatusDbColumnName *)pColName)->byLtcWblost,
		(char *)((stCellStatusDbColumnName *)pColName)->byLtcNcsend,
		//(char *)((stCellStatusDbColumnName *)pColName)->byLtcHwerr,
		(char *)((stCellStatusDbColumnName *)pColName)->byLtcAccept,
		(char *)((stCellStatusDbColumnName *)pColName)->byLtcReject,
		//(char *)((stCellStatusDbColumnName *)pColName)->byLtcDiff,	//%s double unsigned,
		(char *)((stCellStatusDbColumnName *)pColName)->byChipsTemp,
		(char *)((stCellStatusDbColumnName *)pColName)->byUpdateTime
		);
	
	return rtn;
}

//向SwitchBase数据库插入数据
int PackSwitchBaseInsert(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(1, ("%s...\r\n", __FUNCTION__));

	snprintf(dest, destLen, "replace into %s.%s      \
		(%s, %s, %s)values(%s)", 
		(char *)((stMyConnInfo*)pInfo)->myDbName, (char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stSwitchBaseDbColumnName *)pColName)->bySwitchId,
		//(char *)((stSwitchBaseDbColumnName *)pColName)->byAllocPolicy,
		//(char *)((stSwitchBaseDbColumnName *)pColName)->byMaxAllocsize,
		//(char *)((stSwitchBaseDbColumnName *)pColName)->byCpm,
		(char *)((stSwitchBaseDbColumnName *)pColName)->byStatusInterval,
		//(char *)((stSwitchBaseDbColumnName *)pColName)->byWbipBase,
		//(char *)((stSwitchBaseDbColumnName *)pColName)->byWbPort,
		//(char *)((stSwitchBaseDbColumnName *)pColName)->byPoolidPri,
		(char *)((stSwitchBaseDbColumnName *)pColName)->byUpdateTime,
		
		(char *)pComm);

	printf("dest:%s\r\n", dest);

	return rtn;
}

int PackSwitchBaseSelectSingle(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackSwitchBaseUpdate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));
	DEBUGMSG(0,("333 @:%p, szPoolIdPri:%s, szSwiId:%s\r\n", gpSwitchDbInfo, (char *)gpSwitchDbInfo->DbSwi.szPoolIdPri, gpSwitchDbInfo->DbSwi.szSwiId));

	DEBUGMSG(0,("    @:%p, szPoolIdPri:%s, szSwiId:%s\r\n", pComm, (char *)((stSwitchDbInfo*)pComm)->DbSwi.szPoolIdPri, (char *)((stSwitchDbInfo*)pComm)->DbSwi.szSwiId));

	//获取时间
	((stSwitchDbInfo*)pComm)->iUpdateTime = GetTime();

	snprintf(dest, destLen, "update %s set %s='%s',%s=%ld where %s = %s",
		(char *)((stMyConnInfo*)pInfo)->myTableName,
		
		(char *)((stSwitchBaseDbColumnName *)pColName)->byPoolidPri,
		(char *)((stSwitchDbInfo*)pComm)->DbSwi.szPoolIdPri,
		(char *)((stSwitchBaseDbColumnName *)pColName)->byUpdateTime,
		((stSwitchDbInfo*)pComm)->iUpdateTime,
		(char *)((stSwitchBaseDbColumnName *)pColName)->bySwitchId,		
		(char *)((stSwitchDbInfo*)pComm)->DbSwi.szSwiId
		);

	DEBUGMSG(0,("---------------dest:%s\r\n", dest));

	return rtn;
}

int PackSwitchBaseAlter(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackSwitchBaseCreate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	//%s int unsigned not null primary key auto_increment
	snprintf(dest, destLen, "create table if not exists %s.%s       \
		(%s varchar(64) not null default 0 primary key,%s int unsigned not null default 60,	\
		%s varchar(64) default 0, %s bigint not null default 0)", 
		(char *)((stMyConnInfo*)pInfo)->myDbName, (char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stSwitchBaseDbColumnName *)pColName)->bySwitchId,
		(char *)((stSwitchBaseDbColumnName *)pColName)->byStatusInterval,
		(char *)((stSwitchBaseDbColumnName *)pColName)->byPoolidPri,
		(char *)((stSwitchBaseDbColumnName *)pColName)->byUpdateTime	
		);

	return rtn;
}

//向PoolConfig数据库插入数据
int PackPoolConfigInsert(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	snprintf(dest, destLen, "replace into %s.%s      \
		(%s, %s, %s, %s, %s)values(%s)", 
		(char *)((stMyConnInfo*)pInfo)->myDbName, (char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stPoolConfigDbColumnName *)pColName)->byPoolId,
		(char *)((stPoolConfigDbColumnName *)pColName)->byPoolUrl,
		(char *)((stPoolConfigDbColumnName *)pColName)->byPoolUser,
		(char *)((stPoolConfigDbColumnName *)pColName)->byPoolpwd,
		(char *)((stPoolConfigDbColumnName *)pColName)->byAlgo,
		
		(char *)pComm);

	DEBUGMSG(1,("dest:%s\r\n", dest));

	return rtn;
}

int PackPoolConfigSelectSingle(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackPoolConfigUpdate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackPoolConfigSelectPoolMax(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0,("%s..., %s\r\n", __FUNCTION__, (char *)((stCellDbColumnName*)pColName)->byKeyName));
	
	rtn = snprintf(dest, destLen, "select max(%s) as max_id from %s.%s having max_id>=0", 
		     (char *)((stCellDbColumnName*)pColName)->byKeyName, 
		     (char *)((stMyConnInfo*)pInfo)->myDbName,
		     (char *)((stMyConnInfo*)pInfo)->myTableName
		     );

	DEBUGMSG(0,("dest:%s\r\n", dest));

	return rtn;
}

int PackPoolConfigAlter(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackPoolDeleteOneRow(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	rtn = snprintf(dest, destLen, "delete from %s.%s where %s = '%s'", 
			(char *)((stMyConnInfo*)pInfo)->myDbName, 
			(char *)((stMyConnInfo*)pInfo)->myTableName,
			((stPoolConfigDbColumnName*)pColName)->byPoolId,
			
			(char *)pComm);

	return rtn;
}

int PackPoolConfigCreate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	snprintf(dest, destLen, "create table if not exists %s.%s       \
		(%s int unsigned not null primary key,%s varchar (64) not null,	\
		%s varchar (32) not null,%s varchar (32) not null,%s int unsigned not null)", 
		(char *)((stMyConnInfo*)pInfo)->myDbName, (char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stPoolConfigDbColumnName *)pColName)->byPoolId,
		(char *)((stPoolConfigDbColumnName *)pColName)->byPoolUrl,
		(char *)((stPoolConfigDbColumnName *)pColName)->byPoolUser,
		(char *)((stPoolConfigDbColumnName *)pColName)->byPoolpwd,
		(char *)((stPoolConfigDbColumnName *)pColName)->byAlgo
		);

	return rtn;
}

//向AlarmLog数据库插入数据
int PackAlarmLogInsert(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	DEBUGMSG(0,("1 byId@%p, byId:%s\r\n", (char *)((stAlarmLogDbColumnName *)pColName)->byId, 
		(char *)((stAlarmLogDbColumnName *)pColName)->byId));
	snprintf(dest, destLen, "replace into %s.%s      \
		(%s, %s, %s, %s, %s, %s)values(%s)", 
		(char *)((stMyConnInfo*)pInfo)->myDbName, (char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stAlarmLogDbColumnName *)pColName)->byId,
		(char *)((stAlarmLogDbColumnName *)pColName)->byModule,
		(char *)((stAlarmLogDbColumnName *)pColName)->byAlarmType,
		(char *)((stAlarmLogDbColumnName *)pColName)->byLevel,
		(char *)((stAlarmLogDbColumnName *)pColName)->byContent,
		(char *)((stAlarmLogDbColumnName *)pColName)->byUpdateTime,
		(char *)pComm);

	DEBUGMSG(0,("+++++++++++dest:%s\r\n", dest));
	
	return rtn;
}

int PackAlarmLogSelectSingle(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackAlarmLogUpdate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackAlarmLogAlter(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackAlarmLogCreate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	snprintf(dest, destLen, "create table if not exists %s.%s       \
		(%s varchar (32) not null default 0,%s int unsigned not null default 0,		\
		%s int unsigned not null default 0,%s int unsigned not null default 0,	\
		%s varchar (64) not null default 0,%s bigint not null default 0,	\
		primary key (id,module,alarm_type))engine myisam", 
		(char *)((stMyConnInfo*)pInfo)->myDbName, (char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stAlarmLogDbColumnName *)pColName)->byId,
		(char *)((stAlarmLogDbColumnName *)pColName)->byModule,
		(char *)((stAlarmLogDbColumnName *)pColName)->byAlarmType,
		(char *)((stAlarmLogDbColumnName *)pColName)->byLevel,
		(char *)((stAlarmLogDbColumnName *)pColName)->byContent,
		(char *)((stAlarmLogDbColumnName *)pColName)->byUpdateTime
		);

	return rtn;
}

//向cellid_base数据库插入数据
int PackDelCellIdBaseInsert(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	snprintf(dest, destLen, "replace into %s.%s      \
		(%s)values(%d)", 
		(char *)((stMyConnInfo*)pInfo)->myDbName, (char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stDelCellIdBaseColumnName *)pColName)->byCellId,
		((stDelIdAllocTab *)pComm)->iDelCellId);
	
	return rtn;
}

int PackDelCellIdBaseSelectSingle(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackDelCellIdBaseUpdate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackDelCellIdBaseAlter(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	return rtn;
}

int PackDelCellIdDeleteOneRow(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	rtn = snprintf(dest, destLen, "delete from %s.%s where %s = %d", 
			(char *)((stMyConnInfo*)pInfo)->myDbName, 
			(char *)((stMyConnInfo*)pInfo)->myTableName,
			((stDelCellIdBaseColumnName*)pColName)->byCellId,
			((stDelIdAllocTab *)pCoon)->iDelCellId);

	DEBUGMSG(0,("dest:%s\r\n", dest));

	return rtn;
}

int PackDelCellIdBaseCreate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	snprintf(dest, destLen, "create table if not exists %s.%s       \
		(%s int not null primary key default 0)engine myisam", 
		(char *)((stMyConnInfo*)pInfo)->myDbName, (char *)((stMyConnInfo*)pInfo)->myTableName,
		(char *)((stDelCellIdBaseColumnName *)pColName)->byCellId
		);

	return rtn;
}

int PackSelectAll(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));
	
	rtn = snprintf(dest, destLen, "select * from %s.%s", 
		     //(char *)((stMinerDbColumnName*)pColName)->byKeyName, 
		     (char *)((stMyConnInfo*)pInfo)->myDbName,
		     (char *)((stMyConnInfo*)pInfo)->myTableName
		     );

	return rtn;
}

//delete from Data;
int PackDeleteAll(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	rtn = snprintf(dest, destLen, "delete from %s.%s", (char *)((stMyConnInfo*)pInfo)->myDbName, (char *)((stMyConnInfo*)pInfo)->myTableName);

	return rtn;
}

//创建数据库
/*int PackCreateDb(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen)
{
	int rtn = 0;	

	rtn = snprintf(dest, destLen, "create database %s", (char *)((stMyConnInfo*)pInfo)->myDbName);

	return rtn;
}*/

int MysqlDoQueryResInsert(int res, MYSQL *db)
{
	int rtn = 0;
	
	if (!res) 
	{
		//里头的函数返回受表中影响的行数
		//GetLocalTimeLog();
		//ap_log_debug_log("\t\tInserted %lu rows\r\n",(unsigned long)mysql_affected_rows(db));
	} 
	else 
	{
		//分别打印出错误代码及详细信息
		//GetLocalTimeLog();
		ap_log_debug_log("\t\tInsert error %d: %s\r\n",mysql_errno(db),mysql_error(db));
		rtn = 1;
	}

	return rtn;
}

int MysqlDoQueryResSelect(int res, MYSQL *db, IF_PROC_INTLPPSTRINT_INT InitFunc)
{
	int rtn = 0;
	MYSQL_RES *res_ptr;                                 //指向检索的结果存放地址的指针
	MYSQL_ROW sqlrow;                                   //返回的记录信息	
	MYSQL_FIELD *fd;                                    //字段结构指针
	char aszflds[BUFFER_LEN_32][BUFFER_LEN_32]; 		//用来存放各字段名	
	int i, j;

	if (!res) 
	{
		res_ptr = mysql_store_result(db);
		if(res_ptr)
		{
			DEBUGMSG(1,("Retrieved %lu Rows\r\n",(unsigned long)mysql_num_rows(res_ptr)));
			//取得各字段名
			for(i = 0; (fd = mysql_fetch_field(res_ptr)); i++)
				strcpy(aszflds[i], fd->name);
			//输出各条记录
			DEBUGMSG(1,("Here is each record retrieved information:\r\n"));
			j = mysql_num_fields(res_ptr);
			
			for(i = 0; i < j; i++)
				DEBUGMSG(1,("%s\t",aszflds[i]));
			DEBUGMSG(1,("\r\n"));
	
			while((sqlrow = mysql_fetch_row(res_ptr)))
			{
				//for (i = 0; i < dim(gConnIniDb); i++)
				//{
					//if (gConnIniDb[i].pMySql == db)
					//{
					//if (gConnIniDb[i].InitDbToDevFunc(j, sqlrow, 0) != 0)
				
				if (InitFunc(j, sqlrow, 0) != 0)
				{
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, sqlrow[0]:%s error!\r\n",  __FILE__, __FUNCTION__, __LINE__, sqlrow[0]);				
					continue;
				}	
					//}
				//}
				//if (InitDbToMiner(j, sqlrow, 0) != 0)
			
				//for(i = 0;i < j;i++)
				//	printf("%s\t",sqlrow[i]);
				//printf("\r\n");
			}
			if (mysql_errno(db))
			{
				fprintf(stderr,"Retrive error:%s\r\n",mysql_error(db));
			}
		}
		mysql_free_result(res_ptr);
	
	} 
	else 
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\tSelected error %d: %s\r\n",mysql_errno(db),mysql_error(db));
		rtn = 1;
	}
	
	return rtn;
}

int MysqlDoQueryResUpdate(int res, MYSQL *db)
{
	int rtn = 0;

	if (!res) 
	{
		//GetLocalTimeLog();
		//ap_log_debug_log("\t\tUpdated %lu rows\r\n",(unsigned long)mysql_affected_rows(db));
	} 
	else 
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\tUpdate error %d: %s\r\n",mysql_errno(db),mysql_error(db));
		rtn = 1;
	}

	return rtn;
}

int MysqlDoQueryResAlter(int res, MYSQL *db)
{
	int rtn = 0;

	if (!res) 
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\tAltered %lu rows\r\n",(unsigned long)mysql_affected_rows(db));
	} 
	else 
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\tAlter error %d: %s\r\n",mysql_errno(db),mysql_error(db));
		rtn = 1;
	}

	return rtn;
}

int MysqlDoQueryResDelete(int res, MYSQL *db)
{
	int rtn = 0;
	
	if (!res) 
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\tDeleted %lu rows\r\n",(unsigned long)mysql_affected_rows(db));
	} 
	else 
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\tDelete error %d: %s\r\n",mysql_errno(db),mysql_error(db));
		rtn = 1;
	}

	return rtn;
}

int MysqlDoQueryResCreate(int res, MYSQL *db)
{
	int rtn = 0;

	if (!res) 
	{
		//GetLocalTimeLog();
		//ap_log_debug_log("\t\tUpdated %lu rows\r\n",(unsigned long)mysql_affected_rows(db));
	} 
	else 
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\tCreate error %d: %s\r\n",mysql_errno(db),mysql_error(db));
		rtn = 1;
	}

	return rtn;
}


//发送mysql
int MysqlDoQuery(MYSQL *db, const char *pQueInfo, E_QUERY_TYPE eMysqlQuery, IF_PROC_INTLPPSTRINT_INT InitFunc)
{
	int rtn = 0;
	int res;

	DEBUGMSG(0,("%s..., eMysqlQuery:%d\r\n", __FUNCTION__, eMysqlQuery));
	res = mysql_query(db, pQueInfo);

	switch(eMysqlQuery)
	{
	case E_QUERY_TYPE_INSERT:
		rtn = MysqlDoQueryResInsert(res, db);
		break;

	case E_QUERY_TYPE_SELECT:
		rtn = MysqlDoQueryResSelect(res, db, InitFunc);
		break;

	case E_QUERY_TYPE_UPDATE:
		rtn = MysqlDoQueryResUpdate(res, db);
		break;

	case E_QUERY_TYPE_ALTER:
		rtn = MysqlDoQueryResAlter(res, db);
		break;

	case E_QUERY_TYPE_DELETE:
		rtn = MysqlDoQueryResDelete(res, db);
		break;

	case E_QUERY_TYPE_CREATETABLE:
		rtn = MysqlDoQueryResCreate(res, db);
		break;

	default:
		rtn = -1;
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, dbDoQuery error!\r\n",  __FILE__, __FUNCTION__, __LINE__);	
		break;
	}

	return rtn;
}

stMysql MysqlFunc = {
	.dbConnect = MysqlConnect,
	.dbDisconnect = MysqlDisconnect,
	.dbDoQuery = MysqlDoQuery,
	.dbReconnect = MysqlReconnect
};


void SwitchBaseOp()
{
	char bufDest[BUFFER_LEN_512] = {0}, bufSrc[BUFFER_LEN_128] = {0};
	//struct sockaddr_in adr_inet;
	BOOL bOne = TRUE, bModify = FALSE;

	//printf("%s...\r\n", __FUNCTION__);

	//目前只插入一次
	if (gpSwitchDbInfo != NULL && bOne)
	{
		//在SwiId为零的时候改变
		if (strlen((char *)gpSwitchDbInfo->DbSwi.szSwiId) == 0)
		{
			if (strncmp((char *)gpSwitchDbInfo->DbSwi.szSwiId, (char *)gCfgDataDL.SwitchInfo.szSwiId, sizeof(gpSwitchDbInfo->DbSwi.szSwiId)) != 0)
			{
				strncpy((char *)gpSwitchDbInfo->DbSwi.szSwiId, (char *)gCfgDataDL.SwitchInfo.szSwiId, sizeof(gpSwitchDbInfo->DbSwi.szSwiId));
				bModify = TRUE; 
			}
		}

		
		if (gpSwitchDbInfo->DbSwi.iStatsInter != gCfgDataDL.SwitchInfo.iStatsInter)
		{
			gpSwitchDbInfo->DbSwi.iStatsInter = gCfgDataDL.SwitchInfo.iStatsInter;
			bModify = TRUE;
		}

		//memset(&adr_inet,0,sizeof adr_inet);
		//inet_aton((char *)gpSwitchDbInfo->DbWbIp.byWbIp, &adr_inet.sin_addr);

		if (bModify)
		{
			//获取时间
			gpSwitchDbInfo->iUpdateTime = GetTime();
			
			snprintf(bufSrc, sizeof(bufSrc), "\'%s\',%d,%ld", 
				gpSwitchDbInfo->DbSwi.szSwiId, 
				//gpSwitchDbInfo->DbSwi.iNum, 
				//gpSwitchDbInfo->DbSwi.iCellNum,
				gpSwitchDbInfo->DbSwi.iStatsInter,
				//(unsigned long)ntohl(adr_inet.sin_addr.s_addr),
				//htons(gpSwitchDbInfo->DbWbIp.byWbPort),
				//gpSwitchDbInfo->DbSwi.szPoolIdPri,
				gpSwitchDbInfo->iUpdateTime);
		
			SwitchBaseQuery[E_QUERY_TYPE_INSERT-1].QueryPackFunc1(
								bufSrc, 
								&gCfgDataDL.SwitchBaseInfo, 
								&gCfgDataDL.SwitchBaseColName, 
								bufDest, 
								sizeof(bufDest));
			pthread_mutex_lock(&gSwitchBaseLock);
			MysqlFunc.dbDoQuery(&mySwitchBase, bufDest, E_QUERY_TYPE_INSERT, NULL);
			pthread_mutex_unlock(&gSwitchBaseLock);
		}

		bOne = FALSE;
		
		//printf("---------------bufSrc:%s\r\n", bufSrc);

		//free(gpSwitchDbInfo);
		//gpSwitchDbInfo = NULL;
	}
	//memset(bufDest, 0, sizeof(bufDest));	
}

/*void MinerDbOp()
{
	int iMinerListLen = 0;
	structUthash *pMinerList = NULL;
	devlist_ist *pMinerNode = NULL, *tmp = NULL;
	char bufDest[BUFFER_LEN_512] = {0};
	int iQueryRtn = 0;
	//int i = 0;

	pMinerList = &stMinerUtlist;
	pthread_mutex_lock(pMinerList->ListLenLock);
	iMinerListLen = pMinerList->CountUsersInt(*pMinerList->HashUsers);
	pthread_mutex_unlock(pMinerList->ListLenLock);
	
	//如果链表为空则返回
	if (*pMinerList->HashUsers == NULL || iMinerListLen == 0)
	{
		//MSleep(10);
		return ;
	}
	else
	{
		//pthread_mutex_lock(pMinerList->DatabaseRemLock);
		//for (i = 0; i < iMinerListLen; i++)
		HASH_ITER(hh, *pMinerList->HashUsers, pMinerNode, tmp)
		{
			//pMinerNode = pMinerList->ListAt(pMinerList->List, i);
			//printf("1 -----eMinerMySql:%d\r\n", ((stMinerConn *)pMinerNode->val)->eMinerMySql);
			//if (((stMinerConn *)pMinerNode->val)->MinerData.iMinerId == 33)
			//	printf("val->eMinerMySql:%d, @:%p\r\n", ((stMinerConn *)pMinerNode->val)->eMinerMySql, &((stMinerConn *)pMinerNode->val)->eMinerMySql);
			if (((stMinerConn *)pMinerNode->val)->eMinerMySql)
			{
				//printf("2 -----eMinerMySql:%d\r\n", ((stMinerConn *)pMinerNode->val)->eMinerMySql);
				MinerSqlQuery[((stMinerConn *)pMinerNode->val)->eMinerMySql - 1].QueryPackFunc1(
								pMinerNode->val, 
								&gCfgDataDL.MinerBaseInfo, 
								&gCfgDataDL.MinerColName, 
								bufDest, 
								sizeof(bufDest));
				iQueryRtn =MysqlFunc.dbDoQuery(&myMinerBase, bufDest, ((stMinerConn *)pMinerNode->val)->eMinerMySql, NULL);
				memset(bufDest, 0, sizeof(bufDest));
				if (iQueryRtn == 0)
					((stMinerConn *)pMinerNode->val)->eMinerMySql = 0;
			}
		}
		//pthread_mutex_unlock(pMinerList->DatabaseRemLock);
		//dbDisconnect(&MinerSqlQuery);
	}
}*/

void CellDbOp()
{
	int iCellListLen = 0;
	structUthash *pCellList = NULL;
	//int i = 0;
	devlist_st *pCellNode = NULL, *tmp = NULL;
	char bufDest[BUFFER_LEN_512] = {0};
	int iQueryRtn = 0;
	
	pCellList = &stCellUtlist;
	pthread_mutex_lock(((structList *)pCellList)->ListLenLock);
	iCellListLen = pCellList->CountUsersStr(*pCellList->StrHUsers);
	pthread_mutex_unlock(((structList *)pCellList)->ListLenLock);

	DEBUGMSG(0,("iCellListLen:%d\r\n", iCellListLen));

	//如果链表为空则返回
	if (*pCellList->StrHUsers == NULL || iCellListLen == 0)
	{
		//MSleep(10);
		return ;
	}
	else
	{
		pthread_mutex_lock(pCellList->DatabaseRemLock);
		//for (i = 0; i < iCellListLen; i++)
		HASH_ITER(hh, *pCellList->StrHUsers, pCellNode, tmp)
		{
			//pCellNode = pCellList->ListAt(pCellList->List, i);

			if (((stCellConn *)pCellNode->val)->eCellMySql)
			{
				//printf("+++++++++++++++++++++++++++++++==eCellMySql:%d\r\n", ((stCellConn *)pCellNode->val)->eCellMySql);
				CellSqlQuery[((stCellConn *)pCellNode->val)->eCellMySql - 1].QueryPackFunc1(
																				pCellNode->val, 
																				&gCfgDataDL.CellBaseInfo, 
																				&gCfgDataDL.CellColName, 
																				bufDest, 
																				sizeof(bufDest));

				pthread_mutex_lock(&gCellBaseLock);
				iQueryRtn = MysqlFunc.dbDoQuery(&myCellBase, bufDest, ((stCellConn *)pCellNode->val)->eCellMySql, NULL);
				pthread_mutex_unlock(&gCellBaseLock);
				memset(bufDest, 0, sizeof(bufDest));
				if (iQueryRtn == 0)
					((stCellConn *)pCellNode->val)->eCellMySql = 0;
			}
		}
		pthread_mutex_unlock(pCellList->DatabaseRemLock);
	}
}

void DelCellIdBaseOp()
{
	int iListLen = 0, i = 0, iQueryRtn = 0;
	structList *pDelCellIdList = NULL;
	list_node_t *pNode = NULL;
	char bufDest[BUFFER_LEN_512] = {0};

	pDelCellIdList = &stDelCellIdList;
	pthread_mutex_lock(pDelCellIdList->ListLenLock);
	iListLen = pDelCellIdList->List->len;
	pthread_mutex_unlock(pDelCellIdList->ListLenLock);
	
	if (pDelCellIdList->List == NULL || iListLen == 0)
	{
		//MSleep(10);
		return ;
	}
	else
	{	

		//pthread_mutex_lock(pDelCellIdList->DatabaseRemLock);
		//HASH_ITER(hh, *pDelCellIdList->HashUsers, pNode, tmp)
		for (i = 0; i < iListLen; i++)
		{
			pNode = stDelCellIdList.ListAt(stDelCellIdList.List, i);
			if (pNode == NULL)
				continue;
			else
			{
				if (((stDelIdAllocTab *)pNode->val)->eDelCellIdMySql)
				{
			
					DelCellIdBaseQuery[((stDelIdAllocTab *)pNode->val)->eDelCellIdMySql - 1].QueryPackFunc1(
														(stDelIdAllocTab *)pNode->val, 
														&gCfgDataDL.DelCellIdBaseInfo, 
														&gCfgDataDL.DelCellIdBaColName, 
														bufDest, 
														sizeof(bufDest));
					iQueryRtn = MysqlFunc.dbDoQuery(&myDelCellIdBase, bufDest, ((stDelIdAllocTab *)pNode->val)->eDelCellIdMySql, NULL);
					if (iQueryRtn == 0)
						((stDelIdAllocTab *)pNode->val)->eDelCellIdMySql = 0;
				}
			}
		}		
		memset(bufDest, 0, sizeof(bufDest));
		//pthread_mutex_unlock(pDelCellIdList->DatabaseRemLock);
	}
}

void *MysqlProcess(void *arg)
{
	DEBUGMSG(1,("7 %s...\r\n", __FUNCTION__));
	int i = 0;
	int mySqlError = 0;
	
	while(1)
	{
		MSleep(1000);
		if (bIniSwitchBase)
		{
			SwitchBaseOp();
		}
		/*if (bIniMinerBase)
		{
			MinerDbOp();
		}*/
		if (bIniCellBase)
		{
			CellDbOp();
		}
		//MSleep(19000);
		MSleep(40000);

		//重新连接
		for (i = 0; i < dim(gConnIniDb); i++)
		{
			DEBUGMSG(0,("i:%d, myDbName:%s, myTableName:%s\r\n", i, gConnIniDb[i].pConnInfo->myDbName, 
				gConnIniDb[i].pConnInfo->myTableName));

			pthread_mutex_lock(gConnIniDb[i].pMySqlLock);
			mySqlError = MysqlFunc.dbReconnect(gConnIniDb[i].pMySql, gConnIniDb[i].pConnInfo);
			pthread_mutex_unlock(gConnIniDb[i].pMySqlLock);

			if (mySqlError == 0)
			{
				*gConnIniDb[i].pbInitOK = TRUE;
			}	
			else
			{
				*gConnIniDb[i].pbInitOK = FALSE;
			}
		}


		/*if (bIniDelCellIdBase)
		{
			DelCellIdBaseOp();
		}*/		
	}
}

