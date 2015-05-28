/***********************************************************************
* $Id$
* Project:	 switch_network
* File:		 api_comm/api_shell_terminal.c		 
* Description: brief Part of shell terminal api. 
*
* @version:		V1.0.0
* @date:		22th September 2014
*
* Copyright (C) 2014 SFARDS Co., Ltd. All rights reserved.
*-----------------------------------------------------------------
*
* Revise:  
* 		 date		 version 		author 		description
*  		2014/09/22	 1.0.0		zhengchao	create shell communication
*
*
*
**********************************************************************/
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <assert.h>
#include "api_shell_terminal.h"
#include "../app/debug.h"
#include "../ap_log.h"
#include "../ap_utils.h"
#include "../api_net/connection_pool/connection_pool.h"
#include "../api_net/connection_pool/conn_pool_internals.h"
#include "../api_net/api_net.h"
#include "../list/uthash/my_uthash.h"
#include "../fifo_queue/circular_queue.h"
//#include "../api_comm/api_assign_resource.h"
#include "../db_op/db_mysql.h"
#include "./api_command.h"

pthread_mutex_t gCommShellFlagLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCommShellUserSeqLock = PTHREAD_MUTEX_INITIALIZER;

struct ap_net_connection_t *control_conns[MAX_CLIENTS]; /* direct links to clients control channels */
struct ap_net_conn_pool_t *tcp_pool;					/* Those pool will be used for client-server mesaging tests */

server_userdata gUserData;

const int tcp_port = 33000;
const char *control_conn_marker = "sf";
const char *help_message = 
	"\r\n============SWITCH HELP INFORMATION============\r\n"
	"1'get_switch_info': Getting switch information\r\r\n"
	//"2.'get_miner <start_id><end_id>':  Getting miner list.\r\r\n"
	"2.'get_cell <start_id><end_id>':  Getting cell list.\r\n"
	//"3.'set_cpm <id><wb_ip><wb_port>':  Miner load cell count.\r\r\n"
	//"4.'add_miner <id>':  Adding miner.\r\r\n"
	//"5.'del_miner <id>':  Unregister a miner from switch.\r\r\n"
	"3.'del_cell <id>':  unregister a cell from switch.\r\n"
	//"4.'adj_cpm':  After you remove the cell, adjust the load cell number corresponding miner.\r\r\n"
	//"8.'reset':  Resource reallocation.\r\r\n"
	/*"4.'comm<type><id><cmd_str>':  Device Control Commands.(type:cell id:CellId/all)\r\n"
	//"  'comm miner <id> set_algo ltc/btc'\r\r\n"
	"  'comm cell <id> reboot all/mcu/power/bd1/bd2/bd3'\r\n"
	"  'comm cell <id> get_temperature'\r\n"
	"  'comm cell <id> add_miner wb_ip wb_port start end total'\r\n"
	"  'comm cell <id> iddel_miner wb_ip wb_port start end total'\r\n"
	"  'comm cell <id> set_led on/off/flash/normal'\r\n"
	"  'comm cell <id> update_sw tftp_ip filename'\r\n"
	"  'comm cell <id> start_test'\r\n"
	"  'comm cell <id> get_test_result'\r\n"
	"  'comm cell <id> mcu custom_string'\r\n"*/
	"4.'reboot_cell <id/all> all/mcu/fpga/bd1/bd2/bd3':  Set cell reboot.\r\n"
	"5.'set_cell_led <id> on/off/flash/normal':  Set cell led.\r\n"
	"6.'start_cell_test <id>':  Open cell self-test.\r\n"
	"7.'cell_relogin <id/all>':  Set cell login again.\r\n"
	//"13.'miner_relogin <id>':  Set miner login again.\r\n"
	"8.'get_cellid 0 0':Getting all cellid list.\r\n"
	"9.'get_delcellid 0 0':Getting all delcellid list.\r\n"
	"10.'get_pool':Get pool configuration information.\r\n"
	"11.'setfreq <cell_id> <cell_btc_freq> <cell_ltc_freq>':Set the frequency.\r\n"
	//"12.'add_pool <pool_id> <pool_url> <pool_user> <pool_pwd> <algo>':Add a pool configuration.\r\n"
	//"13.'del_pool <pool_id> <algo>':Delete a pool configuration.\r\n"
	//"14.'set_pool <btc,pool_id1,pool_id2...> <ltc,pool_id1,pool_id2...>':Delete a pool configuration.\r\n"
	//"15.'set_cell <start_id><end_id>':Set cell pool.\r\n"
	"12.'restore <id/all>':Empty flash chip.\r\n"
	"13.'get_Dpool':Get default pool configuration information.\r\n"
	"14.'get_Spool':Get Specify pool configuration information.\r\n"
	"===============================================\r\n";

int help_message_len;

stUserCmd gUserCmd[] =
{
	{"help", PackHelp},
	{"get_switch_info", PackSwitchInfo},
	//{"get_miner", PackGetMiner},
	{"get_cell", PackGetCell},
	//{"set_cpm"},
	//{"add_miner", NULL},
	//{"del_miner", PackDelMiner},
	{"del_cell", PackDelCell},
	//{"adj_cpm", PackAdjCpm},
	//{"reset", NULL},
	{"comm", PackComm},
	{"reboot_cell", PackCellRebootToComm},
	{"set_cell_led", PackCellLedToComm},
	{"start_cell_test", PackStartCellTestToComm},
	{"cell_relogin", PackCellReloginToComm},
	//{"miner_relogin", PackMinerReloginToComm},
	{"get_cellid", PackGetAllCellId},
	{"get_delcellid", PackGetAllDelCellId},
	{"get_pool", PackGetPoolConf},
	{"setfreq", PackCellSetFreqToComm},
	//{"add_pool", PackAddPool},
	//{"del_pool", PackDelPool},
	//{"set_pool", PackSetPool},
	//{"set_cell", PackSetCell},
	{"restore", PackCellRestFlashToComm},
	{"get_Spool", PackGetPooSpecifyConf},
	{"get_Dpool", PackGetDefaultPoolConf}
};

stCommandErr gGetOrSetErr[] = 
{
	{"command parameter too few!", NULL},
	{"command parameters too many !", NULL},
	{"is empty!", NULL},
	{"starting id does not exist!", NULL},
	//{"starting position is out of range!", NULL},
	{"ending id does not exist!", NULL},
	//{"ending position is out of range!", NULL},
	{"starting id can not be less than the ending id!", NULL},
	{"parameters is error.(g(s)et_[cell/cellid/delcellid] 0 0)", NULL}
};

stCommandErr gDelErr[] =
{
	{"Delete command parameter too few!", NULL},
	{"Delete command parameters too many!", NULL},
	{"is empty!", NULL},	
	{"Shell layer, ID does not exist!", NULL},
	{"The current workbase index is error!", NULL},
	{"Shell layer, no such algorithm!", NULL},
	{"The current  conncell index is error!", NULL},
	{"Delete Miner flag error!", NULL},
	{"When removed, the database operation fails!", NULL},
	{"Database connection failed!", NULL}
};

stCommandErr gPoolConfErr[] = 
{
	{",pool_id already exists!", NULL},
	{",pool_id can not be zero!",NULL},
	{",algorithm does not exist!(algo:BTC/LTC)", NULL},
	{",this algorithms have not this id!", NULL},
	{",nothing has set up a temporary dual!", NULL},
	{",the second stage parse error!"},
	{",too many of the algorithms behind pool_id(BTC<=3&BTC<=3)"},
	{",too few of the algorithms behind pool_id(BTC>=1&LTC>=1)"}
};

stCommandErr gUserComdErr[] = 
{
	{"Connection Timeout!", PackErrorTimeOut},
	{"Command does not exist!", NULL},
	{"MinerList", PackErrorGetMiner},
	{"CellList", PackErrorGetCell},
	{"DelMiner", PackErrorDelMiner},
	{"DelCell", PackErrorDelCell},
	{"AdjCpm", PackErrorAdjCpm},
	{"SetComm", PackErrorSetComm},
	{"CellIdList", PackErrorGetCellId},
	{"add_pool", PackErrorAddPool},
	{"del_pool", PackErrorDelPool},
	{"set_pool", PackErrorSetPool},
	{"set_cell", PackErrorSetCell}	
};

stType SetPoolPara[] = 
{
	{"BTC"},
	{"LTC"},
	{"cellid"}
};

stUserSendBuf gUserSendBuf;
BYTE byUserCmdErrbuf[BUFFER_LEN_256] = {0};

void InitShell()
{
	CommShellFlag.CommShellFlagLock = &gCommShellFlagLock;
	CommShellFlag.UserSeqLock = &gCommShellUserSeqLock;
	memset(CommShellFlag.bUserFlag, 0, sizeof(CommShellFlag.bUserFlag));
	//memset(CommShellFlag.bMinerShellFlag, FALSE, sizeof(CommShellFlag.bMinerShellFlag));
	memset(CommShellFlag.bCellShellFlag, FALSE, sizeof(CommShellFlag.bCellShellFlag));
	CommShellFlag.bMassFlag = FALSE;
}

//创建到DelCellId节点中 
//此链表是保存删除CellNode时，保留CellId的链表，方便其它设备继续使用这个id。
int CreateDelCellIdBase(int iRemid)
{
	int rtn = 0, iQueryRtn = 0;
	int iDelCellIdTemp = 0;
	structList *pList = NULL;
	char bufDest[BUFFER_LEN_512] = {0};

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	pList = &stDelCellIdList;
	iDelCellIdTemp = iRemid;

	//查找DelDevId	
	pthread_mutex_lock(pList->RemoveLock);
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
		//有新结点，通知数据库插入数据
		//val->eDelCellIdMySql = E_QUERY_TYPE_INSERT;

		//直接插入数据库
		if (bIniDelCellIdBase)
		{	
			DelCellIdBaseQuery[E_QUERY_TYPE_INSERT - 1].QueryPackFunc1(
														val, 
														&gCfgDataDL.DelCellIdBaseInfo, 
														&gCfgDataDL.DelCellIdBaColName, 
														bufDest, 
														sizeof(bufDest));
			iQueryRtn = MysqlFunc.dbDoQuery(&myDelCellIdBase, bufDest, E_QUERY_TYPE_INSERT, NULL);
			memset(bufDest, 0, sizeof(bufDest));
			if (iQueryRtn == 0)	
			{
				//无重复DelDevId，插入链表
				list_node_t *node = pList->ListNodeNew(val);
				assert(node->val == val);
				pList->ListRpush(pList->List, node, pList->ListLenLock);
				rtn = 0;
			}
			else
			{
				rtn = E_DEL_ERROR_DB_OP;
				free(val);
			}			
		}
		else
		{
			rtn = E_DEL_ERROR_DB_CONNECT;
			free(val);
		}
	}
	pthread_mutex_unlock(pList->RemoveLock);

	return rtn;
}


//减少当前算法长度
/*int ReduCrrentAlgoLen(E_ALGO eAlgo, stAssiAlgoRes *pDevAssi)
{
	int rtn = -1;
	
	switch(eAlgo)
	{
	case E_ALGO_BTC:
		(*pDevAssi[E_ALGO_BTC].pCrrAlgoLen)--;
		(*pDevAssi[E_ALGO_DUAL].pCrrAlgoLen)--;
		rtn = 0;
		break;
			
	case E_ALGO_LTC:
		(*pDevAssi[E_ALGO_LTC].pCrrAlgoLen)--;
		(*pDevAssi[E_ALGO_DUAL].pCrrAlgoLen)--;
		rtn = 0;
		break;

	case E_ALGO_DUAL:
		(*pDevAssi[E_ALGO_BTC].pCrrAlgoLen)--;
		(*pDevAssi[E_ALGO_LTC].pCrrAlgoLen)--;
		(*pDevAssi[E_ALGO_DUAL].pCrrAlgoLen) += 2;//(*pDevAssi[E_ALGO_BTC].pCrrAlgoLen) + (*pDevAssi[E_ALGO_LTC].pCrrAlgoLen);
		rtn = 0;
		break;

	default:
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Algorithm[%d] does not exist!\r\n", __FILE__, __FUNCTION__, __LINE__, eAlgo);		
		break;
	}

	return rtn;
}*/

#if 0
int ReleaseMinerRes(devlist_ist *pDevNode, int iNum, stResource *pRes, stResLock *pResLock)
{
	int rtn = -1, i = 0, iIndex = 0;
	E_ALGO eAlgo;

	eAlgo = ((stMinerConn *)pDevNode->val)->MinerData.eMinerAlgo;
	switch(eAlgo)
	{
	case E_ALGO_BTC:	
	case E_ALGO_LTC:
		iIndex = ((stMinerConn *)pDevNode->val)->pMinerRes->iWbIndex;
		if (iIndex < iNum)
		{
			DEBUGMSG(1,("iIndex:%d, iNum:%d\r\n", iIndex, iNum));
			pthread_mutex_lock(pResLock->WbNoFlagLock);
			pRes[iIndex].bWbNoFlag = FALSE;
			pthread_mutex_unlock(pResLock->WbNoFlagLock);			
		}
		else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, WbIndex is too big!\r\n",  __FILE__, __FUNCTION__, __LINE__);
			return (rtn = E_DEL_ERROR_WB_INDEX);
		}
		break;

	case E_ALGO_DUAL:
		for (i = 0; i < BL_CONT_SPEC; i++)
		{
			iIndex = ((stMinerConn *)pDevNode->val)->pMinerRes[i].iWbIndex;
			if (iIndex < iNum)
			{
				DEBUGMSG(1,("iIndex:%d, iNum:%d\r\n", iIndex, iNum));
				pthread_mutex_lock(pResLock->WbNoFlagLock);
				pRes[iIndex].bWbNoFlag = FALSE;
				pthread_mutex_unlock(pResLock->WbNoFlagLock);			
			}
			else
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, WbIndex is too big!\r\n",  __FILE__, __FUNCTION__, __LINE__);
				return (rtn = E_DEL_ERROR_WB_INDEX);
			}
		}
		break;

	default:
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Algorithm[%d] does not exist!\r\n", __FILE__, __FUNCTION__, __LINE__, eAlgo);		
		break;
	}

	return rtn;
}
#endif

#if 0
int ReleaseCellRes(devlist_ist *pDevNode, int iNum, stResource *pRes, stResLock *pResLock)
{
	int rtn = -1, i = 0, iIndexI = 0, iIndexJ = 0;
	E_ALGO eAlgo;

	eAlgo = ((stCellConn *)pDevNode->val)->CellData.eCellAlgo;
	DEBUGMSG(1,("%s..., eAlgo:%d\r\n", __FUNCTION__, eAlgo));
	
	switch(eAlgo)
	{
	case E_ALGO_BTC:	
	case E_ALGO_LTC:
		iIndexI = ((stCellConn *)pDevNode->val)->pCellRes->iWbIndex;
		iIndexJ = ((stCellConn *)pDevNode->val)->pCellRes->iConnCellIndex;
		
		if (iIndexI < iNum)
		{
			if (iIndexJ < pRes[iIndexI].iConnCellNum)
			{		
				pthread_mutex_lock(gResourceLock.CellNoFlagLock);
				gpResource[iIndexI].pbWbCellNoFlag[iIndexJ] = FALSE;
				pthread_mutex_unlock(gResourceLock.CellNoFlagLock); 		
			}
			else
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, ConncellIndex is too big!\r\n", __FILE__, __FUNCTION__, __LINE__);
				return (rtn = E_DEL_ERROR_CONNCELL_INDEX);
			}
		}
		else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, WbIndex is too big!\r\n",  __FILE__, __FUNCTION__, __LINE__);
			return (rtn = E_DEL_ERROR_WB_INDEX);
		}		
		break;

	case E_ALGO_DUAL:
		for (i = 0; i < BL_CONT_SPEC; i++)
		{
			iIndexI = ((stCellConn *)pDevNode->val)->pCellRes[i].iWbIndex;
			iIndexJ = ((stCellConn *)pDevNode->val)->pCellRes[i].iConnCellIndex;

			DEBUGMSG(1,("iIndexI:%d, iIndexJ:%d\r\n", iIndexI, iIndexJ));
			if (iIndexI < iNum)
			{
				if (iIndexJ < pRes[iIndexI].iConnCellNum)
				{		
					pthread_mutex_lock(gResourceLock.CellNoFlagLock);
					gpResource[iIndexI].pbWbCellNoFlag[iIndexJ] = FALSE;
					pthread_mutex_unlock(gResourceLock.CellNoFlagLock); 

					DEBUGMSG(1,("pbWbCellNoFlag:%d\r\n", gpResource[iIndexI].pbWbCellNoFlag[iIndexJ]));
				}
				else
				{
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, ConncellIndex is too big!\r\n", __FILE__, __FUNCTION__, __LINE__);
					return (rtn = E_DEL_ERROR_CONNCELL_INDEX);
				}
			}
			else
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, WbIndex is too big!\r\n",  __FILE__, __FUNCTION__, __LINE__);
				return (rtn = E_DEL_ERROR_WB_INDEX);
			}			
		}
		break;

	default:
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, Algorithm[%d] does not exist!\r\n", __FILE__, __FUNCTION__, __LINE__, eAlgo);		
		break;
	}

	return rtn;
}
#endif 

int ParseGetOrSet(int argc, char **argv, int iListLen, void *pList)
{	
	int iStart = 0;
	int iEnd = 0;
	devlist_st *pNode = NULL;
	//int iIdStart = 0, iIdEnd = 0;
	char szIdStart[BUFFER_LEN_32] = {0}, szIdEnd[BUFFER_LEN_32] = {0};

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	if(argc < COMMANDLINE_3)
		return E_GET_SET_ERROR_PARAM_FEW;
	if (argc > COMMANDLINE_3)
		return E_GET_SET_ERROR_PARAM_MANY;

	//iStart = strtoul(argv[1], 0, 0);
	//iEnd = strtoul(argv[2], 0, 0);
	strncpy(szIdStart, argv[1], sizeof(szIdStart));
	DEBUGMSG(1,("szIdStart:%s, argv[1]:%s\n", szIdStart, argv[1]));
	strncpy(szIdEnd, argv[2], sizeof(szIdEnd));
	DEBUGMSG(1,("szIdEnd:%s, argv[2]:%s\n", szIdEnd, argv[2]));
	
	if (iListLen == 0)
		return E_GET_SET_ERROR_EMPTY;
	if (iStart > iEnd)
		return E_GET_SET_ERROR_NO_LESS;
		
	//if (iStart < 0)
	//查找start ID
	//iIdStart = strtoul(argv[1], 0, 0);
	//pNode = ((structUthash *)pList)->FindUserInt(((structUthash *)pList)->HashUsers, iIdStart);
	pNode = ((structUthash *)pList)->FindUserStr(((structUthash *)pList)->StrHUsers, szIdStart);

	if (pNode == NULL)
		return E_GET_SET_ERROR_START_ID;
	DEBUGMSG(1,("1 @pNode:%p, val: %s\r\n", pNode, (char *)pNode->val));
	//if (iStart >= iListLen)
	//	return E_GET_ERROR_START_OUTRAGE;
	//if (iEnd < 0)
	//查找start ID
	//if (iStart != iEnd)
	if (strcmp(szIdStart, szIdEnd) != 0)
	{
		//iIdEnd = strtoul(argv[2], 0, 0);
		//strncpy(szIdEnd, argv[2], sizeof(szIdEnd));
		//pNode = ((structUthash *)pList)->FindUserInt(((structUthash *)pList)->HashUsers, iIdEnd);
		pNode = ((structUthash *)pList)->FindUserStr(((structUthash *)pList)->StrHUsers, szIdEnd);

		if (pNode == NULL)	
			return E_GET_SET_ERROR_END_ID;
	}
	DEBUGMSG(0,("2 @pNode:%p, val: %s\r\n", pNode, (char *)pNode->val));
	//if (iEnd > iListLen)
	//	return E_GET_ERROR_END_OUTRAGE;	
	
	return 0;
}

