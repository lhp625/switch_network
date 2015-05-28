/***********************************************************************
* $Id$
* Project:	 switch_network
* File:		 alarm/alarm_pro.c		 
* Description: brief Part of shell terminal api. 
*
* @version:		V1.0.0
* @date:		30th October 2014
*
* Copyright (C) 2014 SFARDS Co., Ltd. All rights reserved.
*-----------------------------------------------------------------
*
* Revise:  
* 		 date		 version 		author 		description
*  		2014/10/30	 1.0.0		zhengchao	Creating an alarm-related functions.
*
*
*
**********************************************************************/
#include "alarm_pro.h"
#include "../app/typedefs.h"
//#include "../api_comm/api_login.h"
#include "../api_comm/def_devstr.h"
#include "../list/uthash/my_uthash.h"
#include "../db_op/db_mysql.h"

/*stCommandErr gAlarmErr[] = 
{
	{"[AlarmPro] is empty!", NULL},

};*/

/*int AlarmMiner()
{
	int iListLen = 0;
	devlist_ist *pNode = NULL, *tmp = NULL;
	//int i = 0;
	struct tm *ptm;
	char bufDest[BUFFER_LEN_512] = {0}, bufSrc[BUFFER_LEN_128] = {0};

	pthread_mutex_lock(stMinerUtlist.ListLenLock);
	iListLen = stMinerUtlist.CountUsersInt(*stMinerUtlist.HashUsers);
	pthread_mutex_unlock(stMinerUtlist.ListLenLock);
	
	if (iListLen == 0)
	{
		MSleep(100);
		return E_ALARM_ERROR_EMPTY;
	}
	
	pthread_mutex_lock(stMinerUtlist.AlarmRemLock);
	//for (i = 0; i < iListLen; i++)
	HASH_ITER(hh, *stMinerUtlist.HashUsers, pNode, tmp)
	{
		//pNode = stMinerList.ListAt(stMinerList.List, i);
		//if (pNode == NULL)
		//	continue;

		DEBUGMSG(0,("Time:%ld, AlarmTimer:%ld, bMinerOnline:%d\r\n", 
			GetTime(), ((stMinerConn *)pNode->val)->MinerData.iMinerAlarmTimer, ((stMinerConn *)pNode->val)->MinerData.bMinerOnline));

		//pthread_mutex_lock(DevAlarmTimerLock.pMinerAlarmTimerLock);
		if ((GetTime() - ((stMinerConn *)pNode->val)->MinerData.iMinerAlarmTimer) >= INTERVAL_180_SECOND)
		//if ((GetTime() - ((stMinerConn *)pNode->val)->MinerData.iMinerAlarmTimer) >= 30)//������
		{
			//pthread_mutex_unlock(DevAlarmTimerLock.pMinerAlarmTimerLock);
			DEBUGMSG(0,("11111   alarm\r\n"));
			pthread_mutex_lock(&((stMinerConn *)pNode->val)->MinerDataLock.OnlineLock);
			if ((((stMinerConn *)pNode->val)->MinerData.bMinerOnline == TRUE))
			{
				((stMinerConn *)pNode->val)->MinerData.bMinerOnline = FALSE;
				pthread_mutex_unlock(&((stMinerConn *)pNode->val)->MinerDataLock.OnlineLock);
				DEBUGMSG(0,("MinerId:%d Communication is interrupted!\r\n", ((stMinerConn *)pNode->val)->MinerData.iMinerId));
				//���µǳ�ʱ��
				((stMinerConn *)pNode->val)->MinerData.iLogoutTime = GetTime();
				//�������Ѿ��������ݿ�󣬲��ܽ��и��¡�
				if (((stMinerConn *)pNode->val)->eMinerMySql != E_QUERY_TYPE_INSERT)		
					((stMinerConn *)pNode->val)->eMinerMySql = E_QUERY_TYPE_UPDATE;

				//�����ݿ�alarm_log����в���
				//module:cell(1) miner(2);      alarm_type:ͨ���ж���1;	 level:1 2 3������
				snprintf(bufSrc, sizeof(bufSrc), "%d,%d,%d,%d,\'%s\',%ld", 
					((stMinerConn *)pNode->val)->MinerData.iMinerId,
					2,1,1,
					"Communication is interrupted!",
					((stMinerConn *)pNode->val)->MinerData.iLogoutTime);		
				AlarmLogQuery[E_QUERY_TYPE_INSERT-1].QueryPackFunc1(
								bufSrc, 
								&gCfgDataDL.AlarmInfo, 
								&gCfgDataDL.AlarmLogColName, 
								bufDest, 
								sizeof(bufDest));
				MysqlFunc.dbDoQuery(&myAlarmLog, bufDest, E_QUERY_TYPE_INSERT, NULL);
				memset(bufDest, 0, sizeof(bufDest));
			
				ptm = GetLocalTime(); 
				ap_log_debug_log("\t\t[%d-%02d-%02d %02d:%02d:%02d] MinerId:%d ͨ���ж�\r\n",
				                ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
				                ((stMinerConn *)pNode->val)->MinerData.iMinerId);
			}
			else
			{
				pthread_mutex_unlock(&((stMinerConn *)pNode->val)->MinerDataLock.OnlineLock);
			}
		}
		else
		{
			DEBUGMSG(0,("22222   alarm\r\n"));
			//pthread_mutex_unlock(DevAlarmTimerLock.pMinerAlarmTimerLock);
			pthread_mutex_lock(&((stMinerConn *)pNode->val)->MinerDataLock.OnlineLock);
			if ((((stMinerConn *)pNode->val)->MinerData.bMinerOnline == FALSE))
			{
				((stMinerConn *)pNode->val)->MinerData.bMinerOnline = TRUE;
				pthread_mutex_unlock(&((stMinerConn *)pNode->val)->MinerDataLock.OnlineLock);
				DEBUGMSG(0,("MinerId:%d Interruption recovery\r\n", ((stMinerConn *)pNode->val)->MinerData.iMinerId));
				//���µ�½ʱ��
				((stMinerConn *)pNode->val)->MinerData.iLoginTime = GetTime();
				//�������Ѿ��������ݿ�󣬲��ܽ��и��¡�
				if (((stMinerConn *)pNode->val)->eMinerMySql != E_QUERY_TYPE_INSERT)					
					((stMinerConn *)pNode->val)->eMinerMySql = E_QUERY_TYPE_UPDATE;
				ptm = GetLocalTime(); 
				ap_log_debug_log("\t\t[%d-%02d-%02d %02d:%02d:%02d] MinerId:%d �ж��ѻָ�\r\n",
				                ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
				                ((stMinerConn *)pNode->val)->MinerData.iMinerId);				
			}
			else
			{
				pthread_mutex_unlock(&((stMinerConn *)pNode->val)->MinerDataLock.OnlineLock);
			}		
		}	
	}
	pthread_mutex_unlock(stMinerUtlist.AlarmRemLock);

	return 0;
}*/