#if 0
int ParseDelMiner(int argc, char **argv)
{
	int rtn = 0, iQueryRtn = 0;
	devlist_ist *pNode;
	int i = 0, iListLen = 0, iId = 0;
	char bufDest[BUFFER_LEN_512] = {0};

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	if(argc < COMMANDLINE_2)
		return E_DEL_ERROR_PARAM_FEW;
	if (argc > COMMANDLINE_2)
		return E_DEL_ERROR_PARAM_MANY;

	pthread_mutex_lock(stMinerUtlist.ListLenLock);
	iListLen = stMinerUtlist.CountUsersInt(*stMinerUtlist.HashUsers);
	pthread_mutex_unlock(stMinerUtlist.ListLenLock);
	if (iListLen == 0)
		return E_DEL_ERROR_EMPTY;

	//获得相应的锁，才能进行删除
	pthread_mutex_lock(stMinerUtlist.LoginRemLock);	
	pthread_mutex_lock(stMinerUtlist.CommRemLock);	
	pthread_mutex_lock(stMinerUtlist.ShellRemLock);
	pthread_mutex_lock(stMinerUtlist.AlarmRemLock);
	pthread_mutex_lock(stMinerUtlist.DatabaseRemLock);
	//查找ID
	iId = strtoul(argv[1], 0, 0);
	pNode = stMinerUtlist.FindUserInt(stMinerUtlist.HashUsers, iId);
	if (pNode == NULL)
	{
		rtn = E_DEl_ERROR_ID;
	}
	else
	{
		ReleaseMinerRes(pNode, gLoginResIndex.iMinerNum, gpResource, &gResourceLock);
		
		i = ((stMinerConn *)pNode->val)->MinerData.eMinerAlgo;			
		if (i < sizeof(gMinerAssi)/sizeof(gMinerAssi[0]))
		{
			pthread_mutex_lock(gResourceLock.CrrMinerAlgoLenLock);
			ReduCrrentAlgoLen(i, gMinerAssi);
			pthread_mutex_unlock(gResourceLock.CrrMinerAlgoLenLock);
		}
		else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, MinerAlgo is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
			return (rtn = E_DEL_ERROR_ALGO);
		}

		if (bIniMinerStatus)
		{
			MinerSqlQuery[E_QUERY_TYPE_DELETE - 1].QueryPackFunc1(
								pNode->val, 
								&gCfgDataDL.MinerBaseInfo, 
								&gCfgDataDL.MinerColName, 
								bufDest, 
								sizeof(bufDest));
			DEBUGMSG(1,("+++++++++++bufDest:%s\r\n", bufDest));
			
			iQueryRtn = MysqlFunc.dbDoQuery(&myMinerBase, bufDest, E_QUERY_TYPE_DELETE, NULL);
			memset(bufDest, 0, sizeof(bufDest));

			if (iQueryRtn == 0)
				stMinerUtlist.DeleteUserInt(stMinerUtlist.HashUsers, pNode);
			else
			{
				rtn = E_DEL_ERROR_DB_OP;
			}
		}
		else
		{
			rtn = E_DEL_ERROR_DB_CONNECT;
		}

	}
	pthread_mutex_unlock(stMinerUtlist.LoginRemLock);
	pthread_mutex_unlock(stMinerUtlist.CommRemLock);
	pthread_mutex_unlock(stMinerUtlist.ShellRemLock);
	pthread_mutex_unlock(stMinerUtlist.AlarmRemLock);
	pthread_mutex_unlock(stMinerUtlist.DatabaseRemLock);
			
	return rtn;
}
#endif 

int ParseDelCell(int argc, char **argv)
{
	int rtn = 0, iQueryRtn = 0;
	devlist_st *pNode =NULL;
	//devlist_st *pNodeId = NULL;
	int iListLen = 0;//, iId = 0;//, iIdListLen = 0; //int i = 0, j = 0;
	char bufDest[BUFFER_LEN_512] = {0};
	//BYTE szDevId[BUFFER_LEN_28] = {0};

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	if(argc < COMMANDLINE_2)
		return E_DEL_ERROR_PARAM_FEW;
	if (argc > COMMANDLINE_2)
		return E_DEL_ERROR_PARAM_MANY;

	pthread_mutex_lock(stCellUtlist.ListLenLock);
	iListLen = stCellUtlist.CountUsersStr(*stCellUtlist.StrHUsers);
	pthread_mutex_unlock(stCellUtlist.ListLenLock);

	/*pthread_mutex_lock(stCellIdUtlist.ListLenLock);
	iIdListLen = stCellIdUtlist.CountUsersStr(*stCellIdUtlist.StrHUsers);
	pthread_mutex_unlock(stCellIdUtlist.ListLenLock);*/

	if (iListLen == 0)// || iIdListLen == 0)
		return E_DEL_ERROR_EMPTY;

	//获得相应的锁，才能进行删除
	pthread_mutex_lock(stCellUtlist.LoginRemLock);	
	pthread_mutex_lock(stCellUtlist.CommRemLock);	
	pthread_mutex_lock(stCellUtlist.ShellRemLock);
	pthread_mutex_lock(stCellUtlist.AlarmRemLock);
	pthread_mutex_lock(stCellUtlist.DatabaseRemLock);
	pthread_mutex_lock(stCellUtlist.MonitorRemLock);
	//查找ID	
	//iId = strtoul(argv[1], 0, 0);
	//pNode = stCellUtlist.FindUserInt(stCellUtlist.HashUsers, iId);
	pNode = stCellUtlist.FindUserStr(stCellUtlist.StrHUsers, argv[1]);
	if (pNode != NULL)
	{
/*		strncpy((char *)szDevId, (char *)((stCellConn *)pNode->val)->CellData.szDevId, sizeof(szDevId));
		pNodeId = stCellIdUtlist.FindUserStr(stCellIdUtlist.StrHUsers, (char *)szDevId);
	}
	
	if (pNodeId == NULL)
	{
		rtn = E_DEl_ERROR_ID;
	}
	else
	{*/
		//ReleaseCellRes(pNode, gLoginResIndex.iMinerNum, gpResource, &gResourceLock);

		/*i = ((stCellConn *)pNode->val)->CellData.eCellAlgo;
		if (i < sizeof(gCellAssi)/sizeof(gCellAssi[0]))
		{
			pthread_mutex_lock(gResourceLock.CrrCellAlgoLenLock);
			ReduCrrentAlgoLen(i, gCellAssi);
			pthread_mutex_unlock(gResourceLock.CrrCellAlgoLenLock);
		}
		else
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, CellAlgo is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
			return (rtn = E_DEL_ERROR_ALGO);
		}*/

#if 1	//此处要加事务操作2015-01-07，   不用加事物了，DelCellIdBase不要了2015-04-27
		if (bIniCellStatus)// && bIniDelCellIdBase)
		{	
			//删除结点并删除cell_base表中数据
			CellSqlQuery[E_QUERY_TYPE_DELETE - 1].QueryPackFunc1(
								(stCellConn *)pNode->val, 
								&gCfgDataDL.CellBaseInfo, 
								&gCfgDataDL.CellColName, 
								bufDest, 
								sizeof(bufDest));

			pthread_mutex_lock(&gCellBaseLock);
			iQueryRtn = MysqlFunc.dbDoQuery(&myCellBase, bufDest, E_QUERY_TYPE_DELETE, NULL);
			pthread_mutex_unlock(&gCellBaseLock);
			
			memset(bufDest, 0, sizeof(bufDest));
			//创建结点并插入delcellid_base表中
			//rtn = CreateDelCellIdBase(iId);
			DEBUGMSG(1,("iQueryRtn:%d, rtn:%d\r\n", iQueryRtn, rtn));
			if (iQueryRtn == 0)// && rtn == 0)
			{
				//删除CellId链表中的结点和DevId链表中的结点
				stCellUtlist.DeleteUserStr(stCellUtlist.StrHUsers, pNode);
				//stCellIdUtlist.DeleteUserStr(stCellIdUtlist.StrHUsers, pNodeId);
			}
			else
			{
				rtn = E_DEL_ERROR_DB_OP;
			}
				
		}
		else
		{
			rtn = E_DEL_ERROR_DB_CONNECT;
		}
#endif		
	}
	pthread_mutex_unlock(stCellUtlist.LoginRemLock);
	pthread_mutex_unlock(stCellUtlist.CommRemLock);
	pthread_mutex_unlock(stCellUtlist.ShellRemLock);
	pthread_mutex_unlock(stCellUtlist.AlarmRemLock);
	pthread_mutex_unlock(stCellUtlist.DatabaseRemLock);
	pthread_mutex_unlock(stCellUtlist.MonitorRemLock);

	return rtn;
}

#if 0
int ParseAdjCpm(int argc, char **argv)
{
	int iListLen = 0, i = 0;
	
	if(argc < COMMANDLINE_1)
		return E_DEL_ERROR_PARAM_FEW;
	if (argc > COMMANDLINE_1)
		return E_DEL_ERROR_PARAM_MANY;

	pthread_mutex_lock(stCellUtlist.ListLenLock);
	iListLen = stCellUtlist.CountUsersInt(*stCellUtlist.HashUsers);
	pthread_mutex_unlock(stCellUtlist.ListLenLock);
	if (iListLen == 0)
		return E_DEL_ERROR_EMPTY;

	for (i = 0; i < gLoginResIndex.iMinerNum; i++)
	{
		if (gpResource[i].bDelMinerFlag == TRUE)
		{
			
		}
	}
	
	return 0;	
}
#endif

int ParseErrorGetAll(int argc, char **argv, int iListLen, void *pList)
{
	int iId1 = 0, iId2 = 0;
	
	if(argc < COMMANDLINE_3)
		return E_GET_SET_ERROR_PARAM_FEW;
	
	if (argc > COMMANDLINE_3)
		return E_GET_SET_ERROR_PARAM_MANY;

	if (iListLen == 0)
		return E_GET_SET_ERROR_EMPTY;

	iId1 = strtoul(argv[1], 0, 0);
	iId2 = strtoul(argv[2], 0, 0);
	if (iId1 != 0 || iId2 != 0)
		return E_GET_SET_ERROR_ALL;

	return 0;
}

int ParseAddPool(int argc, char **argv, int iStartPos, void *eAlgoTemp)
{
	int rtn = 0, index = 0;
	E_ALGO eAlgo;
	stPoolConfInfo *val = NULL;
	structList *pList = NULL;
	list_node_t *node = NULL, *NodeBTCTemp = NULL, *NodeLTCTemp = NULL;
	int iPoolIdTemp = 0;

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	index = ParseAlgo(argc, argv, iStartPos+4, &eAlgo);
	if (index < 0)
	{
		return E_POOLCNF_ERROR_ALGO;
	}

	iPoolIdTemp = strtoul(argv[iStartPos], 0, 0);

	//pool_id不能为0
	if (!iPoolIdTemp)
	{
		return E_POOLCNF_ERROR_POOLID_NOZERO;
	}
	
	val = malloc(sizeof(stPoolConfInfo));
	memset(val, 0, sizeof(stPoolConfInfo));
	
	DEBUGMSG(1,("eAlgo:%d\n", eAlgo));
	switch(eAlgo)
	{
	case E_ALGO_BTC:
		val->eAlgo = eAlgo;
		*(E_ALGO*)eAlgoTemp = eAlgo;

		pList = &stLTCPoolConfList;
		//查找BTC pool_id	
		
		NodeLTCTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);

		pList = &stBTCPoolConfList;
 		//查找LTC pool_id	
 		NodeBTCTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
		
		if (NodeBTCTemp == NULL && NodeLTCTemp == NULL)
		{	
			//init
			val->iPoolId = iPoolIdTemp;
			index = 2;
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
			free(val);
			return E_POOLCNF_ERROR_POOLID;
		}
		break;

	case E_ALGO_LTC:
		val->eAlgo = eAlgo;
		*(E_ALGO*)eAlgoTemp = eAlgo;

		pList = &stBTCPoolConfList;
 		//查找BTC pool_id	
 		NodeBTCTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
		
		pList = &stLTCPoolConfList;
		//查找LTC pool_id	
		//iPoolIdTemp = strtoul(argv[iStartPos], 0, 0);
		NodeLTCTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
		if (NodeLTCTemp == NULL && NodeBTCTemp == NULL)
		{
			//init
			val->iPoolId = iPoolIdTemp;
			index = 2;
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
			free(val);
			return E_POOLCNF_ERROR_POOLID;	
		}
		break;

	case E_ALGO_DUAL:
		rtn = E_POOLCNF_ERROR_DUAL;
		free(val);
		break;

	default:
		free(val);
		break;
	}

	return rtn;
}

int ParseDelPool(int argc, char **argv, int iStartPos, void *vPoolIdTemp, int type)
{
	int rtn = 0, index = 0, i = 0;
	E_ALGO eAlgo;
	//stPoolConfInfo *val = NULL;
	structList *pList = NULL;
	list_node_t *NodeTemp = NULL;
	int iPoolIdTemp = 0;
	int j = 0, iLoop = 1;
	BOOL bMonitorLoop = FALSE;

	DEBUGMSG(1,("%s..., iStartPos:%d\r\n", __FUNCTION__, iStartPos));

	//协议被改了，没办法，暂时这么处理吧。
	//shell
	if (type == 1)
	{
		index = ParseAlgo(argc, argv, iStartPos+1, &eAlgo);
		if (index < 0)
		{
			return E_POOLCNF_ERROR_ALGO;
		}
	}
	//Monitor
	else if (type == 0) 
	{
		iLoop = 2;
		eAlgo = E_ALGO_BTC;
		bMonitorLoop = TRUE;
	}
	else
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, E_POOLCNF_ERROR_DUAL\r\n",	
			__FILE__, __FUNCTION__, __LINE__);
		return (rtn = E_POOLCNF_ERROR_DUAL);
	}

	for (j = 0; j < iLoop; j++)
	{
		if (bMonitorLoop)
			eAlgo += j;
		
		switch(eAlgo)
		{
		case E_ALGO_BTC:
			pList = &stBTCPoolConfList;
			//查找pool_id	
			iPoolIdTemp = strtoul(argv[iStartPos], 0, 0);
			DEBUGMSG(1,("E_ALGO_BTC iPoolIdTemp:%d\n", iPoolIdTemp));
			NodeTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
			if (NodeTemp != NULL)
			{		
				*(int *)vPoolIdTemp = ((stPoolConfInfo *)NodeTemp->val)->iPoolId;
				pList->ListRemove(pList->List, NodeTemp);

				//删除BTCPoolList链表中的结点时，
				//同时在gPoolDefaultConf也存在这个pool_id,则清空整个gPoolDefaultConf
				//最后清空数据库(switch_db)中表(switch_base)里面的PoolIdPri
				for (i = 0; i < gPoolDefaultConf.iBTCPoolCount && gPoolDefaultConf.iBTCPoolCount <= 3; i++)
				{
					if (gPoolDefaultConf.stBTCPoolConf[i].iPoolId == iPoolIdTemp)
					{
						memset(&gPoolDefaultConf, 0, sizeof(stPoolFixedConf));
						memset((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri, 0, sizeof(gpSwitchDbInfo->DbSwi.szPoolIdPri));
					}			
				}	
			}
			else
			{
				if (!bMonitorLoop)
					return E_POOLCNF_ERROR_POOLID_EXIST;
			}
			break;
			
		case E_ALGO_LTC:
			pList = &stLTCPoolConfList;
			//查找pool_id	
			iPoolIdTemp = strtoul(argv[iStartPos], 0, 0);
			DEBUGMSG(1,("E_ALGO_LTC iPoolIdTemp:%d\n", iPoolIdTemp));
			NodeTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
			if (NodeTemp != NULL)
			{		
				*(int *)vPoolIdTemp = ((stPoolConfInfo *)NodeTemp->val)->iPoolId;
				pList->ListRemove(pList->List, NodeTemp);

				//删除LTCPoolList链表中的结点时，
				//同时在gPoolDefaultConf也存在这个pool_id,则清空整个gPoolDefaultConf
				//最后清空数据库(switch_db)中表(switch_base)里面的PoolIdPri
				for (i = 0; i < gPoolDefaultConf.iLTCPoolCount && gPoolDefaultConf.iLTCPoolCount <= 3; i++)
				{
					if (gPoolDefaultConf.stLTCPoolConf[i].iPoolId == iPoolIdTemp)
					{
						memset(&gPoolDefaultConf, 0, sizeof(stPoolFixedConf));
						memset((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri, 0, sizeof(gpSwitchDbInfo->DbSwi.szPoolIdPri));
					}			
				}				
			}
			else
			{
				if (!bMonitorLoop)
					return E_POOLCNF_ERROR_POOLID_EXIST;
			}
			break;
			
		case E_ALGO_DUAL:
			rtn = E_POOLCNF_ERROR_DUAL;
			break;

		default:
			//free(val);
			break;
		}
		if (NodeTemp != NULL)
			break;
	}

	if ((NodeTemp == NULL && bMonitorLoop))
	{
		return E_POOLCNF_ERROR_POOLID_EXIST;
	}
	
	return rtn;
}

int ParseSetPoolAlgo(int argc, char **argv, int *pPos, void *vPoolErr)
{
	int rtn = 0, index = 0, i = 0, j = 0;
	E_ALGO eAlgo;
	int iPoolIdTemp = 0;
	structList *pList = NULL;
	list_node_t *NodeTemp = NULL;
	int iPos = 0;

	iPos = *pPos;
	//btc 或者ltc后面的pool_id 最多3个，最少1个

	DEBUGMSG(1,("%s...\n", __FUNCTION__));
	if (argc < 2) 
		return E_POOLCNF_ERROR_TOO_FEWPOOLID;
	if (argc > 4)
		return E_POOLCNF_ERROR_TOO_MANYPOOLID;

	index = ParseAlgo(argc, argv, 0, &eAlgo);

	DEBUGMSG(1,("-----------------index:%d\n", index));
	if (index < 0)
	{
		return E_POOLCNF_ERROR_ALGO;
	}

	//把算法,打包到池顺序的缓冲区中,清空
	if (iPos == 0)
	{
		memset((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri, 0, sizeof(gpSwitchDbInfo->DbSwi.szPoolIdPri));
		memset(&gPoolDefaultConf, 0, sizeof(stPoolFixedConf));
	}

	switch(eAlgo)
	{
	case E_ALGO_BTC:
		pList = &stBTCPoolConfList;
		snprintf((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri + iPos, sizeof(gpSwitchDbInfo->DbSwi.szPoolIdPri), "%s", argv[0]);
		iPos += strlen((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri + iPos);		
		for (i = 1; i < argc;i++,j++)
		{
			//查找pool_id	
			iPoolIdTemp = strtoul(argv[i], 0, 0);
			NodeTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
			if (NodeTemp != NULL)
			{
				gPoolDefaultConf.stBTCPoolConf[j].iPoolId = iPoolIdTemp;
				strcpy((char *)gPoolDefaultConf.stBTCPoolConf[j].szUrl, (char *)((stPoolConfInfo *)NodeTemp->val)->szUrl);
				strcpy((char *)gPoolDefaultConf.stBTCPoolConf[j].szUser, (char *)((stPoolConfInfo *)NodeTemp->val)->szUser);
				strcpy((char *)gPoolDefaultConf.stBTCPoolConf[j].szPasswd, (char *)((stPoolConfInfo *)NodeTemp->val)->szPasswd);
				gPoolDefaultConf.stBTCPoolConf[j].eAlgo = ((stPoolConfInfo *)NodeTemp->val)->eAlgo;

				snprintf((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri + iPos, sizeof(gpSwitchDbInfo->DbSwi.szPoolIdPri), "/%d", iPoolIdTemp);
				iPos += strlen((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri + iPos);
					
				gPoolDefaultConf.iBTCPoolCount++;
			}
			else
			{
				*pPos = 0;
				sprintf((char *)vPoolErr, "%s,%d", argv[0],iPoolIdTemp);
				memset((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri, 0, sizeof(gpSwitchDbInfo->DbSwi.szPoolIdPri));
				return E_POOLCNF_ERROR_POOLID_EXIST;
			}
		}
		break;
		
	case E_ALGO_LTC:
		pList = &stLTCPoolConfList;
		DEBUGMSG(0,("aaaa iPos:%d, sizeof:%d, szPoolIdPri:%s, argv[0]:%s\r\n", iPos, sizeof(gpSwitchDbInfo->DbSwi.szPoolIdPri), (char *)gpSwitchDbInfo->DbSwi.szPoolIdPri, argv[0]));
		snprintf((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri + iPos, sizeof(gpSwitchDbInfo->DbSwi.szPoolIdPri), " %s", argv[0]);
		iPos += strlen((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri + iPos);
		DEBUGMSG(0,("bbbb iPos:%d, szPoolIdPri:%s, argv[0]:%s\r\n", iPos, (char *)gpSwitchDbInfo->DbSwi.szPoolIdPri, argv[0]));
		for (i = 1; i < argc;i++, j++)
		{
			//查找pool_id	
			iPoolIdTemp = strtoul(argv[i], 0, 0);
			NodeTemp = pList->ListFind(pList->List, (char *)&iPoolIdTemp);
			if (NodeTemp != NULL)
			{		
				gPoolDefaultConf.stLTCPoolConf[j].iPoolId = iPoolIdTemp;
				strcpy((char *)gPoolDefaultConf.stLTCPoolConf[j].szUrl, (char *)((stPoolConfInfo *)NodeTemp->val)->szUrl);
				strcpy((char *)gPoolDefaultConf.stLTCPoolConf[j].szUser, (char *)((stPoolConfInfo *)NodeTemp->val)->szUser);
				strcpy((char *)gPoolDefaultConf.stLTCPoolConf[j].szPasswd, (char *)((stPoolConfInfo *)NodeTemp->val)->szPasswd);
				gPoolDefaultConf.stLTCPoolConf[j].eAlgo = ((stPoolConfInfo *)NodeTemp->val)->eAlgo;
				
				snprintf((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri + iPos, sizeof(gpSwitchDbInfo->DbSwi.szPoolIdPri), "/%d", iPoolIdTemp);
				iPos += strlen((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri + iPos);
				DEBUGMSG(0,("cccc iPos:%d, szPoolIdPri:%s, argv[0]:%s\r\n", iPos, (char *)gpSwitchDbInfo->DbSwi.szPoolIdPri, argv[0]));

				gPoolDefaultConf.iLTCPoolCount++;
			}
			else
			{
				*pPos = 0;
				sprintf((char *)vPoolErr, "%s,%d", argv[0],iPoolIdTemp);
				memset((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri, 0, sizeof(gpSwitchDbInfo->DbSwi.szPoolIdPri));
				return E_POOLCNF_ERROR_POOLID_EXIST;
			}
		}
		break;
		
	case E_ALGO_DUAL:
		rtn = E_POOLCNF_ERROR_DUAL;
		break;

	default:
		break;
	}

	*pPos = iPos;
		
	return rtn;
}

/*int ParseSetPoolModifyCell(int argc, char **argv, void *vCellIdTemp)
{
	int rtn = 0, i = 0, iTotListLen = 0, iListLen = 0, iErrIndex = 0, j = 0;
	structUthash *pList = NULL;
	devlist_ist *pNode = NULL;
	BYTE AlgoBuf[BUFFER_LEN_16] = {0};
	BYTE OnLineBuf[BUFFER_LEN_8] = {0};
	int iPos = 0;
	int iNumPack = 0, iRemain = 0, iLoop = 0, iIndex = 0;
	devlist_ist *SelNode = NULL;
	long iReg = 0, iLogin = 0, iLogout = 0;
	BYTE regBuf[BUFFER_LEN_64] = {0}, loginBuf[BUFFER_LEN_64] = {0}, logoutBuf[BUFFER_LEN_64] = {0};

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));



	pList = &stCellUtlist;
	pthread_mutex_lock(((structList *)pList)->ListLenLock);
	iTotListLen = ((structUthash *)pList)->CountUsersInt(*((structUthash *)pList)->HashUsers); 
	pthread_mutex_unlock(((structList *)pList)->ListLenLock);

	if ((iErrIndex = ParseGetOrSet(argc, argv, iTotListLen, pList)) > 0)
	{
		if (gUserComdErr[E_SHELL_ERROR_GETCELL].UserCmdFunc(gGetOrSetErr[iErrIndex - 1].cmd, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen) < 0)
			return (rtn = 2);
		else
			return rtn;
	}
	
	//先指定起始id和结束id
	memset(((structUthash *)pList)->SelCrrNode, 0, sizeof(selnode_st));
	((structUthash *)pList)->SelCrrNode->StartId = atoi(argv[1]);
	((structUthash *)pList)->SelCrrNode->EndId = atoi(argv[2]);	
	((structUthash *)pList)->SelCrrNode->SelCrrUsers = ((structUthash *)pList)->SelectcondInt(((structUthash *)pList)->HashUsers);
	for(SelNode = ((structUthash *)pList)->SelCrrNode->SelCrrUsers; SelNode; SelNode = (devlist_ist*)SelNode->ah.next)
	{
		((structUthash *)pList)->SelCrrNode->SelLen++;
	}
	iListLen = ((structUthash *)pList)->SelCrrNode->SelLen;	

	pNode = ((structUthash *)pList)->SelCrrNode->SelCrrUsers;
	if (pNode != NULL)
	{
		//pthread_mutex_lock(((structList *)pList)->RemoveLock);
	    for (i = 0; i < iListLen; i++)
		{
			//结点中的pool信息修改完成后，在进行打包发送结点信息
			pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.PoolConfLock);
			for(j = 0;j < gPoolDefaultConf.iBTCPoolCount && gPoolDefaultConf.iBTCPoolCount <= MAX_POOL_COUNT;j++)
			{	
				((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].iPoolId = gPoolDefaultConf.stBTCPoolConf[j].iPoolId;
				strcpy(((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].szUrl, gPoolDefaultConf.stBTCPoolConf[j].szUrl);
				strcpy(((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].szUser, gPoolDefaultConf.stBTCPoolConf[j].szUser);
				strcpy(((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].szPasswd, gPoolDefaultConf.stBTCPoolConf[j].szPasswd);
				((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].eAlgo = gPoolDefaultConf.stBTCPoolConf[j].eAlgo;
			}
			
			for(j = 0;j < gPoolDefaultConf.iLTCPoolCount && gPoolDefaultConf.iLTCPoolCount <= MAX_POOL_COUNT;j++)
			{	
				((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].iPoolId = gPoolDefaultConf.stLTCPoolConf[j].iPoolId;
				strcpy(((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].szUrl, gPoolDefaultConf.stLTCPoolConf[j].szUrl);
				strcpy(((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].szUser, gPoolDefaultConf.stLTCPoolConf[j].szUser);
				strcpy(((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].szPasswd, gPoolDefaultConf.stLTCPoolConf[j].szPasswd);
				((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].eAlgo = gPoolDefaultConf.stLTCPoolConf[j].eAlgo;
			}
			pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.PoolConfLock);
			
			iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);
			pNode = (devlist_ist*)(pNode->ah.next);
			if (pNode == NULL)
				break;		
			//printf("Cell.......iPos:%d\r\n", iPos);
		}
		((structUthash *)pList)->SelCrrNode->SelCrrUsers = pNode;		
	}
	//pthread_mutex_unlock(((structList *)pList)->RemoveLock);

	DEBUGMSG(1,("****iPos:%d\r\n", iPos));
	return rtn;
}*/

int ParseSetPool(int argc, char **argv, int iStartPos, void *vPoolErr)
{
	int rtn = 0, i = 0, k = 0;//j = 0, 
	//E_ALGO eAlgo;
	//stPoolConfInfo *val = NULL;
	//structList *pList = NULL;
	//list_node_t *NodeTemp = NULL;
	//int iPoolIdTemp = 0;
	int argcSec = 0;
	char *argvSec[BUFFER_LEN_64] = {0};
	char delim[] = ",";
	//int iAlgLen = 0, iParamLen = 0;
	int iPos = 0;

	for (i = iStartPos; i < argc; i++)
	{
		if (Parse(argv[i], &argcSec, argvSec, delim) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, secondary parse error!\r\n",	__FILE__, __FUNCTION__, __LINE__);
			return -1;
		}

		//iAlgLen = strlen((char *)SetPoolPara[j].type);
		//iParamLen = strlen(argvSec[0]);
		//if (strncasecmp((char *)SetPoolPara[j++].type, argvSec[0], iAlgLen) == 0 && iAlgLen == iParamLen)
		{
			//if (j < 3)
			//{
			rtn = ParseSetPoolAlgo(argcSec, argvSec, &iPos, vPoolErr);
			if (rtn != 0)
			{
				//memset((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri, 0, sizeof(gpSwitchDbInfo->DbSwi.szPoolIdPri));
				return rtn;
			}
			/*else
			{
				//把算法,打包到池顺序的缓冲区中
				if (i == 1)
				{
					snprintf((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri + iPos, sizeof(gpSwitchDbInfo->DbSwi.szPoolIdPri), "%s", argv[i]);
					iPos += strlen((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri + iPos);
				}
				else if (i == 2)
				{
					snprintf((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri + iPos, sizeof(gpSwitchDbInfo->DbSwi.szPoolIdPri), " %s", argv[i]);
					iPos += strlen((char *)gpSwitchDbInfo->DbSwi.szPoolIdPri + iPos);
				}
			}*/
			//}
			/*else
			{
				rtn = ParseSetPoolModifyCell(argcSec, argvSec, dest, destLen);
				if (rtn != 0)
					return rtn;
			}*/
			for (k = 0; k < argcSec; k++)
			{
				memset(argvSec+k, 0, sizeof(*argvSec));
			}
			argcSec = 0;
		}
	}

	//DEBUGMSG(0,("111 szPoolIdPri:%s, iSwiId:%d\r\n", (char *)gpSwitchDbInfo->DbSwi.szPoolIdPri, gpSwitchDbInfo->DbSwi.iSwiId));

	/*switch(argc)
	{
	case 2:
		if (Parse(argv[1], &argcBTC, argvBTC, delim) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, btc pool_id parse error!\r\n",	__FILE__, __FUNCTION__, __LINE__);
			return -1;
		}
		break;

	case 3:
		if (Parse(argv[1], &argcBTC, argvBTC, delim) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, btc pool_id parse error!\r\n",	__FILE__, __FUNCTION__, __LINE__);
			return -1;
		}
		if (Parse(argv[3], &argcLTC, argvLTC, delim) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, btc pool_id parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
			return -1;
		}			
		break;

	case 4:
		if (Parse(argv[1], &argcBTC, argvBTC, delim) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, btc pool_id parse error!\r\n",	__FILE__, __FUNCTION__, __LINE__);
			return -1;
		}
		if (Parse(argv[2], &argcLTC, argvLTC, delim) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, btc pool_id parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
			return -1;
		}	
		if (Parse(argv[3], &argcLTC, argvLTC, delim) != 0)
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, btc pool_id parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
			return -1;
		}
		break;

	default:
		break;
	}*/
	
	return rtn;
}

int PackErrorTimeOut(void *src, void *dest, int destLen)
{
	int rtn = 0;
	struct tm *ptm;
	
	DEBUGMSG(1,("%s..., destLen:%d\r\n", __FUNCTION__, destLen));

	ptm = GetLocalTime();
	rtn = snprintf((char *)dest, destLen, 
	"\r\r\n\t%d-%02d-%02d %02d:%02d:%02d\tConnection Timeout!\r\n",
	    ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

	return rtn;
}

int PackErrorGetMiner(void *src, void *dest, int destLen)
{
	return snprintf((char *)dest, destLen, "MinerList %s\r\n", (char *)src);
}

int PackErrorGetCell(void *src, void *dest, int destLen)
{
	BYTE timeBuf[BUFFER_LEN_256] = {0};
	
	GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());

	return snprintf((char *)dest, destLen, "%s\tCellList %s\r\n", (char *)timeBuf, (char *)src);
}

int PackErrorDelMiner(void *src, void *dest, int destLen)
{
	return snprintf((char *)dest, destLen, "[del_miner] %s\r\n", (char *)src);
}

int PackErrorDelCell(void *src, void *dest, int destLen)
{
	BYTE timeBuf[BUFFER_LEN_256] = {0};

	GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
	
	return snprintf((char *)dest, destLen, "%s\t[del_cell] %s\r\n", (char *)timeBuf, (char *)src);
}

int PackErrorAdjCpm(void *src, void *dest, int destLen)
{
	return snprintf((char *)dest, destLen, "[adj_cpm]No need to adjust, cell %s\r\n", (char *)src);
}

int PackErrorSetComm(void *src, void *dest, int destLen)
{
	int rtn = 0;
	struct tm *ptm;

	ptm = GetLocalTime();
	snprintf((char *)dest, destLen, 
	"[%d-%02d-%02d %02d:%02d:%02d]\t[set_comm]Number of parameters is error,or please check the end of the space!\r\n",
	    ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);

	return rtn;
}

int PackErrorGetCellId(void *src, void *dest, int destLen)
{
	BYTE timeBuf[BUFFER_LEN_256] = {0};

	GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
	
	return snprintf((char *)dest, destLen, "%s\tCellIdList %s\r\n", (char *)timeBuf, (char *)src);
}

int PackErrorAddPool(void *src, void *dest, int destLen)
{
	DEBUGMSG(0,("src:%s\r\n", (char *)src));
	return snprintf((char *)dest, destLen, "AddPool %s\r\n", (char *)src);
}

int PackErrorDelPool(void *src, void *dest, int destLen)
{
	return snprintf((char *)dest, destLen, "DelPool %s\r\n", (char *)src);
}

int PackErrorSetPool(void *src, void *dest, int destLen)
{
	return snprintf((char *)dest, destLen, "SetPool %s\r\n", (char *)src);
}

int PackErrorSetCell(void *src, void *dest, int destLen)
{
	return snprintf((char *)dest, destLen, "SetCell %s\r\n", (char *)src);
}

int PackUserLogin(void *dest, int destLen)
{
	int rtn = 0;
	struct tm *ptm;
	
	ptm = GetLocalTime(); 
	snprintf((char *)dest, destLen, 
		"\r\r\n\r\r\n\r\r\n+---------------------------------------------------------+ \r\r\n\twelcome to login switch_network!\r\r\n\t\t%d-%02d-%02d %02d:%02d:%02d\r\r\n+---------------------------------------------------------+ \r\r\n",
	    ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	
	return rtn;
}

int PackHelp(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0;
	
	DEBUGMSG(1,("%s..., destLen:%d\r\n", __FUNCTION__, destLen));

	snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen, "%s", help_message);
	DEBUGMSG(0,("UserSendLen:%d, %d, %d\r\n", strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf), sizeof(gUserSendBuf), sizeof(stUserSendBuf)));
	
	return rtn;
}

int PackSwitchInfo(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0;//i = 0;
	int iCellCount = 0, iCellIdCount = 0, iDelCellIdCount = 0;//iMinerCount = 0, 
	int iOnLineCellCount = 0;//iOnLineMinerCount = 0, 
	structUthash *pListCell = NULL, *pListCellId = NULL;//*pListMiner = NULL, 
	structList *pListDelCellId = NULL;
	BYTE timeBuf[BUFFER_LEN_256] = {0};
	devlist_st *pNode = NULL, *tmp = NULL;
	stRunTime TimeTemp;
	int iCrrMaxCellId = 0;

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	//pListMiner = &stMinerUtlist;
	pListCell = &stCellUtlist;
	pListCellId = &stCellIdUtlist;
	pListDelCellId = &stDelCellIdList;
	
	iCrrMaxCellId = gCrrMaxCellId;
	/*pthread_mutex_lock(((structUthash *)pListMiner)->ListLenLock);
	iMinerCount = ((structUthash *)pListMiner)->CountUsersInt(*((structUthash *)pListMiner)->HashUsers);
	pthread_mutex_unlock(((structUthash *)pListMiner)->ListLenLock);*/

	//DEBUGMSG(0,("iMinerCount:%d\r\n", iMinerCount));
	pthread_mutex_lock(((structUthash *)pListCell)->ListLenLock);
	iCellCount = ((structUthash *)pListCell)->CountUsersStr(*((structUthash *)pListCell)->StrHUsers);
	pthread_mutex_unlock(((structUthash *)pListCell)->ListLenLock);

	pthread_mutex_lock(pListCellId->ListLenLock);
	iCellIdCount = pListCellId->CountUsersStr(*pListCellId->StrHUsers);
	pthread_mutex_unlock(pListCellId->ListLenLock);

	pthread_mutex_lock(pListDelCellId->ListLenLock);
	iDelCellIdCount = pListDelCellId->List->len;
	pthread_mutex_unlock(pListDelCellId->ListLenLock);


	/*pthread_mutex_lock(((structUthash *)pListMiner)->ShellRemLock);
	//for (i = 0; i < iMinerCount; i++)
	DEBUGMSG(0,("@HashUsers:%p\r\n", *((structUthash *)pListMiner)->HashUsers));
	HASH_ITER(hh, *((structUthash *)pListMiner)->HashUsers, pNode, tmp)
	{
		//pNode = ((structList *)pListMiner)->ListAt(((structList *)pListMiner)->List, i);
		pthread_mutex_lock(&((stMinerConn *)pNode->val)->MinerDataLock.OnlineLock);
		if (((stMinerConn *)pNode->val)->MinerData.bMinerOnline == TRUE)
		{
			iOnLineMinerCount++;
		}
		pthread_mutex_unlock(&((stMinerConn *)pNode->val)->MinerDataLock.OnlineLock);
	}
	pthread_mutex_unlock(((structUthash *)pListMiner)->ShellRemLock);*/

	pthread_mutex_lock(((structUthash *)pListCell)->ShellRemLock);
	//for (i = 0; i < iCellCount; i++)
	HASH_ITER(hh, *((structUthash *)pListCell)->StrHUsers, pNode, tmp)
	{
		//pNode = ((structList *)pListCell)->ListAt(((structList *)pListCell)->List, i);
		pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
		if (((stCellConn *)pNode->val)->CellData.bCellOnline == TRUE)
		{
			iOnLineCellCount++;
		}
		pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
	}
	pthread_mutex_unlock(((structUthash *)pListCell)->ShellRemLock);

	TimeTemp = RunTime();
	DEBUGMSG(0,("1 here\r\n"));
	GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());	
	snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen, 
			  "%s\r\n\tcell_count:%d\r\n\tonline_cell:%d	\
			  \r\n\tcellid_count:%d\r\n\tdel_cellId_count:%d\r\n\tcrr_max_cellid:%d	\
			  \r\n\trun_time:%dD--%dH--%dM--%dS\r\n", 
			  (char *)timeBuf, iCellCount, iOnLineCellCount, 
			  iCellIdCount, iDelCellIdCount, iCrrMaxCellId, 
			  TimeTemp.iDay, TimeTemp.iHour, TimeTemp.iMinute, TimeTemp.iSecond);
	
	return rtn;
}

/*int PackGetMiner(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0, i = 0, iTotListLen = 0, iListLen = 0, iErrIndex = 0;
	void *pList = NULL;
	devlist_ist *pNode = NULL;
	BYTE AlgoBuf[BUFFER_LEN_16] = {0};
	BYTE OnLineBuf[BUFFER_LEN_8] = {0};
	int iPos = 0;
	int iNumPack = 0, iRemain = 0, iLoop = 0, iIndex = 0;
	//devlist_st *ausers = NULL;
	devlist_ist *SelNode = NULL;
	long iReg = 0, iLogin = 0, iLogout = 0;
	BYTE regBuf[BUFFER_LEN_64] = {0}, loginBuf[BUFFER_LEN_64] = {0}, logoutBuf[BUFFER_LEN_64] = {0};

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	//DEBUGMSG(1,("------- @pNode:%p, @SelCrrUsers:%p\r\n", pNode, ((structUthash *)pList)->SelCrrNode->SelCrrUsers));
	pList = &stMinerUtlist;	
	if (!((stUserSendBuf*)dest)->bSubcontFlag)
	{
		pthread_mutex_lock(((structUthash *)pList)->ListLenLock);
		iTotListLen = ((structUthash *)pList)->CountUsersInt(*((structUthash *)pList)->HashUsers);    //查看链表长度，判断是否是空链表
		pthread_mutex_unlock(((structUthash *)pList)->ListLenLock);
		if ((iErrIndex = ParseGetOrSet(argc, argv, iTotListLen, pList)) > 0)
		{
			if (gUserComdErr[E_SHELL_ERROR_GETMINER].UserCmdFunc(gGetOrSetErr[iErrIndex - 1].cmd, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen) < 0)
				return (rtn = 2);
			else
				return rtn;
		}

		//((stUserSendBuf*)dest)->iNumStart = atoi(argv[1]);
		//((stUserSendBuf*)dest)->iNumEnd = atoi(argv[2]);

		//先指定起始id和结束id
		memset(((structUthash *)pList)->SelCrrNode, 0, sizeof(selnode_st));
		((structUthash *)pList)->SelCrrNode->StartId = atoi(argv[1]);
		((structUthash *)pList)->SelCrrNode->EndId = atoi(argv[2]);
		//DEBUGMSG(1,("1 @SelCrrNode->SelCrrUsers:%p\r\n", ((structUthash *)pList)->SelCrrNode->SelCrrUsers));
		((structUthash *)pList)->SelCrrNode->SelCrrUsers = ((structUthash *)pList)->SelectcondInt(((structUthash *)pList)->HashUsers);
		for(SelNode = ((structUthash *)pList)->SelCrrNode->SelCrrUsers; SelNode; SelNode = (devlist_ist*)SelNode->ah.next)
		{
			((structUthash *)pList)->SelCrrNode->SelLen++;
		}
		iListLen = ((structUthash *)pList)->SelCrrNode->SelLen;
		iRemain = iListLen % MINER_LIST_PACK_NUM;
		iNumPack = iRemain > 0 ? iListLen / MINER_LIST_PACK_NUM+1 : iListLen / MINER_LIST_PACK_NUM;
		
		if (iNumPack > 1)	//分包
		{	
			((stUserSendBuf*)dest)->bSubcontFlag = TRUE;
			((stUserSendBuf*)dest)->iNumPack = iNumPack;
			((stUserSendBuf*)dest)->iRemain = iRemain;
		}

		//((structUthash *)pList)->SelCrrNode->SelCrrUsers = ausers;
		//DEBUGMSG(1,("2 @CrrNode:%p\r\n", ((structUthash *)pList)->SelCrrNode->SelCrrUsers));
		//for(userTemp = ausers; ((structUthash *)pList)->SelCrrNode->SelCrrUsers; ((structUthash *)pList)->SelCrrNode->SelCrrUsers = (devlist_st*)(((structUthash *)pList)->SelCrrNode->SelCrrUsers->ah.next))
		//for(userTemp = ((structUthash *)pList)->SelCrrNode->SelCrrUsers; userTemp; userTemp = (devlist_st*)userTemp->ah.next)
		//{
			//DEBUGMSG(1,("Miner @CrrNode:%p, val: %s\r\n", ((structUthash *)pList)->SelCrrNode->SelCrrUsers, (char *)((structUthash *)pList)->SelCrrNode->SelCrrUsers->val));
			//DEBUGMSG(1,("Miner @userTemp:%p, val: %s\r\n", userTemp, (char *)userTemp->val));
		//}
		//DEBUGMSG(1,("3 @CrrNode:%p\r\n", ((structUthash *)pList)->SelCrrNode->SelCrrUsers));
		//DEBUGMSG(1,("iRemain:%d, iSubindex:%d\r\n",((stUserSendBuf*)dest)->iRemain, ((stUserSendBuf*)dest)->iSubindex));
	}

	//循环打包
	if (((stUserSendBuf*)dest)->iSubindex <= ((stUserSendBuf*)dest)->iNumPack - 1)
	{
		if (((stUserSendBuf*)dest)->iSubindex == (((stUserSendBuf*)dest)->iNumPack - 1) && ((stUserSendBuf*)dest)->iRemain)
			iLoop = ((stUserSendBuf*)dest)->iRemain;
		else
			iLoop = MINER_LIST_PACK_NUM;
	}
	else
	{
		iLoop = iListLen;
	}

	DEBUGMSG(0,("iSubindex:%d\r\n", ((stUserSendBuf*)dest)->iSubindex));
	iIndex = ((stUserSendBuf*)dest)->iSubindex * MINER_LIST_PACK_NUM;//((stUserSendBuf*)dest)->iNumStart;	//分包后打过包的链表下标
	DEBUGMSG(0,("iIndex:%d, iListLen:%d, iLoop:%d\r\n", iIndex, iListLen, iLoop));
	DEBUGMSG(0,("1 @pNode:%p, @SelCrrUsers:%p\r\n", pNode, ((structUthash *)pList)->SelCrrNode->SelCrrUsers));

	pNode = ((structUthash *)pList)->SelCrrNode->SelCrrUsers;

	//pthread_mutex_lock(((structUthash *)pList)->ShellRemLock);   //暂时不加锁
	if (pNode != NULL)
	{	DEBUGMSG(0,("1 here\r\n"));
		for (i = 0; i < iLoop; i++)
		{
			AlgoEnumToString(((stMinerConn *)pNode->val)->MinerData.eMinerAlgo, (char *)AlgoBuf, sizeof(AlgoBuf));

			pthread_mutex_lock(&((stMinerConn *)pNode->val)->MinerDataLock.OnlineLock);
			if (((stMinerConn *)pNode->val)->MinerData.bMinerOnline == FALSE)
				strcpy((char *)OnLineBuf, "FALSE");
			else
				strcpy((char *)OnLineBuf, "TRUE");
			pthread_mutex_unlock(&((stMinerConn *)pNode->val)->MinerDataLock.OnlineLock);

			iReg = ((stMinerConn *)pNode->val)->MinerData.iRegtime;
			iLogin = ((stMinerConn *)pNode->val)->MinerData.iLoginTime;
			iLogout = ((stMinerConn *)pNode->val)->MinerData.iLogoutTime;		
			GetLocalTimeString((char *)regBuf, sizeof(regBuf), iReg);
			GetLocalTimeString((char *)loginBuf, sizeof(loginBuf), iLogin);
			if (iLogout)
				GetLocalTimeString((char *)logoutBuf, sizeof(logoutBuf), iLogout);
			else
				strcpy((char *)logoutBuf, "NULL");
			
			switch(((stMinerConn *)pNode->val)->MinerData.eMinerAlgo)
			{
			case E_ALGO_BTC:
			case E_ALGO_LTC:
				snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos, destLen, 
						"No:%d\r\n\tminer_id:%d\r\n\tminer_ip:%s\r\n\tminer_port:%d\r\n\talgo:%s\r\n\tis_online:%s\r\n\twbs_id:%d	\
						\r\n\tcpm:%d\r\n\twb_ip:%s\r\n\twb_port:%s\r\n\treg_time:%s\r\n\tlogin_time:%s\r\n\tlogout_time:%s\r\n", 
						iIndex + i,
						((stMinerConn *)pNode->val)->MinerData.iMinerId,
						inet_ntoa(((stMinerConn *)pNode->val)->remote.addr4.sin_addr),
						((stMinerConn *)pNode->val)->remote.addr4.sin_port,
						AlgoBuf,
						OnLineBuf,
						((stMinerConn *)pNode->val)->pMinerRes->iWbsId,
						((stMinerConn *)pNode->val)->pMinerRes->iConnCellNum,
						((stMinerConn *)pNode->val)->pMinerRes->WbIpPort.byWbIp,
						((stMinerConn *)pNode->val)->pMinerRes->WbIpPort.byWbPort,
						regBuf, loginBuf, logoutBuf
						);
				break;
				
			case E_ALGO_DUAL:
				snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos, destLen, 
						"No:%d\r\n\tminer_id:%d\r\n\tminer_ip:%s\r\n\tminer_port:%d\r\n\talgo:%s\r\n\tis_online:%s\r\n\tbtc_wbsid:%d	\
						\r\n\tbtc_cpm:%d\r\n\tbtc_wbip:%s\r\n\tbtc_wbport:%s\r\n\tltc_wbsid:%d\r\n\tltc_cpm:%d\r\n\tltc_wbip:%s		\
						\r\n\tltc_wbport:%s\r\n\treg_time:%s\r\n\tlogin_time:%s\r\n\tlogout_time:%s\r\n", 
						iIndex + i,
						((stMinerConn *)pNode->val)->MinerData.iMinerId,
						inet_ntoa(((stMinerConn *)pNode->val)->remote.addr4.sin_addr),
						((stMinerConn *)pNode->val)->remote.addr4.sin_port,
						AlgoBuf,
						OnLineBuf,
						((stMinerConn *)pNode->val)->pMinerRes[0].iWbsId,
						((stMinerConn *)pNode->val)->pMinerRes[0].iConnCellNum,
						((stMinerConn *)pNode->val)->pMinerRes[0].WbIpPort.byWbIp,
						((stMinerConn *)pNode->val)->pMinerRes[0].WbIpPort.byWbPort,
						((stMinerConn *)pNode->val)->pMinerRes[1].iWbsId,
						((stMinerConn *)pNode->val)->pMinerRes[1].iConnCellNum,
						((stMinerConn *)pNode->val)->pMinerRes[1].WbIpPort.byWbIp,
						((stMinerConn *)pNode->val)->pMinerRes[1].WbIpPort.byWbPort,
						regBuf, loginBuf, logoutBuf
						);

				//printf("0 iWbsId:%d, @:%p, 1 iWbsId:%d. @%p\r\n", 
				//	((stMinerConn *)pNode->val)->pMinerRes[0].iWbsId, &((stMinerConn *)pNode->val)->pMinerRes[0].iWbsId,
				//	((stMinerConn *)pNode->val)->pMinerRes[1].iWbsId, &((stMinerConn *)pNode->val)->pMinerRes[1].iWbsId);
				break;

			default:
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, Algorithm[%d] does not exist!\r\n", __FILE__, __FUNCTION__, __LINE__, ((stMinerConn *)pNode->val)->eClinetType);
				break;
			}			
			//printf("Miner.......iPos:%d, byUserCmdBuf:%s\r\n", iPos, ((stUserSendBuf*)dest)->byUserCmdBuf+iPos);
			iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);			
			DEBUGMSG(0,("1 Miner @pNode:%p, val: %s\r\n", pNode, (char *)pNode->val));
			pNode = (devlist_ist*)(pNode->ah.next);
			if (pNode == NULL)
				break;
		}
		((structUthash *)pList)->SelCrrNode->SelCrrUsers = pNode;
		DEBUGMSG(0,("2 @pNode:%p, @SelCrrUsers:%p\r\n", pNode, ((structUthash *)pList)->SelCrrNode->SelCrrUsers));
	}
	else
	{
		((structUthash *)pList)->SelCrrNode->SelCrrUsers = NULL;
	}
	//pthread_mutex_unlock(((structUthash *)pList)->ShellRemLock);

	DEBUGMSG(0,("****iPos:%d\r\n", iPos));
	return rtn;
}*/

#if 1
int PackGetCell(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0, i = 0, iTotListLen = 0, iListLen = 0, iErrIndex = 0, j = 0;
	void *pList = NULL;
	devlist_st *pNode = NULL;
	BYTE AlgoBuf[BUFFER_LEN_16] = {0};
	BYTE OnLineBuf[BUFFER_LEN_8] = {0};
	int iPos = 0, iPosPool = 0;
	int iNumPack = 0, iRemain = 0, iLoop = 0, iIndex = 0;
	devlist_st *SelNode = NULL;
	long iReg = 0, iLogin = 0, iLogout = 0;
	BYTE regBuf[BUFFER_LEN_64] = {0}, loginBuf[BUFFER_LEN_64] = {0}, logoutBuf[BUFFER_LEN_64] = {0};
	BYTE poolBufPri[BUFFER_LEN_64] = {0};

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	pList = &stCellUtlist;
	if (!((stUserSendBuf*)dest)->bSubcontFlag)
	{	
		pthread_mutex_lock(((structList *)pList)->ListLenLock);
		iTotListLen = ((structUthash *)pList)->CountUsersStr(*((structUthash *)pList)->StrHUsers); 
		pthread_mutex_unlock(((structList *)pList)->ListLenLock);

		if ((iErrIndex = ParseGetOrSet(argc, argv, iTotListLen, pList)) > 0)
		{
			if (gUserComdErr[E_SHELL_ERROR_GETCELL].UserCmdFunc(gGetOrSetErr[iErrIndex - 1].cmd, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen) < 0)
				return (rtn = E_SHELL_ERROR_GETCELL);
			else
				return rtn;
		}
		
		//((stUserSendBuf*)dest)->iNumStart = atoi(argv[1]);
		//((stUserSendBuf*)dest)->iNumEnd = atoi(argv[2]);
		//先指定起始id和结束id
		memset(((structUthash *)pList)->SelCrrNode, 0, sizeof(selnode_st));
		//((structUthash *)pList)->SelCrrNode->StartId = atoi(argv[1]);
		strncpy(((structUthash *)pList)->SelCrrNode->szStartId, argv[1], sizeof(((structUthash *)pList)->SelCrrNode->szStartId ));
		//((structUthash *)pList)->SelCrrNode->EndId = atoi(argv[2]);	
		strncpy(((structUthash *)pList)->SelCrrNode->szEndId, argv[2], sizeof(((structUthash *)pList)->SelCrrNode->szEndId ));
		((structUthash *)pList)->SelCrrNode->SelCrrUsers = ((structUthash *)pList)->SelectcondStr(((structUthash *)pList)->StrHUsers);
		for(SelNode = ((structUthash *)pList)->SelCrrNode->SelCrrUsers; SelNode; SelNode = (devlist_st*)SelNode->ah.next)
		{
			((structUthash *)pList)->SelCrrNode->SelLen++;
		}
		iListLen = ((structUthash *)pList)->SelCrrNode->SelLen;		
		iRemain = iListLen % CELL_LIST_PACK_NUM;
		iNumPack = iRemain > 0 ? iListLen / CELL_LIST_PACK_NUM+1 : iListLen / CELL_LIST_PACK_NUM;
		
		if (iNumPack > 1)	//分包
		{	
			((stUserSendBuf*)dest)->bSubcontFlag = TRUE;
			((stUserSendBuf*)dest)->iNumPack = iNumPack;
			((stUserSendBuf*)dest)->iRemain = iRemain;
		}

		DEBUGMSG(1,("iRemain:%d, iSubindex:%d\r\n",((stUserSendBuf*)dest)->iRemain, ((stUserSendBuf*)dest)->iSubindex));
	}

	//循环打包
	if (((stUserSendBuf*)dest)->iSubindex <= ((stUserSendBuf*)dest)->iNumPack - 1)
	{
		if (((stUserSendBuf*)dest)->iSubindex == (((stUserSendBuf*)dest)->iNumPack - 1) && ((stUserSendBuf*)dest)->iRemain)
			iLoop = ((stUserSendBuf*)dest)->iRemain;
		else
			iLoop = CELL_LIST_PACK_NUM;
	}
	else
	{
		iLoop = iListLen;
	}

	DEBUGMSG(1,("iSubindex:%d, iNumStart:%d, iNumEnd:%d\r\n", ((stUserSendBuf*)dest)->iSubindex, ((stUserSendBuf*)dest)->iNumStart, ((stUserSendBuf*)dest)->iNumEnd));

	iIndex = ((stUserSendBuf*)dest)->iSubindex * CELL_LIST_PACK_NUM;//((stUserSendBuf*)dest)->iNumStart;	//分包后打过包的链表下标
    DEBUGMSG(1,("iIndex:%d, iLoop:%d\r\n", iIndex, iLoop));

	pNode = ((structUthash *)pList)->SelCrrNode->SelCrrUsers;
	if (pNode != NULL)
	{
		DEBUGMSG(1,("3333333333 @pNode->val:%p, sin_port:%d\n", (stCellConn *)pNode->val, ((stCellConn *)pNode->val)->remote.addr4.sin_port));
	
		//pthread_mutex_lock(((structList *)pList)->RemoveLock);
	    for (i = 0; i < iLoop; i++)
		{
			//if (iIndex + i >= ((stUserSendBuf*)dest)->iNumEnd)			//打包到结尾了
			//	break;
			
			//pNode = ((structList *)pList)->ListAt(((structList *)pList)->List, iIndex + i);

			AlgoEnumToString(((stCellConn *)pNode->val)->CellData.eCellAlgo, (char *)AlgoBuf, sizeof(AlgoBuf));

			pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
			if (((stCellConn *)pNode->val)->CellData.bCellOnline == FALSE)
				strcpy((char *)OnLineBuf, "FALSE");
			else
				strcpy((char *)OnLineBuf, "TRUE");
			pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);

			iReg = ((stCellConn *)pNode->val)->CellData.iRegtime;
			iLogin = ((stCellConn *)pNode->val)->CellData.iLoginTime;
			iLogout = ((stCellConn *)pNode->val)->CellData.iLogoutTime;		
			GetLocalTimeString((char *)regBuf, sizeof(regBuf), iReg);
			GetLocalTimeString((char *)loginBuf, sizeof(loginBuf), iLogin);
			if (iLogout)
				GetLocalTimeString((char *)logoutBuf, sizeof(logoutBuf), iLogout);
			else
				strcpy((char *)logoutBuf, "NULL");

			//初始化下poolBufPri
			memset(poolBufPri, 0, sizeof(poolBufPri));
			iPosPool = 0;
			if (((stCellConn *)pNode->val)->CellPoolConf.iBTCPoolCount != 0 && 
				((stCellConn *)pNode->val)->CellPoolConf.iBTCPoolCount <= MAX_POOL_COUNT)
			{
				snprintf((char *)poolBufPri + iPosPool, sizeof(poolBufPri), "%d",
					((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[0].eAlgo);
				iPosPool += strlen((char *)poolBufPri + iPosPool);
					
				for (j = 0;j < ((stCellConn *)pNode->val)->CellPoolConf.iBTCPoolCount;j++)
				{
					snprintf((char *)poolBufPri + iPosPool, sizeof(poolBufPri), "/%d",
						((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].iPoolId);
					iPosPool += strlen((char *)poolBufPri + iPosPool);
				}					
			}

			if (((stCellConn *)pNode->val)->CellPoolConf.iLTCPoolCount != 0 && 
				((stCellConn *)pNode->val)->CellPoolConf.iLTCPoolCount <= MAX_POOL_COUNT)
			{
				snprintf((char *)poolBufPri + iPosPool, sizeof(poolBufPri), " %d",
					((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[0].eAlgo);
				iPosPool += strlen((char *)poolBufPri + iPosPool);
					
				for (j = 0;j < ((stCellConn *)pNode->val)->CellPoolConf.iLTCPoolCount;j++)
				{
					snprintf((char *)poolBufPri + iPosPool, sizeof(poolBufPri), "/%d",
						((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].iPoolId);
					iPosPool += strlen((char *)poolBufPri + iPosPool);
				}					
			}

			snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos, destLen, 
					"\r\nNo:%d\r\n\tcell_id:%s\r\n\tcell_ip:%s\r\n\tcell_port:%d\r\n\tsw_ver:%s\r\n\thw_ver:%s\r\n\tchip_cnt:%d	\
					\r\n\tchip_btc_hash:%lld\r\n\tchip_ltc_hash:%lld\r\n\tchip_btc_freq:%d\r\n\tchip_ltc_freq:%d\r\n\talgo:%s\r\n\tis_online:%s	\
					\r\n\tpoolPri:%s\r\n\tTestResult:%s\r\n\tBTCRatedKHash:%lld\r\n\tLTCRatedKHash:%lld	\
					\r\n\tBTCRealKHash:%lld\r\n\tLTCRealKHash:%lld	\
					\r\n\treg_time:%s\r\n\tlogin_time:%s\r\n\tlogout_time:%s\r\n", 
					iIndex + i,
					//((stCellConn *)pNode->val)->CellData.iCellId,
					((stCellConn *)pNode->val)->CellData.szCellId,
					//(char *)((stCellConn *)pNode->val)->CellData.szDevId,	// \tdev_id:%s\r\n
					inet_ntoa(((stCellConn *)pNode->val)->remote.addr4.sin_addr),
					((stCellConn *)pNode->val)->remote.addr4.sin_port,
					//((stCellConn *)pNode->val)->CellData.iSwVer,
					//((stCellConn *)pNode->val)->CellData.iHwVer,
					((stCellConn *)pNode->val)->CellData.szSwVer,
					((stCellConn *)pNode->val)->CellData.szHwVer,
					((stCellConn *)pNode->val)->CellData.iChipCnt,
					((stCellConn *)pNode->val)->CellData.iChipBTCHash,
					((stCellConn *)pNode->val)->CellData.iChipLTCHash,					
					((stCellConn *)pNode->val)->CellData.iChipBTCFreq,
					((stCellConn *)pNode->val)->CellData.iChipLTCFreq,
					AlgoBuf,
					OnLineBuf,
					(char *)poolBufPri,
					((stCellConn *)pNode->val)->CellData.szTestResult,	//\tTestResult:%s\r\n
					((stCellConn *)pNode->val)->CellData.lBTCRatedKHash,
					((stCellConn *)pNode->val)->CellData.lLTCRatedKHash,
					((stCellConn *)pNode->val)->CellStat.llBTCRealKHash,
					((stCellConn *)pNode->val)->CellStat.llLTCRealKHash,
					regBuf, loginBuf, logoutBuf
					);
			/*switch(((stCellConn *)pNode->val)->CellData.eCellAlgo)
			{
			case E_ALGO_BTC:
			case E_ALGO_LTC:
				snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos, destLen, 
						"No:%d\r\n\tcell_id:%d\r\n\tdev_id:%ld\r\n\tcell_ip:%s\r\n\tcell_port:%d\r\n\tsw_ver:%d\r\n\thw_ver:%d\r\n\tchip_hash:%d	\
						\r\n\tchip_cnt:%d\r\n\tchip_freq:%d\r\n\talgo:%s\r\n\tis_online:%s\r\n\twbsid:%d\r\n\twbcid:%d	\
						\r\n\tcpm:%d\r\n\twbip:%s\r\n\twbport:%s\r\n\treg_time:%s\r\n\tlogin_time:%s\r\n\tlogout_time:%s\r\n", 
						iIndex + i,
						((stCellConn *)pNode->val)->CellData.iCellId,
						((stCellConn *)pNode->val)->CellData.lDevId,
						inet_ntoa(((stCellConn *)pNode->val)->remote.addr4.sin_addr),
						((stCellConn *)pNode->val)->remote.addr4.sin_port,
						((stCellConn *)pNode->val)->CellData.iSwVer,
						((stCellConn *)pNode->val)->CellData.iHwVer,
						((stCellConn *)pNode->val)->CellData.iChipBTCHash,
						((stCellConn *)pNode->val)->CellData.iChipCnt,
						((stCellConn *)pNode->val)->CellData.iChipBTCFreq,
						AlgoBuf,
						OnLineBuf,
						((stCellConn *)pNode->val)->pCellRes->iWbNo,
						((stCellConn *)pNode->val)->pCellRes->iWbcId,
						((stCellConn *)pNode->val)->pCellRes->iConnCellNum,
						((stCellConn *)pNode->val)->pCellRes->WbIpPort.byWbIp,
						((stCellConn *)pNode->val)->pCellRes->WbIpPort.byWbPort,
						regBuf, loginBuf, logoutBuf
						);
				break;
			case E_ALGO_DUAL:
				snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos, destLen, 
						"No:%d\r\n\tcell_id:%d\r\n\tdev_id:%ld\r\n\tcell_ip:%s\r\n\tcell_port:%d\r\n\tsw_ver:%d\r\n\thw_ver:%d\r\n\tchip_hash:%d	\
						\r\n\tchip_cnt:%d\r\n\tchip_freq:%d\r\n\talgo:%s\r\n\tis_online:%s\r\n\tbtc_wbsid:%d\r\n\tbtc_wbcid:%d	\
						\r\n\tbtc_cpm:%d\r\n\tbtc_wbip:%s\r\n\tbtc_wbport:%s\r\n\tltc_wbsid:%d\r\n\tltc_wbcid:%d\r\n\tltc_cpm:%d	\
						\r\n\tltc_wbip:%s\r\n\tltc_wbport:%s\r\n\treg_time:%s\r\n\tlogin_time:%s\r\n\tlogout_time:%s\r\n", 
						iIndex + i,
						((stCellConn *)pNode->val)->CellData.iCellId,
						((stCellConn *)pNode->val)->CellData.lDevId,
						inet_ntoa(((stCellConn *)pNode->val)->remote.addr4.sin_addr),
						((stCellConn *)pNode->val)->remote.addr4.sin_port,		
						((stCellConn *)pNode->val)->CellData.iSwVer,
						((stCellConn *)pNode->val)->CellData.iHwVer,
						((stCellConn *)pNode->val)->CellData.iChipBTCHash,
						((stCellConn *)pNode->val)->CellData.iChipCnt,
						((stCellConn *)pNode->val)->CellData.iChipBTCFreq,
						AlgoBuf,
						OnLineBuf,
						((stCellConn *)pNode->val)->pCellRes[0].iWbNo,
						((stCellConn *)pNode->val)->pCellRes[0].iWbcId,
						((stCellConn *)pNode->val)->pCellRes[0].iConnCellNum,
						((stCellConn *)pNode->val)->pCellRes[0].WbIpPort.byWbIp,
						((stCellConn *)pNode->val)->pCellRes[0].WbIpPort.byWbPort,
						((stCellConn *)pNode->val)->pCellRes[1].iWbNo,
						((stCellConn *)pNode->val)->pCellRes[1].iWbcId,
						((stCellConn *)pNode->val)->pCellRes[1].iConnCellNum,
						((stCellConn *)pNode->val)->pCellRes[1].WbIpPort.byWbIp,
						((stCellConn *)pNode->val)->pCellRes[1].WbIpPort.byWbPort,
						regBuf, loginBuf, logoutBuf
						);			
				break;
				
			default:
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, Algorithm[%d] does not exist!\r\n", __FILE__, __FUNCTION__, __LINE__, ((stCellConn *)pNode->val)->eClinetType);
				break;
			}*/

			DEBUGMSG(1,("4444444444 @pNode->val:%p, sin_port:%d\n", (stCellConn *)pNode->val, ((stCellConn *)pNode->val)->remote.addr4.sin_port));
			
			iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);
			pNode = (devlist_st*)(pNode->ah.next);
			if (pNode == NULL)
				break;		
			//printf("Cell.......iPos:%d\r\n", iPos);
		}
		((structUthash *)pList)->SelCrrNode->SelCrrUsers = pNode;		
	}
	//pthread_mutex_unlock(((structList *)pList)->RemoveLock);

	DEBUGMSG(1,("****iPos:%d\r\n", iPos));
	return rtn;
}
#endif

#if 0
int PackGetCell(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0, i = 0, j = 0;//iTotListLen = 0, iListLen = 0, iErrIndex = 0, 
	structUthash *pList = NULL;//void *pList = NULL;
	devlist_st *pNode = NULL, *tmp = NULL;
	BYTE AlgoBuf[BUFFER_LEN_16] = {0};
	BYTE OnLineBuf[BUFFER_LEN_8] = {0};
	int iPos = 0, iPosPool = 0;
	int iIndex = 0;//iNumPack = 0, iRemain = 0, iLoop = 0, 
	//devlist_st *SelNode = NULL;
	long iReg = 0, iLogin = 0, iLogout = 0;
	BYTE regBuf[BUFFER_LEN_64] = {0}, loginBuf[BUFFER_LEN_64] = {0}, logoutBuf[BUFFER_LEN_64] = {0};
	BYTE poolBufPri[BUFFER_LEN_64] = {0};

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	pList = &stCellUtlist;

//{
	//DEBUGMSG(1,("3333333333 @pNode->val:%p, sin_port:%d\n", (stCellConn *)pNode->val, ((stCellConn *)pNode->val)->remote.addr4.sin_port));

	pthread_mutex_lock(((structList *)pList)->RemoveLock);
	HASH_ITER(hh, *pList->StrHUsers, pNode, tmp)//for (i = 0; i < iLoop; i++)
	{
		//if (iIndex + i >= ((stUserSendBuf*)dest)->iNumEnd)			//打包到结尾了
		//	break;
		
		//pNode = ((structList *)pList)->ListAt(((structList *)pList)->List, iIndex + i);

		AlgoEnumToString(((stCellConn *)pNode->val)->CellData.eCellAlgo, (char *)AlgoBuf, sizeof(AlgoBuf));

		pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
		if (((stCellConn *)pNode->val)->CellData.bCellOnline == FALSE)
			strcpy((char *)OnLineBuf, "FALSE");
		else
			strcpy((char *)OnLineBuf, "TRUE");
		pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);

		iReg = ((stCellConn *)pNode->val)->CellData.iRegtime;
		iLogin = ((stCellConn *)pNode->val)->CellData.iLoginTime;
		iLogout = ((stCellConn *)pNode->val)->CellData.iLogoutTime; 	
		GetLocalTimeString((char *)regBuf, sizeof(regBuf), iReg);
		GetLocalTimeString((char *)loginBuf, sizeof(loginBuf), iLogin);
		if (iLogout)
			GetLocalTimeString((char *)logoutBuf, sizeof(logoutBuf), iLogout);
		else
			strcpy((char *)logoutBuf, "NULL");

		//初始化下poolBufPri
		memset(poolBufPri, 0, sizeof(poolBufPri));
		iPosPool = 0;
		if (((stCellConn *)pNode->val)->CellPoolConf.iBTCPoolCount != 0 && 
			((stCellConn *)pNode->val)->CellPoolConf.iBTCPoolCount <= MAX_POOL_COUNT)
		{
			snprintf((char *)poolBufPri + iPosPool, sizeof(poolBufPri), "%d",
				((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[0].eAlgo);
			iPosPool += strlen((char *)poolBufPri + iPosPool);
				
			for (j = 0;j < ((stCellConn *)pNode->val)->CellPoolConf.iBTCPoolCount;j++)
			{
				snprintf((char *)poolBufPri + iPosPool, sizeof(poolBufPri), "/%d",
					((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].iPoolId);
				iPosPool += strlen((char *)poolBufPri + iPosPool);
			}					
		}

		if (((stCellConn *)pNode->val)->CellPoolConf.iLTCPoolCount != 0 && 
			((stCellConn *)pNode->val)->CellPoolConf.iLTCPoolCount <= MAX_POOL_COUNT)
		{
			snprintf((char *)poolBufPri + iPosPool, sizeof(poolBufPri), " %d",
				((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[0].eAlgo);
			iPosPool += strlen((char *)poolBufPri + iPosPool);
				
			for (j = 0;j < ((stCellConn *)pNode->val)->CellPoolConf.iLTCPoolCount;j++)
			{
				snprintf((char *)poolBufPri + iPosPool, sizeof(poolBufPri), "/%d",
					((stCellConn *)pNode->val)->CellPoolConf.stLTCPoolConf[j].iPoolId);
				iPosPool += strlen((char *)poolBufPri + iPosPool);
			}					
		}

		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos, destLen, 
				"\r\nNo:%d\r\n\tcell_id:%s\r\n\tcell_ip:%s\r\n\tcell_port:%d\r\n\tsw_ver:%s\r\n\thw_ver:%s\r\n\tchip_cnt:%d \
				\r\n\tchip_btc_hash:%lld\r\n\tchip_ltc_hash:%lld\r\n\tchip_btc_freq:%d\r\n\tchip_ltc_freq:%d\r\n\talgo:%s\r\n\tis_online:%s \
				\r\n\tpoolPri:%s\r\n\tTestResult:%s\r\n\tBTCRatedKHash:%lld\r\n\tLTCRatedKHash:%lld \
				\r\n\tBTCRealKHash:%lld\r\n\tLTCRealKHash:%lld	\
				\r\n\treg_time:%s\r\n\tlogin_time:%s\r\n\tlogout_time:%s\r\n", 
				iIndex + i,
				//((stCellConn *)pNode->val)->CellData.iCellId,
				((stCellConn *)pNode->val)->CellData.szCellId,
				//(char *)((stCellConn *)pNode->val)->CellData.szDevId, // \tdev_id:%s\r\n
				inet_ntoa(((stCellConn *)pNode->val)->remote.addr4.sin_addr),
				((stCellConn *)pNode->val)->remote.addr4.sin_port,
				//((stCellConn *)pNode->val)->CellData.iSwVer,
				//((stCellConn *)pNode->val)->CellData.iHwVer,
				((stCellConn *)pNode->val)->CellData.szSwVer,
				((stCellConn *)pNode->val)->CellData.szHwVer,
				((stCellConn *)pNode->val)->CellData.iChipCnt,
				((stCellConn *)pNode->val)->CellData.iChipBTCHash,
				((stCellConn *)pNode->val)->CellData.iChipLTCHash,					
				((stCellConn *)pNode->val)->CellData.iChipBTCFreq,
				((stCellConn *)pNode->val)->CellData.iChipLTCFreq,
				AlgoBuf,
				OnLineBuf,
				(char *)poolBufPri,
				((stCellConn *)pNode->val)->CellData.szTestResult,	//\tTestResult:%s\r\n
				((stCellConn *)pNode->val)->CellData.lBTCRatedKHash,
				((stCellConn *)pNode->val)->CellData.lLTCRatedKHash,
				((stCellConn *)pNode->val)->CellStat.llBTCRealKHash,
				((stCellConn *)pNode->val)->CellStat.llLTCRealKHash,
				regBuf, loginBuf, logoutBuf
				);
				i++;
		//DEBUGMSG(1,("4444444444 @pNode->val:%p, sin_port:%d\n", (stCellConn *)pNode->val, ((stCellConn *)pNode->val)->remote.addr4.sin_port));
		
		iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);
		//pNode = (devlist_st*)(pNode->ah.next);
		//if (pNode == NULL)
		//	break;		
		//printf("Cell.......iPos:%d\r\n", iPos);
	}
	pthread_mutex_unlock(((structList *)pList)->RemoveLock);
	//((structUthash *)pList)->SelCrrNode->SelCrrUsers = pNode;		
//}

	return rtn;
}
#endif

#if 0
int PackDelMiner(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0, iErrIndex = 0;
	BYTE timeBuf[BUFFER_LEN_256] = {0};

	if ((iErrIndex = ParseDelMiner(argc, argv)) != 0)
	{
		if (gUserComdErr[E_SHELL_ERROR_DELMINER].UserCmdFunc(gDelErr[iErrIndex - 1].cmd, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen) < 0)
			return (rtn = 2);
		else
			return rtn;
	}
	else
	{
		GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen, "%s\t%s\r\n", (char *)timeBuf, "Deleted successfully!");
	}

	return rtn;
}
#endif

int PackDelCell(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0, iErrIndex = 0;
	BYTE timeBuf[BUFFER_LEN_256] = {0};

	if ((iErrIndex = ParseDelCell(argc, argv)) != 0)
	{
		if (gUserComdErr[E_SHELL_ERROR_DELCELL].UserCmdFunc(gDelErr[iErrIndex - 1].cmd, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen) < 0)
			return (rtn = 2);
		else
			return rtn;
	}
	else
	{
		GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen, "%s\t%s\r\n", (char *)timeBuf, "Deleted successfully!");
	}

	return rtn;
}

#if 0
int PackAdjCpm(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0, iErrIndex = 0;
	BYTE timeBuf[BUFFER_LEN_256] = {0};

	if ((iErrIndex = ParseAdjCpm(argc, argv)) != 0)
	{
		if (gUserComdErr[E_SHELL_ERROR_ADJCPM].UserCmdFunc(gDelErr[iErrIndex - 1].cmd, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen) < 0)
			return (rtn = 2);
		else
			return rtn;
	}
	else
	{
		GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen, "%s\t%s\r\n", (char *)timeBuf, "Adjust successfully!");
	}

	return rtn;
}
#endif 

//设置可控制命令
int PackComm(int argc, char **argv, void *dest, int destLen)
{
	int rtn = -1;
	int i = 0, j = 0;//iCountI = 0, 
	BYTE timeBuf[BUFFER_LEN_256] = {0};
	devlist_st *pNode, *tmp = NULL;
	int iPos = 0;
	E_SET_MASS eSetMass = E_SET_MASS_ALL_SUCC;
	int iTotListLen = 0, ioffLineCount = 0;
	
	DEBUGMSG(1,("%s..., destLen:%d\r\n", __FUNCTION__, destLen));
	//DEBUGMSG(1,("################pSrcBuf:%p\r\n", idControlInput[0].pIdCommandPack[1].pSrcBuf));
		
	rtn = ParseShellCommand(argc, argv, 1);
	if(rtn == 0)
	{
		if (CommShellFlag.bMassFlag)
		{
			//因为群发，数据多，最多延迟15s。
			MSleep(15000);
			CommShellFlag.bMassFlag = FALSE;

			pthread_mutex_lock(idControlInput[i].pList->ListLenLock);
			iTotListLen = idControlInput[i].pList->CountUsersStr(*idControlInput[i].pList->StrHUsers); 
			pthread_mutex_unlock(idControlInput[i].pList->ListLenLock);

			pthread_mutex_lock(CommShellFlag.UserSeqLock);
			//for (j = 0; j < MAX_CLIENTS; j++)
			//{
			//if (CommShellFlag.bRecvSeqFlag[j] == TRUE)
			//{
			GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
			snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen, "%s\t", (char *)timeBuf);
			iPos = strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf);
			
			pthread_mutex_lock(idControlInput[i].pList->ShellRemLock);
			HASH_ITER(hh, *idControlInput[i].pList->StrHUsers, pNode, tmp)
			{
				pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
				//只对在线的设备进行发送重新登录命令，同时关闭掉了状态查询。
				if (((stCellConn *)pNode->val)->CellData.bCellOnline == TRUE)
				{
					DEBUGMSG(0,("+++++++++++++++szCellId:%s, eCellComm:%d\r\n", ((stCellConn *)pNode->val)->CellData.szCellId, 
						((stCellConn *)pNode->val)->eCellComm));
					if (((stCellConn *)pNode->val)->eCellComm)
					{
						//对于没有收到的结点，序列号也复位。
						((stCellConn *)pNode->val)->uSeqId = 0;
						
						((stCellConn *)pNode->val)->eCellComm = E_COMMAND_TYPE_STAT;
						if (!eSetMass)
							eSetMass = E_SET_MASS_PART_FAIL;
						snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "%s ", 
							((stCellConn *)pNode->val)->CellData.szCellId);	
						//iPos = iPos + sizeof(((stCellConn *)pNode->val)->CellData.iCellId) + 1;
						iPos = strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf);
					}
				}
				else
				{
					ioffLineCount++;
				}
				pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
			}
			pthread_mutex_unlock(idControlInput[i].pList->ShellRemLock); 

			if (ioffLineCount == iTotListLen)
				eSetMass = E_SET_MASS_ALL_INTER;

			switch(eSetMass)
			{
				case E_SET_MASS_ALL_SUCC:
					snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "%s\r\n", "Setting successfully!");
					break;

				case E_SET_MASS_ALL_INTER:
					snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "%s\r\n", "Communications interruption!");
					break;

				case E_SET_MASS_PART_FAIL:
					snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "%s\r\n", " These cell_id setting failed!");
					break;

				default:
					GetLocalTimeLog();
					ap_log_debug_log("\t\t%s, %s, %d, eSetMass is error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
					break;
			}
		
			//CommShellFlag.bRecvSeqFlag[j] = FALSE;
			pthread_mutex_unlock(CommShellFlag.UserSeqLock);
			//DEBUGMSG(1,("--------------i:%d\r\n", i));
			return 0;
			//}
			//}
			//pthread_mutex_unlock(CommShellFlag.UserSeqLock);
		}
		else
		{
			for(i = 0; i < QUERY_COUNT; i++)
			{
				MSleep(1000);
				pthread_mutex_lock(CommShellFlag.UserSeqLock);
				for (j = 0; j < MAX_CLIENTS; j++)
				{
					if (CommShellFlag.bRecvSeqFlag[j] == TRUE)
					{
						GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
						snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen, "%s\t%s\r\n", (char *)timeBuf, "Setting successfully!");
						CommShellFlag.bRecvSeqFlag[j] = FALSE;
						pthread_mutex_unlock(CommShellFlag.UserSeqLock);
						DEBUGMSG(1,("--------------i:%d\r\n", i));
						return 0;
					}
					/*else
					{
						if (++iCountI == MAX_CLIENTS)
							snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen, "%s\r\n", "Setting failed");
					}*/
				}
				pthread_mutex_unlock(CommShellFlag.UserSeqLock);
			}
		}				
		DEBUGMSG(1,("--------------i--------------:%d\r\n", i));
		GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen, "%s\t%s\r\n", (char *)timeBuf, "Setting failed");		
	}
	else if (rtn > 0)
	{
		GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen, "%s\t%s\r\n", (char *)timeBuf, (char *)gCommandErr[rtn - 1].cmd);
	}
	DEBUGMSG(1,("%s, UserSendLen:%d, %d, %d\r\n", (char *)((stUserSendBuf*)dest)->byUserCmdBuf, strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf), sizeof(gUserSendBuf), sizeof(stUserSendBuf)));
	
	return rtn;
}

//把用户命令reboot_cell打包成Command解析形式的命令
int PackCellRebootToComm(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0;
	char delim[] = " ";
	int PackArgc = 0;
	char *PackArgv[BUFFER_LEN_256] = {0};
	char Packbuf[BUFFER_LEN_256] = {0};
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	if(argc != COMMANDLINE_3)
	{
		gUserComdErr[E_SHELL_ERROR_SET_COMM].UserCmdFunc(NULL, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen);		
		return E_SHELL_ERROR_SET_COMM;
	}

	if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "comm cell %s reboot %s", argv[1], argv[2])) < 0)
		return rtn;

	if (Parse(Packbuf, &PackArgc, PackArgv, delim) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return (rtn = 2);
	}
	
	rtn = PackComm(PackArgc, PackArgv, dest, destLen);

	return rtn;
}