int AlarmCell()
{
	int iListLen = 0;
	devlist_st *pNode = NULL, *tmp = NULL;
	//int i = 0;
	struct tm *ptm;
	char bufDest[BUFFER_LEN_512] = {0}, bufSrc[BUFFER_LEN_128] = {0};

	DEBUGMSG(0,("%s...\r\n", __FUNCTION__));

	pthread_mutex_lock(stCellUtlist.ListLenLock);
	iListLen = stCellUtlist.CountUsersStr(*stCellUtlist.StrHUsers);
	pthread_mutex_unlock(stCellUtlist.ListLenLock);

	DEBUGMSG(0,("iListLen:%d\r\n", iListLen));
	
	if (iListLen == 0)
	{
		MSleep(100);
		return E_ALARM_ERROR_EMPTY;
	}

	pthread_mutex_lock(stCellUtlist.AlarmRemLock);
	//for (i = 0; i < iListLen; i++)
	HASH_ITER(hh, *stCellUtlist.StrHUsers, pNode, tmp)
	{
		DEBUGMSG(0,("Time:%ld, AlarmTimer:%ld, bCellOnline:%d\r\n", 
			GetTime(), ((stCellConn *)pNode->val)->CellData.iCellAlarmTimer, ((stCellConn *)pNode->val)->CellData.bCellOnline));
		
		//pNode = stCellList.ListAt(stCellList.List, i);
		//if (pNode == NULL)
		//	continue;
		DEBUGMSG(0,("22222   alarm\r\n"));
		//pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.AlarmTimerLock);
		if ((GetTime() - ((stCellConn *)pNode->val)->CellData.iCellAlarmTimer) > INTERVAL_180_SECOND)
		//if ((GetTime() - ((stCellConn *)pNode->val)->CellData.iCellAlarmTimer) > 30)//������
		{			
			DEBUGMSG(0,("33333   alarm\r\n"));
			//pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.AlarmTimerLock);
			pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
			if (((stCellConn *)pNode->val)->CellData.bCellOnline == TRUE)
			{
				((stCellConn *)pNode->val)->CellData.bCellOnline = FALSE;
				pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
				
				DEBUGMSG(0,("CellId:%s Communication is interrupted!\r\n", ((stCellConn *)pNode->val)->CellData.szCellId));
				//���µǳ�ʱ��
				((stCellConn *)pNode->val)->CellData.iLogoutTime = GetTime();

				//�������Ѿ��������ݿ�󣬲��ܽ��и��¡�
				if (((stCellConn *)pNode->val)->eCellMySql != E_QUERY_TYPE_INSERT)				
					((stCellConn *)pNode->val)->eCellMySql = E_QUERY_TYPE_UPDATE;
				
				//�����ݿ�alarm_log����в���
				//module:cell(1) miner(2);      alarm_type:ͨ���ж���1;	 level:1 2 3������
				snprintf(bufSrc, sizeof(bufSrc), "\'%s\',%d,%d,%d,\'%s\',%ld", 
					(char *)((stCellConn *)pNode->val)->CellData.szCellId,
					1,1,1,
					"Communication is interrupted!",
					((stCellConn *)pNode->val)->CellData.iLogoutTime);		
				AlarmLogQuery[E_QUERY_TYPE_INSERT-1].QueryPackFunc1(
								bufSrc, 
								&gCfgDataDL.AlarmInfo, 
								&gCfgDataDL.AlarmLogColName, 
								bufDest, 
								sizeof(bufDest));
				pthread_mutex_lock(&gAlarmLogLock);
				MysqlFunc.dbDoQuery(&myAlarmLog, bufDest, E_QUERY_TYPE_INSERT, NULL);
				pthread_mutex_unlock(&gAlarmLogLock);
				memset(bufDest, 0, sizeof(bufDest));

				ptm = GetLocalTime(); 
				//ap_log_debug_log
				ap_log_debug_log("\t\t[%d-%02d-%02d %02d:%02d:%02d] CellId:%s ͨ���ж�\r\n",
								ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
								((stCellConn *)pNode->val)->CellData.szCellId);


			}
			else
			{
				pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
			}
		}
		else
		{
			DEBUGMSG(0,("44444   alarm\r\n"));
			//pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.AlarmTimerLock);
			pthread_mutex_lock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
			if ((((stCellConn *)pNode->val)->CellData.bCellOnline == FALSE))
			{
				((stCellConn *)pNode->val)->CellData.bCellOnline = TRUE;
				pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
				
				DEBUGMSG(0,("CellId:%s Interruption recovery\r\n", ((stCellConn *)pNode->val)->CellData.szCellId));
				//���µ�½ʱ��
				((stCellConn *)pNode->val)->CellData.iLoginTime = GetTime();
				//�������Ѿ��������ݿ�󣬲��ܽ��и��¡�
				if (((stCellConn *)pNode->val)->eCellMySql != E_QUERY_TYPE_INSERT)			
					((stCellConn *)pNode->val)->eCellMySql = E_QUERY_TYPE_UPDATE;
				ptm = GetLocalTime();
				//ap_log_debug_log
				ap_log_debug_log("\t\t[%d-%02d-%02d %02d:%02d:%02d] CellId:%s �ж��ѻָ�\r\n",
								ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
								((stCellConn *)pNode->val)->CellData.szCellId);	
			}
			else
			{
				pthread_mutex_unlock(&((stCellConn *)pNode->val)->CellDataLock.OnlineLock);
			}		
		}
	}
	pthread_mutex_unlock(stCellUtlist.AlarmRemLock);

	return 0;
}

void *AlarmProcess(void *arg)
{
	//int rtn = 0;
	
	DEBUGMSG(1,("6 %s...\r\n\r\n", __FUNCTION__));

	arg = arg;
	
	while(1)
	{
		//AlarmMiner();
		AlarmCell();
		MSleep(10000);
	}
}