//把用户命令set_cell_led打包成Command解析形式的命令
int PackCellLedToComm(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0;
	char delim[] = " ";
	int PackArgc = 0;
	char *PackArgv[BUFFER_LEN_256] = {0};
	char Packbuf[BUFFER_LEN_256] = {0};
	
	if(argc != COMMANDLINE_3)
	{
		gUserComdErr[E_SHELL_ERROR_SET_COMM].UserCmdFunc(NULL, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen);		
		return E_SHELL_ERROR_SET_COMM;
	}

	if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "comm cell %s setled %s", argv[1], argv[2])) < 0)
		return rtn;

	if (Parse(Packbuf, &PackArgc, PackArgv, delim) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return (rtn = 2);
	}
	
	rtn = PackComm(PackArgc, PackArgv, dest, destLen);

	return rtn;
}

//把用户命令start_cell_test打包成Command解析形式的命令
int PackStartCellTestToComm(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0;
	char delim[] = " ";
	int PackArgc = 0;
	char *PackArgv[BUFFER_LEN_256] = {0};
	char Packbuf[BUFFER_LEN_256] = {0};

	if(argc != COMMANDLINE_2)
	{
		gUserComdErr[E_SHELL_ERROR_SET_COMM].UserCmdFunc(NULL, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen);		
		return E_SHELL_ERROR_SET_COMM;
	}

	if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "comm cell %s start_test", argv[1])) < 0)
		return rtn;

	if (Parse(Packbuf, &PackArgc, PackArgv, delim) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return (rtn = 2);
	}
	
	rtn = PackComm(PackArgc, PackArgv, dest, destLen);

	return rtn;
}

//把用户命令cell_relogin打包成Command解析形式的命令
int PackCellReloginToComm(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0;
	char delim[] = " ";
	int PackArgc = 0;
	char *PackArgv[BUFFER_LEN_256] = {0};
	char Packbuf[BUFFER_LEN_256] = {0};

	if(argc != COMMANDLINE_2)
	{
		gUserComdErr[E_SHELL_ERROR_SET_COMM].UserCmdFunc(NULL, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen);		
		return E_SHELL_ERROR_SET_COMM;
	}

	if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "comm cell %s relogin", argv[1])) < 0)
		return rtn;

	if (Parse(Packbuf, &PackArgc, PackArgv, delim) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return (rtn = 2);
	}
	
	rtn = PackComm(PackArgc, PackArgv, dest, destLen);

	return rtn;
}

//把用户命令miner_relogin打包成Command解析形式的命令
int PackMinerReloginToComm(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0;
	char delim[] = " ";
	int PackArgc = 0;
	char *PackArgv[BUFFER_LEN_256] = {0};
	char Packbuf[BUFFER_LEN_256] = {0};

	if(argc != COMMANDLINE_2)
	{
		gUserComdErr[E_SHELL_ERROR_SET_COMM].UserCmdFunc(NULL, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen);		
		return E_SHELL_ERROR_SET_COMM;
	}

	if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "comm miner %s relogin", argv[1])) < 0)
		return rtn;

	if (Parse(Packbuf, &PackArgc, PackArgv, delim) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return (rtn = 2);
	}
	
	rtn = PackComm(PackArgc, PackArgv, dest, destLen);

	return rtn;
}

int PackGetAllCellId(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0, iErrIndex = 0;
	structUthash *pUthash = NULL;
	devlist_st *pNode = NULL, *tmp = NULL;
	int iPos = 0, iCellIdCount = 0;
	BYTE OnLineBuf[BUFFER_LEN_8] = {0};

	DEBUGMSG(1,("%s..., destLen:%d\r\n", __FUNCTION__, destLen));
	//pUthash = &stCellIdUtlist;
	pUthash = &stCellUtlist;

	pthread_mutex_lock(pUthash->ListLenLock);
	iCellIdCount = pUthash->CountUsersStr(*pUthash->StrHUsers);
	pthread_mutex_unlock(pUthash->ListLenLock);

	if ((iErrIndex = ParseErrorGetAll(argc, argv, iCellIdCount, NULL)) > 0)
	{
		if (gUserComdErr[E_SHELL_ERROR_GETCELLID].UserCmdFunc(gGetOrSetErr[iErrIndex - 1].cmd, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen) < 0)
			return (rtn = 2);
		else
			return rtn;
	}

	snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "\r\n\t%s\t\t\t\t%s\r\n",
		(char *)gCfgDataDL.CellColName.byKeyName,
		//(char *)gCfgDataDL.CellColName.byDevId,
		(char *)gCfgDataDL.CellColName.byOnline);
	iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);

	pthread_mutex_lock(pUthash->ShellRemLock);
	HASH_ITER(hh, *pUthash->StrHUsers, pNode, tmp)
	{
		//snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "\t%4d\t\t%s\r\n", 
		//	((stIdAllocTab *)pNode->val)->iId, (char *)((stIdAllocTab *)pNode->val)->szDevId);
		if (((stCellConn *)pNode->val)->CellData.bCellOnline == FALSE)
			strcpy((char *)OnLineBuf, "FALSE");
		else
			strcpy((char *)OnLineBuf, "TRUE");
		
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "\t%s\t\t%s\r\n", 
			((stCellConn *)pNode->val)->CellData.szCellId,
			(char *)OnLineBuf);

		iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);
	}
	pthread_mutex_unlock(pUthash->ShellRemLock);

	return rtn;
}

int PackGetAllDelCellId(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0, iErrIndex = 0;
	structList *pList = NULL;
	list_node_t *pNode = NULL;
	int iPos = 0, iDelCellIdCount = 0;
	int i = 0;

	DEBUGMSG(1,("%s..., destLen:%d\r\n", __FUNCTION__, destLen));
	pList = &stDelCellIdList;

	pthread_mutex_lock(pList->ListLenLock);
	iDelCellIdCount = pList->List->len;
	pthread_mutex_unlock(pList->ListLenLock);

	if ((iErrIndex = ParseErrorGetAll(argc, argv, iDelCellIdCount, NULL)) > 0)
	{
		if (gUserComdErr[E_SHELL_ERROR_GETCELLID].UserCmdFunc(gGetOrSetErr[iErrIndex - 1].cmd, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen) < 0)
			return (rtn = 2);
		else
			return rtn;
	}

	snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "\t%s\r\n",
		(char *)gCfgDataDL.DelCellIdBaColName.byCellId);
	iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);

	pthread_mutex_lock(pList->RemoveLock);
	for(i = 0;i < iDelCellIdCount;i++)
	{
		pNode = pList->ListAt(pList->List, i);
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "\t%4d\r\n", 
			((stDelIdAllocTab *)pNode->val)->iDelCellId);

		iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);
	}
	pthread_mutex_unlock(pList->RemoveLock);

	return rtn;
}

int PackGetPoolConf(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0;
	structList *pList = NULL;
	list_node_t *pNode = NULL;
	int iPos = 0, iCount = 0;
	int i = 0;

	DEBUGMSG(1,("%s..., destLen:%d\r\n", __FUNCTION__, destLen));

	//打包池的优先级
	DEBUGMSG(1,("PoolIdPri:%s, iBTCPoolCount:%d, iLTCPoolCount:%d\r\n", gpSwitchDbInfo->DbSwi.szPoolIdPri,
		gPoolDefaultConf.iBTCPoolCount, gPoolDefaultConf.iLTCPoolCount));
	if (gPoolDefaultConf.iBTCPoolCount == 0 && gPoolDefaultConf.iLTCPoolCount == 0)
	{
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "\r\n######################################\r\nPoolPri:NULL\r\n");
	}
	else
	{
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, 
			"\r\n######################################\r\nPoolPri:%s\r\n-------------------PoolDefaultConfStart---------------------\r\nBTCFixedPoolCount:%d\r\n", 
			(char *)gpSwitchDbInfo->DbSwi.szPoolIdPri,
			gPoolDefaultConf.iBTCPoolCount);
		iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);

		
		
		for (i = 0; i < gPoolDefaultConf.iBTCPoolCount && gPoolDefaultConf.iBTCPoolCount <=3; i++)
		{
			DEBUGMSG(0,("222 stBTCPoolConf@%p, iPoolId[%d]:%d\r\n", gPoolDefaultConf.stBTCPoolConf, i, gPoolDefaultConf.stBTCPoolConf[i].iPoolId));
			snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, 
				"\tid:%d\r\n\t\turl:'%s'\r\n\t\tuser:'%s'\r\n\t\tpwd:'%s'\r\n", 
				gPoolDefaultConf.stBTCPoolConf[i].iPoolId,
				gPoolDefaultConf.stBTCPoolConf[i].szUrl, 
				gPoolDefaultConf.stBTCPoolConf[i].szUser,
				gPoolDefaultConf.stBTCPoolConf[i].szPasswd
				);
			iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);
		}

		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "LTCFixedPoolCount:%d\r\n", 
			gPoolDefaultConf.iLTCPoolCount);
		iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);

		for (i = 0; i < gPoolDefaultConf.iLTCPoolCount && gPoolDefaultConf.iLTCPoolCount <=3; i++)
		{
			DEBUGMSG(0,("222 stLTCPoolConf@%p, iPoolId[%d]:%d\r\n", gPoolDefaultConf.stLTCPoolConf, i, gPoolDefaultConf.stLTCPoolConf[i].iPoolId));
			snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, 
				"\tid:%d\r\n\t\turl:'%s'\r\n\t\tuser:'%s'\r\n\t\tpwd:'%s'\r\n", 
				gPoolDefaultConf.stLTCPoolConf[i].iPoolId,
				gPoolDefaultConf.stLTCPoolConf[i].szUrl, 
				gPoolDefaultConf.stLTCPoolConf[i].szUser,
				gPoolDefaultConf.stLTCPoolConf[i].szPasswd
				);
			iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);
		}
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, 
			"-------------------PoolDefaultConfEnd-----------------------\r\n");
		
	}
	iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);	

	pList = &stBTCPoolConfList;
	pthread_mutex_lock(pList->ListLenLock);
	iCount = pList->List->len;
	pthread_mutex_unlock(pList->ListLenLock);

	snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "BTCPoolcount:%d\r\n", iCount);
	iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);

	pthread_mutex_lock(pList->RemoveLock);
	for(i = 0;i < iCount;i++)
	{
		pNode = pList->ListAt(pList->List, i);
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, 
			"\tid:%d\r\n\t\turl:'%s'\r\n\t\tuser:'%s'\r\n\t\tpwd:'%s'\r\n", 
			((stPoolConfInfo *)pNode->val)->iPoolId,
			((stPoolConfInfo *)pNode->val)->szUrl, 
			((stPoolConfInfo *)pNode->val)->szUser,
			((stPoolConfInfo *)pNode->val)->szPasswd
			);

		iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);
	}
	pthread_mutex_unlock(pList->RemoveLock);

	pList = &stLTCPoolConfList;
	pthread_mutex_lock(pList->ListLenLock);
	iCount = pList->List->len;
	pthread_mutex_unlock(pList->ListLenLock);

	snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "LTCPoolcount:%d\r\n", iCount);
	iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);

	pthread_mutex_lock(pList->RemoveLock);
	for(i = 0;i < iCount;i++)
	{
		pNode = pList->ListAt(pList->List, i);
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, 
			"\tid:%d\r\n\t\turl:'%s'\r\n\t\tuser:'%s'\r\n\t\tpwd:'%s'\r\n", 
			((stPoolConfInfo *)pNode->val)->iPoolId,
			((stPoolConfInfo *)pNode->val)->szUrl, 
			((stPoolConfInfo *)pNode->val)->szUser,
			((stPoolConfInfo *)pNode->val)->szPasswd
			);

		iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);
	}
	pthread_mutex_unlock(pList->RemoveLock);


	return rtn;
}

int PackGetDefaultPoolConf(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0;
	int iPos = 0;//, iCount = 0;
	int i = 0;

	DEBUGMSG(1,("%s... \n", __FUNCTION__));

	DEBUGMSG(1,("iBTCPoolCount:%d, iLTCPoolCount:%d\n", gPoolDefaultConf.iBTCPoolCount, gPoolDefaultConf.iLTCPoolCount));

	if (gPoolDefaultConf.iBTCPoolCount == 0 && gPoolDefaultConf.iLTCPoolCount == 0)
	{
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "\r\n######################################\r\nPoolPri:NULL\r\n");
	}
	else
	{
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, 
			"\r\n######################################\r\nPoolPri:%s\r\n-------------------PoolSpecifyConfStart---------------------\r\nBTCDefaultPoolCount:%d\r\n", 
			(char *)gpSwitchDbInfo->DbSwi.szPoolIdPri,
			gPoolDefaultConf.iBTCPoolCount);
		iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);

		
		
		for (i = 0; i < gPoolDefaultConf.iBTCPoolCount && gPoolDefaultConf.iBTCPoolCount <=3; i++)
		{
			DEBUGMSG(0,("222 stBTCPoolConf@%p, iPoolId[%d]:%d\r\n", gPoolDefaultConf.stBTCPoolConf, i, gPoolDefaultConf.stBTCPoolConf[i].iPoolId));
			snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, 
				"\tseq:%d\r\n\t\turl:'%s'\r\n\t\tuser:'%s'\r\n\t\tpwd:'%s'\r\n", 
				//gPoolDefaultConf.stBTCPoolConf[i].iPoolId,
				i,
				gPoolDefaultConf.stBTCPoolConf[i].szUrl, 
				gPoolDefaultConf.stBTCPoolConf[i].szUser,
				gPoolDefaultConf.stBTCPoolConf[i].szPasswd
				);
			iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);
		}

		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "LTCFixedPoolCount:%d\r\n", 
			gPoolDefaultConf.iLTCPoolCount);
		iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);

		for (i = 0; i < gPoolDefaultConf.iLTCPoolCount && gPoolDefaultConf.iLTCPoolCount <=3; i++)
		{
			DEBUGMSG(1,("222 stLTCPoolConf@%p, iPoolId[%d]:%d\r\n", gPoolDefaultConf.stLTCPoolConf, i, gPoolDefaultConf.stLTCPoolConf[i].iPoolId));
			snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, 
				"\tseq:%d\r\n\t\turl:'%s'\r\n\t\tuser:'%s'\r\n\t\tpwd:'%s'\r\n", 
				//gPoolDefaultConf.stLTCPoolConf[i].iPoolId,
				i,
				gPoolDefaultConf.stLTCPoolConf[i].szUrl, 
				gPoolDefaultConf.stLTCPoolConf[i].szUser,
				gPoolDefaultConf.stLTCPoolConf[i].szPasswd
				);
			iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);
		}
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, 
			"-------------------PoolDefaultConfEnd-----------------------\r\n");
		
	}
	iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);	

	return rtn;
}


int PackGetPooSpecifyConf(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0;
	int iPos = 0;//, iCount = 0;
	int i = 0;

	if (gPoolSpecifyConf.iBTCPoolCount == 0 && gPoolSpecifyConf.iLTCPoolCount == 0)
	{
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "\r\n######################################\r\nPoolPri:NULL\r\n");
	}
	else
	{
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, 
			"\r\n######################################\r\nPoolPri:%s\r\n-------------------PoolSpecifyConfStart---------------------\r\nBTCFixedPoolCount:%d\r\n", 
			(char *)gpSwitchDbInfo->DbSwi.szPoolIdPri,
			gPoolSpecifyConf.iBTCPoolCount);
		iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);

		
		
		for (i = 0; i < gPoolSpecifyConf.iBTCPoolCount && gPoolSpecifyConf.iBTCPoolCount <=3; i++)
		{
			DEBUGMSG(0,("222 stBTCPoolConf@%p, iPoolId[%d]:%d\r\n", gPoolSpecifyConf.stBTCPoolConf, i, gPoolSpecifyConf.stBTCPoolConf[i].iPoolId));
			snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, 
				"\tid:%d\r\n\t\turl:'%s'\r\n\t\tuser:'%s'\r\n\t\tpwd:'%s'\r\n", 
				gPoolSpecifyConf.stBTCPoolConf[i].iPoolId,
				gPoolSpecifyConf.stBTCPoolConf[i].szUrl, 
				gPoolSpecifyConf.stBTCPoolConf[i].szUser,
				gPoolSpecifyConf.stBTCPoolConf[i].szPasswd
				);
			iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);
		}

		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "LTCFixedPoolCount:%d\r\n", 
			gPoolSpecifyConf.iLTCPoolCount);
		iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);

		for (i = 0; i < gPoolSpecifyConf.iLTCPoolCount && gPoolSpecifyConf.iLTCPoolCount <=3; i++)
		{
			DEBUGMSG(0,("222 stLTCPoolConf@%p, iPoolId[%d]:%d\r\n", gPoolSpecifyConf.stLTCPoolConf, i, gPoolSpecifyConf.stLTCPoolConf[i].iPoolId));
			snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, 
				"\tid:%d\r\n\t\turl:'%s'\r\n\t\tuser:'%s'\r\n\t\tpwd:'%s'\r\n", 
				gPoolSpecifyConf.stLTCPoolConf[i].iPoolId,
				gPoolSpecifyConf.stLTCPoolConf[i].szUrl, 
				gPoolSpecifyConf.stLTCPoolConf[i].szUser,
				gPoolSpecifyConf.stLTCPoolConf[i].szPasswd
				);
			iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);
		}
		snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, 
			"-------------------PoolSpecifyConfEnd-----------------------\r\n");
		
	}
	iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);	

	return rtn;
}

//把用户命令setfreq打包成Command解析形式的命令
int PackCellSetFreqToComm(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0;
	char delim[] = " ";
	int PackArgc = 0;
	char *PackArgv[BUFFER_LEN_256] = {0};
	char Packbuf[BUFFER_LEN_256] = {0};
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	if(argc != COMMANDLINE_4)
	{
		gUserComdErr[E_SHELL_ERROR_SET_COMM].UserCmdFunc(NULL, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen);		
		return E_SHELL_ERROR_SET_COMM;
	}

	if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "comm cell %s %s %s %s", argv[1], argv[0], argv[2], argv[3])) < 0)
		return rtn;

	if (Parse(Packbuf, &PackArgc, PackArgv, delim) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return (rtn = 2);
	}
	
	rtn = PackComm(PackArgc, PackArgv, dest, destLen);

	return rtn;
}

//添加一个池配置信息
int PackAddPool(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0, iQueryRtn = 0, iErrIndex = 0;
	char Packbuf[BUFFER_LEN_256] = {0};
	char bufDest[BUFFER_LEN_512] = {0};
	E_ALGO eAlgo;
	BYTE timeBuf[BUFFER_LEN_256] = {0};
	int iPos = 0;
	int iCrrPoolId = 0;
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	if(argc != COMMANDLINE_6)
	{
		gUserComdErr[E_SHELL_ERROR_SET_COMM].UserCmdFunc(NULL, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen);		
		return E_SHELL_ERROR_SET_COMM;
	}

	GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
	snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen, "%s\t", (char *)timeBuf);
	iPos = strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf);

	if ((iErrIndex = ParseAddPool(argc, argv, 1, (void*)&eAlgo)) > 0)
	{
		//DEBUGMSG(1,("iErrIndex:%d, E_SHELL_ERROR_ADDPOOL:%d\r\n", iErrIndex, E_SHELL_ERROR_ADDPOOL));
		if (gUserComdErr[E_SHELL_ERROR_ADDPOOL].UserCmdFunc(gPoolConfErr[iErrIndex - 1].cmd, (char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen) < 0)
			return (rtn = E_SHELL_ERROR_ADDPOOL);
		else
			return rtn;
	}

	if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "%s,'%s','%s','%s',%d", argv[1], argv[2], argv[3], argv[4], eAlgo)) < 0)
		return rtn;
	//DEBUGMSG(1,("Packbuf:%s\r\n", Packbuf));

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
		iCrrPoolId = strtoul(argv[1], 0, 0);
		if (iCrrPoolId > gCrrMaxPoolId)
		{
			gCrrMaxPoolId = iCrrPoolId;
		}
		strcpy((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, "Addpool setting ok,please set priorities!\r\r\n");
	}
	rtn = iQueryRtn;

	return rtn;
}

//删除一个池配置信息
int PackDelPool(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0, iQueryRtn = 0, iErrIndex = 0, iQuerySwitchRtn = 0;
	char Packbuf[BUFFER_LEN_256] = {0};
	char bufDest[BUFFER_LEN_512] = {0};
	int iPoolId = 0;
	BYTE timeBuf[BUFFER_LEN_256] = {0};
	int iPos = 0;
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	if(argc != COMMANDLINE_3)
	{
		gUserComdErr[E_SHELL_ERROR_SET_COMM].UserCmdFunc(NULL, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen);		
		return E_SHELL_ERROR_SET_COMM;
	}

	GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
	snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "%s\t", (char *)timeBuf);
	iPos = strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos);

	if ((iErrIndex = ParseDelPool(argc, argv, 1, (void*)&iPoolId, 1)) > 0)
	{
		DEBUGMSG(1,("iErrIndex:%d, E_SHELL_ERROR_ADDPOOL:%d\r\n", iErrIndex, E_SHELL_ERROR_DELPOOL));
		if (gUserComdErr[E_SHELL_ERROR_DELPOOL].UserCmdFunc(gPoolConfErr[iErrIndex - 1].cmd, (char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen) < 0)
			return (rtn = E_SHELL_ERROR_DELPOOL);
		else
			return rtn;
	}

	if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "%d", iPoolId)) < 0)
		return rtn;
	DEBUGMSG(1,("Packbuf:%s\r\n", Packbuf));

	if (bIniPoolConfig)
	{
		PoolConfigQuery[E_QUERY_TYPE_DELETE - 1].QueryPackFunc1(
															Packbuf, 
															&gCfgDataDL.PoolInfo, 
															&gCfgDataDL.PoolConfigColName, 
															bufDest, 
															sizeof(bufDest));
		pthread_mutex_lock(&gPoolConfigLock);
		iQueryRtn = MysqlFunc.dbDoQuery(&myPoolConfig, bufDest, E_QUERY_TYPE_DELETE, NULL);
		pthread_mutex_unlock(&gPoolConfigLock);
		
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
				strcpy((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, "Delpool, setting ok, please set priorities!(set_pool)\r\r\n");
			else
				strcpy((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, 
				"Delpool, delete poolnodes ok, delete gPoolDefaultConf ok, failed to delete the data in the table!(switch_base)\r\r\n");
		}
		else
		{
			strcpy((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, 
			"Delpool, delete poolnodes ok, delete gPoolDefaultConf ok, failed to delete the data in the table!(pool_conf)\r\r\n");
		}
		rtn = iQueryRtn;

	}
	else
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d,  bIniPoolConfig error:%d\r\n",	
			__FILE__, __FUNCTION__, __LINE__, bIniPoolConfig);		
	}

	return rtn;
}

//设置一个池配置信息，有cell_id时就重启对应的Cell
int PackSetPool(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0, iErrIndex = 0, iQueryRtn = 0;
	//char Packbuf[BUFFER_LEN_256] = {0};
	char bufDest[BUFFER_LEN_512] = {0};
	//int iPoolId = 0;
	char bufPoolErr[BUFFER_LEN_16] = {0};
	BYTE timeBuf[BUFFER_LEN_256] = {0};
	int iPos = 0;
	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));
	if(argc != COMMANDLINE_3)
	{
		gUserComdErr[E_SHELL_ERROR_SET_COMM].UserCmdFunc(NULL, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen);		
		return E_SHELL_ERROR_SET_COMM;
	}

	GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
	snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "%s\t", (char *)timeBuf);
	iPos = strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos);

	if ((iErrIndex = ParseSetPool(argc, argv, 1, (void*)bufPoolErr)) > 0)
	{
		DEBUGMSG(1,("iErrIndex:%d, E_SHELL_ERROR_ADDPOOL:%d\r\n", iErrIndex, E_SHELL_ERROR_SETPOOL));
		if (gUserComdErr[E_SHELL_ERROR_SETPOOL].UserCmdFunc(gPoolConfErr[iErrIndex - 1].cmd, (char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen) < 0)
			return (rtn = E_SHELL_ERROR_SETPOOL);
		else
		{
			//把不存在的pool_id返回
			if (iErrIndex == E_POOLCNF_ERROR_POOLID_EXIST)
			{
				iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos);
				snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "\t\t\t(%s)\r\n", bufPoolErr);
			}
			return rtn;
		}
			
	}

	DEBUGMSG(1,("222 @%p, szPoolIdPri:%s, szSwiId:%s\r\n", gpSwitchDbInfo, (char *)gpSwitchDbInfo->DbSwi.szPoolIdPri, (char *)gpSwitchDbInfo->DbSwi.szSwiId));
	
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
		strcpy((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, "Setpool setting priorities ok!\r\r\n");
	}
	else
	{
		strcpy((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, "Setpool update databases is error!\r\r\n");
	}

	return rtn;
}

int PackSetCell(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0, i = 0, j = 0, iErrIndex = 0;
	structUthash *pList = NULL;
	int iListLen = 0;//iId = -1;
	devlist_st *pNode = NULL, *SelNode = NULL;
	BYTE timeBuf[BUFFER_LEN_256] = {0};
	int iPos = 0;
	char bufDest[BUFFER_LEN_576] = {0};

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	if(argc != COMMANDLINE_3)
	{
		gUserComdErr[E_SHELL_ERROR_SET_COMM].UserCmdFunc(NULL, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen);		
		return E_SHELL_ERROR_SET_COMM;
	}
	
	GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
	snprintf((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen, "%s\t", (char *)timeBuf);
	iPos = strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos);

	pList = &stCellUtlist;
	pthread_mutex_lock(((structList *)pList)->ListLenLock);
	iListLen = ((structUthash *)pList)->CountUsersStr(*((structUthash *)pList)->StrHUsers); 
	pthread_mutex_unlock(((structList *)pList)->ListLenLock);

	if ((iErrIndex = ParseGetOrSet(argc, argv, iListLen, pList)) > 0)
	{
		if (gUserComdErr[E_SHELL_ERROR_SETCELL].UserCmdFunc(gGetOrSetErr[iErrIndex - 1].cmd, (char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, destLen) < 0)
			return E_SHELL_ERROR_SET_COMM;
		else
			return rtn;
	}

	//先指定起始id和结束id
	memset(((structUthash *)pList)->SelCrrNode, 0, sizeof(selnode_st));
	//((structUthash *)pList)->SelCrrNode->StartId = strtoul(argv[1], 0, 0);
	strncpy(((structUthash *)pList)->SelCrrNode->szStartId, argv[1], sizeof(((structUthash *)pList)->SelCrrNode->szStartId ));
	//((structUthash *)pList)->SelCrrNode->EndId = strtoul(argv[2], 0, 0);	
	strncpy(((structUthash *)pList)->SelCrrNode->szEndId, argv[2], sizeof(((structUthash *)pList)->SelCrrNode->szEndId ));
	((structUthash *)pList)->SelCrrNode->SelCrrUsers = ((structUthash *)pList)->SelectcondStr(((structUthash *)pList)->StrHUsers);
	for(SelNode = ((structUthash *)pList)->SelCrrNode->SelCrrUsers; SelNode; SelNode = (devlist_st*)SelNode->ah.next)
	{
		((structUthash *)pList)->SelCrrNode->SelLen++;
	}
	iListLen = ((structUthash *)pList)->SelCrrNode->SelLen;	

	pNode = ((structUthash *)pList)->SelCrrNode->SelCrrUsers;
	if (pNode != NULL)
	{
		//pthread_mutex_lock(((structList *)pList)->RemoveLock);
	    for (i = 0; i < iListLen; i++)
		{
			DEBUGMSG(1,("-------iCellId:%d, szDevId:%s\r\n", 
				((stCellConn *)pNode->val)->CellData.iCellId, ((stCellConn *)pNode->val)->CellData.szDevId));
			//结点中的pool信息修改完成后，在进行打包发送结点信息
			pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.PoolConfLock);
			((stCellConn *)pNode->val)->CellPoolConf.iBTCPoolCount = 0;
			for(j = 0;j < gPoolDefaultConf.iBTCPoolCount && gPoolDefaultConf.iBTCPoolCount <= MAX_POOL_COUNT;j++)
			{	
				((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].iPoolId = gPoolDefaultConf.stBTCPoolConf[j].iPoolId;
				DEBUGMSG(0,("BTC iPoolId[%d]:%d\r\n", j, ((stCellConn *)pNode->val)->CellPoolConf.stBTCPoolConf[j].iPoolId));
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
				pthread_mutex_lock(&gCellBaseLock);
				MysqlFunc.dbDoQuery(&myCellBase, bufDest, E_QUERY_TYPE_UPDATE, NULL);
				pthread_mutex_unlock(&gCellBaseLock);
				memset(bufDest, 0, sizeof(bufDest));	
			}
			

			//iPos += strlen((char *)((stUserSendBuf*)dest)->byUserCmdBuf+iPos);
			pNode = (devlist_st*)(pNode->ah.next);
			if (pNode == NULL)
			{
				strcpy((char *)((stUserSendBuf*)dest)->byUserCmdBuf + iPos, "SetCell ok, please relogin devices!(relogin)\r\r\n");
				break;
			}	
			//printf("Cell.......iPos:%d\r\n", iPos);
		}
		((structUthash *)pList)->SelCrrNode->SelCrrUsers = pNode;		
	}
	//pthread_mutex_unlock(((structList *)pList)->RemoveLock);

	//DEBUGMSG(1,("****iPos:%d\r\n", iPos));
	return rtn;
}

//把用户命令restflash打包成Command解析形式的命令
int PackCellRestFlashToComm(int argc, char **argv, void *dest, int destLen)
{
	int rtn = 0;
	char delim[] = " ";
	int PackArgc = 0;
	char *PackArgv[BUFFER_LEN_256] = {0};
	char Packbuf[BUFFER_LEN_256] = {0};

	if(argc != COMMANDLINE_2)
	{
		gUserComdErr[E_SHELL_ERROR_SET_COMM].UserCmdFunc(NULL, (char *)((stUserSendBuf*)dest)->byUserCmdBuf, destLen);		
		return E_SHELL_ERROR_SET_COMM;
	}

	if ((rtn = snprintf(Packbuf, sizeof(Packbuf), "comm cell %s %s", argv[1], argv[0])) < 0)
		return rtn;

	if (Parse(Packbuf, &PackArgc, PackArgv, delim) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return (rtn = 2);
	}
	
	rtn = PackComm(PackArgc, PackArgv, dest, destLen);

	return rtn;
}

void ClearSendBuf(struct ap_net_connection_t *pConn)
{
	DEBUGMSG(0,("sizeof(stUserSendBuf):%d, byUserCmdBuf:%d\r\n", sizeof(stUserSendBuf), sizeof(((stUserSendBuf *)pConn->sendBuf)->byUserCmdBuf)));
	memset((char *)((stUserSendBuf *)pConn->sendBuf), 0, sizeof(stUserSendBuf));
}

void ClearUserData(struct ap_net_connection_t *pConn)
{
	DEBUGMSG(0,("sizeof(server_userdata):%d\r\n", sizeof(server_userdata)));
	memset(((server_userdata *)pConn->user_data), 0, sizeof(server_userdata));
}

int ShellParseSend(void *conn)
{
	int rtn = 0, rtnCmd = 0, i = 0, iCountI = 0;
	struct ap_net_connection_t * pConn;
	int iSendLen = 0, iCmdLen = 0;
	int argc = 0;
	char *argv[BUFFER_LEN_256] = {0};
	char delim[] = " ";
	BYTE timeBuf[BUFFER_LEN_256] = {0};
	int iPos = 0;	
	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	pConn = ((struct ap_net_connection_t *)conn);
	iSendLen = sizeof(gUserSendBuf.byUserCmdBuf);

	if (Parse((char *)((server_userdata *)pConn->user_data)->cmd, &argc, argv, delim) != 0)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
		return (rtn = -1);
	}
	
	for (i = 0; i < argc; i++)
		DEBUGMSG(1,("^^^%p, %s\r\n", *(argv+i), *(argv+i)));
	iCmdLen = dim(gUserCmd);	
	DEBUGMSG(1,("iCmdLen:%d\r\n", iCmdLen));
	for (i = 0; i < iCmdLen; i++)
	{	DEBUGMSG(0,("i:%d, argv[0]:%s, cmd:%s\r\n", i, argv[0], (char *)gUserCmd[i].cmd));
		if (strncmp(argv[0], (char *)gUserCmd[i].cmd, sizeof(gUserCmd[0].cmd)) == 0)
		{	
			do{
				DEBUGMSG(0,("2 %s...\r\n", __FUNCTION__));
				
				/*if ((rtn = gUserCmd[i].UserCmdFunc(argc, argv, (stUserSendBuf *)pConn->sendBuf, iSendLen)) != 0)
					return rtn;*/
				rtnCmd = gUserCmd[i].UserCmdFunc(argc, argv, (stUserSendBuf *)pConn->sendBuf, iSendLen);

				DEBUGMSG(0,("pConn->idx:%d, rtnCmd:%d, i:%d\r\n", pConn->idx, rtnCmd, i));
				
				stApiNetShell.ConnectionPoolSend(pConn->parent, pConn->idx, 
												(char *)((stUserSendBuf*)pConn->sendBuf)->byUserCmdBuf, strlen((char *)((stUserSendBuf *)pConn->sendBuf)->byUserCmdBuf));
				memset((char *)((stUserSendBuf *)pConn->sendBuf)->byUserCmdBuf, 0, iSendLen);
			}while(++((stUserSendBuf *)pConn->sendBuf)->iSubindex < ((stUserSendBuf *)pConn->sendBuf)->iNumPack);
			
			DEBUGMSG(0,("3 %s...\r\n", __FUNCTION__));
			((server_userdata *)pConn->user_data)->state = E_USER_STATE_ANSWER_SENT;
			DEBUGMSG(0,("4 %s...\r\n", __FUNCTION__));
		}
		else
		{
			if (++iCountI == iCmdLen)
			{

				GetLocalTimeString((char *)timeBuf, sizeof(timeBuf), GetTime());
				sprintf((char *)((stUserSendBuf*)pConn->sendBuf)->byUserCmdBuf + iPos, "%s\t", (char *)timeBuf);
				iPos = strlen((char *)((stUserSendBuf*)pConn->sendBuf)->byUserCmdBuf + iPos);				
				sprintf((char *)((stUserSendBuf*)pConn->sendBuf)->byUserCmdBuf + iPos, "%s\r\r\n", gUserComdErr[E_SHELL_ERROR_COMMAND].cmd);
				stApiNetShell.ConnectionPoolSend(pConn->parent, pConn->idx, 
												(char *)((stUserSendBuf*)pConn->sendBuf)->byUserCmdBuf, strlen((char *)((stUserSendBuf *)pConn->sendBuf)->byUserCmdBuf));				
			}
		}
		if (rtnCmd > 0)
			break;
	}

	DEBUGMSG(0,("here 1\r\n"));
	ClearUserData(pConn);
	ClearSendBuf(pConn);

	/*for (i = 0; i < argc; i++)
	{
		memset(argv+i, 0, sizeof(*argv));
	}*/

	DEBUGMSG(0,("state:%d\r\n", ((server_userdata *)pConn->user_data)->state));

	return rtn;
}

void ShellTeiminalDestroy()
{
	stApiNetShell.ConnectionPoolDestroy(tcp_pool,1);
}

int SignalConnCreated(void *conn)
{
	int rtn = 0;

	if(((struct ap_net_connection_t *)conn)->user_data == NULL)
	{
	    ((struct ap_net_connection_t *)conn)->user_data = &gUserData;					//malloc(sizeof(server_userdata) * MAX_COMMAND_BUF);
	
		if (((struct ap_net_connection_t *)conn)->user_data == NULL)
	    {
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, * !creat user_data error!\r\n", __FILE__, __FUNCTION__, __LINE__);
	        rtn = 1;
	    }
	}

	if (((struct ap_net_connection_t *)conn)->sendBuf == NULL)
	{
		((struct ap_net_connection_t *)conn)->sendBuf = &gUserSendBuf;	
		
		if (((struct ap_net_connection_t *)conn)->sendBuf == NULL)
	    {
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, * !creat sendBuf error!\r\n", __FILE__, __FUNCTION__, __LINE__);			
	        rtn = 2;
	    }
	}

	return rtn;
}

int SignalConnAccepted(void *conn)
{
	int rtn = 0;
	
	memset(((server_userdata *)((struct ap_net_connection_t *)conn)->user_data), 0, sizeof(server_userdata));


	return rtn;
}

int  SignalConnClose(void *conn)
{
	int rtn = 0;
	int i = 0;

	for (i = 0; i < MAX_CLIENTS; ++i) 			
	{
		if (((struct ap_net_connection_t *)conn) == control_conns[i])
		{
			control_conns[i] = NULL;
			memset(&gUserData, 0, sizeof(gUserData));
		}
	}
	memset(((struct ap_net_connection_t *)conn)->recebuf, 0,  ((struct ap_net_connection_t *)conn)->bufsize);
	
	return rtn;
}

//end of control connection message processing 
int SignalConnDataIn(void *conn)
{
	int rtn = 1, i = 0;

	//timeprintf();

	for (i = 0; i < MAX_CLIENTS; ++i) 					//finding is this a message from control channel? 
    {
    	DEBUGMSG(0,("%s..., i:%d, &conn:%p ?= %p\r\n", __FUNCTION__, i, conn, &control_conns[i]));
        if (((struct ap_net_connection_t *)conn) == control_conns[i])
        {
        	CommShellFlag.bUserFlag[i] = TRUE;				
        	DEBUGMSG(0,("2 **************i:%d\r\n", i));
			strncpy(((server_userdata *)((struct ap_net_connection_t *)conn)->user_data)->cmd, 
				(char *)((struct ap_net_connection_t *)conn)->recebuf, 
				sizeof(((server_userdata *)((struct ap_net_connection_t *)conn)->user_data)->cmd));

			memset(((struct ap_net_connection_t *)conn)->recebuf, 0, ((struct ap_net_connection_t *)conn)->bufsize);

			DEBUGMSG(1,("1 ##################\r\n"));
			if (ShellParseSend(conn) == 0) 
			{
				((server_userdata *)((struct ap_net_connection_t *)conn)->user_data)->state = E_USER_STATE_GOT_MESSAGE;
				rtn = 0;			
			}
			else
			{
				rtn = 2;
			}
			DEBUGMSG(0,("state:%d\r\n", ((server_userdata *)((struct ap_net_connection_t *)conn)->user_data)->state));
			DEBUGMSG(1,("2 ##################\r\n"));
        }
    }	

	return rtn;
}

//check if it is new control connection
int SignalConnUserLogin(void *conn)
{
	int rtn = 0, i = 0;
	int n = 0, iReceLen = 0;
	BYTE destBuf[BUFFER_LEN_256] = {0};
	BYTE srcBuf[BUFFER_LEN_256] = {0};

	DEBUGMSG(1,("%s...\r\n", __FUNCTION__));

	strcpy((char *)srcBuf, (char *)((struct ap_net_connection_t *)conn)->recebuf);
	memset(((struct ap_net_connection_t *)conn)->recebuf, 0, ((struct ap_net_connection_t *)conn)->bufsize);
	n = strlen(control_conn_marker);
	iReceLen = strlen((char *)srcBuf);
	
	DEBUGMSG(1,("control_conn_marker n:%d, recebuf:%s, iReceLen:%d\r\n", 
		n, (char *)srcBuf, iReceLen));
	
    if (0 == strncasecmp((char *)srcBuf, control_conn_marker, n) && iReceLen == n)
    {
    	for ( i = 0; i < MAX_CLIENTS; ++i )
    	{
    		if (control_conns[i] == NULL )
    		{
    			DEBUGMSG(1,("1 **************i:%d\r\n", i));
    			control_conns[i] = ((struct ap_net_connection_t *)conn);
				PackUserLogin(destBuf,sizeof(destBuf));
				stApiNetShell.ConnectionPoolSend(((struct ap_net_connection_t *)conn)->parent, ((struct ap_net_connection_t *)conn)->idx, (char *)destBuf, sizeof(destBuf));
    			break;
    		}
    	}
		
    	//ap_log_debug_log("USER%d##%s\r\n", i, ((struct ap_net_connection_t *)conn)->recebuf);
    	ap_utils_timespec_clear(&((struct ap_net_connection_t *)conn)->expire); 			// no expiration on control 

		return (rtn = 1);
	}
	stApiNetShell.ConnectionPoolClose(((struct ap_net_connection_t *)conn)->parent, ((struct ap_net_connection_t *)conn)->idx);

	return rtn;
}

int SignalConnTimedOut(void *conn)
{
	int rtn = 0;
	int iSendLen = 0;

	//char delim[] = " ";
	
	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));

	gUserComdErr[E_SHELL_ERROR_TIMEOUT].UserCmdFunc(NULL, byUserCmdErrbuf, sizeof(byUserCmdErrbuf));
	iSendLen = strlen((char *)byUserCmdErrbuf);

	DEBUGMSG(0,("iSendLen:%d, byUserCmdErrbuf:%s\r\n", iSendLen, byUserCmdErrbuf));
	
	stApiNetShell.ConnectionPoolSend(((struct ap_net_connection_t *)conn)->parent, ((struct ap_net_connection_t *)conn)->idx, (char *)byUserCmdErrbuf, iSendLen);

	memset(byUserCmdErrbuf, 0, sizeof(byUserCmdErrbuf));
		
	return rtn;
}

int server_callback(struct ap_net_connection_t *conn, int signal_type)
{
	DEBUGMSG(0,("\r\n%s..., signal_type:%d\r\n", __FUNCTION__, signal_type));
    switch (signal_type)
    {
        case AP_NET_SIGNAL_CONN_CREATED:
			SignalConnCreated(conn);
            break;

        case AP_NET_SIGNAL_CONN_CLOSING:
			SignalConnClose(conn);
            break;

        case AP_NET_SIGNAL_CONN_ACCEPTED:
			SignalConnAccepted(conn);
            break;

        case AP_NET_SIGNAL_CONN_MOVED_FROM:
        case AP_NET_SIGNAL_CONN_MOVED_TO:
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, MOVED???? Unimplemented!\r\n", __FILE__, __FUNCTION__, __LINE__);			
        	exit(1);
            break;

        case AP_NET_SIGNAL_CONN_DATA_IN:
        case AP_NET_SIGNAL_CONN_DATA_LEFT:
			//临时处理
			if (!gCommandType)
				gCommandType = E_COMM_TYPE_SHELL;
			if (SignalConnDataIn(conn) != 0)
			{
				SignalConnUserLogin(conn);
			}

			if (gCommandType == E_COMM_TYPE_SHELL)
				gCommandType = 0;					
            break;

        case AP_NET_SIGNAL_CONN_CAN_SEND:
            stApiNetShell.ConnectionPoolSend(conn->parent, conn->idx, (char *)help_message, help_message_len);
            break;

		case AP_NET_SIGNAL_CONN_TIMED_OUT:
			SignalConnTimedOut(conn);
            break;

		//case AP_NET_SIGNAL_CONN_PACK:
			//SignalConnPack(conn);
			//ShellParseSend(conn);
			break;
			
        default:
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, * !ERROR: Server callback: unknown signal: %d!\r\n", __FILE__, __FUNCTION__, __LINE__, signal_type);
            exit(1);
    }

    return 1;
}

void *ShellTerminalProcess(void *arg)
{
	//int i = 0;
	//int n = 0;
	//ap_net_connection_t *conn;
	//server_userdata *ud;	
	//struct ap_net_connection_t *conn;


	DEBUGMSG(1,("5 %s...\r\n\r\n", __FUNCTION__));
	
	help_message_len = strlen(help_message);

	tcp_pool = stApiNetShell.ConnectionPoolCreate(AP_NET_POOL_FLAGS_TCP, 
												MAX_CLIENTS,
												CONNECTION_TIMEOUT,
												BUFFER_LEN_256,
												server_callback);
	
    if ( tcp_pool == NULL
        || ! stApiNetShell.ConnectionPoolSetAddr(&tcp_pool->listener.addr4, INADDR_ANY, tcp_port)
        || -1 == stApiNetShell.ConnectionPoolListenerCreate(tcp_pool, 1, 1) )
    {
		GetLocalTimeLog();
		ap_log_debug_log("* !ERROR: tcp_pool: %s\r\n", ap_error_get_string());
        exit(1);
    }

	MSleep(3000);

	while(1)
	{
		//MSleep(1000);
		if (!stApiNetShell.ConnectionPoolPoll(tcp_pool))
		{
			GetLocalTimeLog();
			ap_log_debug_log("\t\t%s, %s, %d, * !ERROR: tcp_pool: %s!\r\n", __FILE__, __FUNCTION__, __LINE__, ap_error_get_string());
			//exit(1);
		}
		else
		{
			MSleep(2000);
		}
		DEBUGMSG(0,("max_connections:%d\r\n", tcp_pool->max_connections));

		/* checking for zombies first */
		/*for (i = 0; i < tcp_pool->max_connections; ++i ) 
		{	
			conn = &tcp_pool->conns[i];
			if (conn == NULL)
				continue;
			
			if (((server_userdata *)conn->user_data)->state == E_USER_STATE_GOT_MESSAGE)
			{
			DEBUGMSG(1,("\r\n----i:%d, %d ?= %d\r\n\r\n", i, ((server_userdata *)conn->user_data)->state, ((server_userdata *)tcp_pool->conns[i].user_data)->state));
			
				DEBUGMSG(1,("i:%d, %d ?= %d\r\n", i, ((server_userdata *)conn->user_data)->state, E_USER_STATE_GOT_MESSAGE));
				//ShellParseSend(conn);
				 if ( tcp_pool->callback_func != NULL )
                	tcp_pool->callback_func(conn, AP_NET_SIGNAL_CONN_PACK);
			}
		}*/
	}
}




