/***********************************************************************
* $Id$
* Project:	 switch_network
* File:download/download.c		 
* Description: brief Part of read configure file. 
*
* @version:		V1.0.0
* @date:		4th September 2014
*
* Copyright (C) 2014 SFARDS Co., Ltd. All rights reserved.
*-----------------------------------------------------------------
*
* Revise:  
* 		 date		 version 		author 		description
*  		2014/09/04	 1.0.0		zhengchao	create read CfgData.ini
*
*
*
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "./download.h"
#include "../app/debug.h"

const stFindHeadInfo cFindHeadInfo[] =
{
	{"[SwitchInfo]", ParseSwitchInfo},
	{"[MulticastInfo]", ParseMulticastInfo},
	{"[MonitorInfo]", ParseMonitorInfo},	
	//{"[CellInfo]", ParseCellInfo},
	//{"[DividInfo]", ParseDividInfo},
	//{"[Muticast]", ParseMuticast},
	//{"[MinerBaseDb]", ParseMinerBaseDbName},
	{"[CellBaseDb]", ParseCellBaseDbName},
	//{"[MinerStatusDb]", ParseMinerStatusDbName},
	{"[CellStatusDb]", ParseCellStatusDbName},
	{"[SwitchBaseDb]", ParseSwitchBaseDbName},
	{"[PoolConfigDb]", ParsePoolConfigDbName},
	{"[AlarmLogDb]", ParseAlarmLogDbName},
	{"[DelCellIdBaseDb]", ParseCellIdBaseDbName},
	//{"[MinerColumnName]", ParseMinerBaseColumName},
	{"[CellColumnName]", ParseCellBaseColumName},
	//{"[MinerStatusColumnName]", ParseMinerStatusColumName},
	{"[CellStatusColumnName]", ParseCellStatusColumName},
	{"[SwitchBaseColumnName]", ParseSwitchBaseColumnName},
	{"[PoolConfigColumnName]", ParsePoolConfigColumnName},
	{"[AlarmLogColumnName]", ParseAlarmLogColumnName},
	{"[DelCellIdBaseColumnName]", ParseDelCellIdBaseColumnName}
};

stSwitchDbInfo *gpSwitchDbInfo = NULL;

int ParseSwitchInfo(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;
	char bufMacDest[BUFFER_LEN_64] = "0";
	char szNameBuf[BUFFER_LEN_64] = "0";
	
	DEBUGMSG(0, ("1 %s...\r\n", __FUNCTION__));

	if (argc < iStartPos || argc - 1 < atoi(argv[0]))
		return (rtn = 1);
	
	index = iStartPos;
	//gCfgDataDL.SwitchInfo.iSwiId = atoi(argv[index++]);

	strncpy(szNameBuf, argv[index++], sizeof(szNameBuf));
	
	//gCfgDataDL.SwitchInfo.iNum = atoi(argv[index++]);
	//gCfgDataDL.SwitchInfo.iCellNum = atoi(argv[index++]);
	gCfgDataDL.SwitchInfo.iStatsInter = atoi(argv[index++]);
	strncpy((char *)gCfgDataDL.SwitchInfo.szServerIp, argv[index++], sizeof(gCfgDataDL.SwitchInfo.szServerIp));
	gCfgDataDL.SwitchInfo.iServerPort = atoi(argv[index++]);

	get_mac(bufMacDest);
	if (strlen(bufMacDest) != MAC_LEN)
		snprintf((char *)gCfgDataDL.SwitchInfo.szSwiId, sizeof(gCfgDataDL.SwitchInfo.szSwiId), "112233445566&%s", szNameBuf);
	else
		snprintf((char *)gCfgDataDL.SwitchInfo.szSwiId, sizeof(gCfgDataDL.SwitchInfo.szSwiId), "%s&%s", bufMacDest, szNameBuf);

	DEBUGMSG(1,("szSwiId:%s, iStatsInter:%d\r\n", gCfgDataDL.SwitchInfo.szSwiId, gCfgDataDL.SwitchInfo.iStatsInter));

	
	gpSwitchDbInfo = malloc(sizeof(stSwitchDbInfo));
	memset(gpSwitchDbInfo, 0, sizeof(stSwitchDbInfo));

	return rtn;
}

int ParseMulticastInfo(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;

	DEBUGMSG(0, ("1 %s...\r\n", __FUNCTION__));

	if (argc < iStartPos || argc - 1 < atoi(argv[0]))
	return (rtn = 1);

	index = iStartPos;

	strncpy((char *)gCfgDataDL.CellMulticast.szMulticastIp, argv[index++], sizeof(gCfgDataDL.CellMulticast.szMulticastIp));
	gCfgDataDL.CellMulticast.iMulticastPort = atoi(argv[index++]);

	return rtn;
}

int ParseMonitorInfo(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;
	
	DEBUGMSG(0, ("1 %s...\r\n", __FUNCTION__));

	if (argc < iStartPos || argc - 1 < atoi(argv[0]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.MonitorInfo.szMoniIp, argv[index++], sizeof(gCfgDataDL.MonitorInfo.szMoniIp));
	//strncpy((char *)gCfgDataDL.MonitorInfo.szMoniPort, argv[index++], sizeof(gCfgDataDL.MonitorInfo.szMoniPort));
	gCfgDataDL.MonitorInfo.iMoniPort = atoi(argv[index++]);
	strncpy((char *)gCfgDataDL.MonitorInfo.szToken, argv[index++], sizeof(gCfgDataDL.MonitorInfo.szToken));
	DEBUGMSG(1,("szMoniIp:%s, szMoniPort:%d, szToken:%s\r\n", 
		(char *)gCfgDataDL.MonitorInfo.szMoniIp, gCfgDataDL.MonitorInfo.iMoniPort,
		(char *)gCfgDataDL.MonitorInfo.szToken));
	

	return rtn;
}


int ParseCellInfo(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;

	DEBUGMSG(0, ("1 %s...\r\n", __FUNCTION__));

	if (argc < iStartPos || argc - 1 < atoi(argv[0]))
		return (rtn = 1);
	
	index = iStartPos;
	gCfgDataDL.CellInfo.iNum= atoi(argv[index++]);

	return rtn;
}

/*int ParseDividInfo(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;
	
	DEBUGMSG(0, ("1 %s...\r\n", __FUNCTION__));
	
	if (argc < iStartPos || argc - 1 < atoi(argv[0]))
		return (rtn = 1);
	
	index = iStartPos;
	
	gCfgDataDL.DLDivid.bDividDisable = atoi(argv[index++]);
	gCfgDataDL.DLDivid.iStart = atoi(argv[index++]);
	gCfgDataDL.DLDivid.iEnd = atoi(argv[index++]);
	gCfgDataDL.DLDivid.iTotal = atoi(argv[index++]);
	gCfgDataDL.DLDivid.iShare = atoi(argv[index++]);

	//printf("!!!!!!!!!!!!!!!!%d %d %d %d %d\r\n", 
	//	gCfgDataDL.DLDivid.bDividDisable, gCfgDataDL.DLDivid.iStart, 
	//	gCfgDataDL.DLDivid.iEnd, gCfgDataDL.DLDivid.iTotal, gCfgDataDL.DLDivid.iShare);

	return rtn;
}*/

int ParseMuticast(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.DLWb.byWbIp, argv[index++], sizeof(gCfgDataDL.DLWb.byWbIp));
	strncpy((char *)gCfgDataDL.DLWb.byWbPort, argv[index++], sizeof(gCfgDataDL.DLWb.byWbPort));
	
	//printf("%s %s\r\n", 
	//	(char *)gCfgDataDL.DLWb.byWbIp, (char *)gCfgDataDL.DLWb.byWbPort);
	
	return rtn;
}	

int ParseMinerBaseDbName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.MinerBaseInfo.myServerIp, argv[index++], sizeof(gCfgDataDL.MinerBaseInfo.myServerIp));
	strncpy((char *)gCfgDataDL.MinerBaseInfo.myUserName, argv[index++], sizeof(gCfgDataDL.MinerBaseInfo.myUserName));
	strncpy((char *)gCfgDataDL.MinerBaseInfo.myUserPw, argv[index++], sizeof(gCfgDataDL.MinerBaseInfo.myUserPw));
	strncpy((char *)gCfgDataDL.MinerBaseInfo.myDbName, argv[index++], sizeof(gCfgDataDL.MinerBaseInfo.myDbName));
	strncpy((char *)gCfgDataDL.MinerBaseInfo.myTableName, argv[index++], sizeof(gCfgDataDL.MinerBaseInfo.myTableName));

	return rtn;
}

int ParseCellBaseDbName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));
	
	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.CellBaseInfo.myServerIp, argv[index++], sizeof(gCfgDataDL.CellBaseInfo.myServerIp));
	strncpy((char *)gCfgDataDL.CellBaseInfo.myUserName, argv[index++], sizeof(gCfgDataDL.CellBaseInfo.myUserName));
	strncpy((char *)gCfgDataDL.CellBaseInfo.myUserPw, argv[index++], sizeof(gCfgDataDL.CellBaseInfo.myUserPw));
	strncpy((char *)gCfgDataDL.CellBaseInfo.myDbName, argv[index++], sizeof(gCfgDataDL.CellBaseInfo.myDbName));
	strncpy((char *)gCfgDataDL.CellBaseInfo.myTableName, argv[index++], sizeof(gCfgDataDL.CellBaseInfo.myTableName));

	return rtn;
}

int ParseMinerStatusDbName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.MinerStatusInfo.myServerIp, argv[index++], sizeof(gCfgDataDL.MinerStatusInfo.myServerIp));
	strncpy((char *)gCfgDataDL.MinerStatusInfo.myUserName, argv[index++], sizeof(gCfgDataDL.MinerStatusInfo.myUserName));
	strncpy((char *)gCfgDataDL.MinerStatusInfo.myUserPw, argv[index++], sizeof(gCfgDataDL.MinerStatusInfo.myUserPw));
	strncpy((char *)gCfgDataDL.MinerStatusInfo.myDbName, argv[index++], sizeof(gCfgDataDL.MinerStatusInfo.myDbName));
	strncpy((char *)gCfgDataDL.MinerStatusInfo.myTableName, argv[index++], sizeof(gCfgDataDL.MinerStatusInfo.myTableName));

	return rtn;
}

int ParseCellStatusDbName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));
	
	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.CellStatusInfo.myServerIp, argv[index++], sizeof(gCfgDataDL.CellStatusInfo.myServerIp));
	strncpy((char *)gCfgDataDL.CellStatusInfo.myUserName, argv[index++], sizeof(gCfgDataDL.CellStatusInfo.myUserName));
	strncpy((char *)gCfgDataDL.CellStatusInfo.myUserPw, argv[index++], sizeof(gCfgDataDL.CellStatusInfo.myUserPw));
	strncpy((char *)gCfgDataDL.CellStatusInfo.myDbName, argv[index++], sizeof(gCfgDataDL.CellStatusInfo.myDbName));
	strncpy((char *)gCfgDataDL.CellStatusInfo.myTableName, argv[index++], sizeof(gCfgDataDL.CellStatusInfo.myTableName));

	return rtn;
}

int ParseSwitchBaseDbName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));
	
	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.SwitchBaseInfo.myServerIp, argv[index++], sizeof(gCfgDataDL.SwitchBaseInfo.myServerIp));
	strncpy((char *)gCfgDataDL.SwitchBaseInfo.myUserName, argv[index++], sizeof(gCfgDataDL.SwitchBaseInfo.myUserName));
	strncpy((char *)gCfgDataDL.SwitchBaseInfo.myUserPw, argv[index++], sizeof(gCfgDataDL.SwitchBaseInfo.myUserPw));
	strncpy((char *)gCfgDataDL.SwitchBaseInfo.myDbName, argv[index++], sizeof(gCfgDataDL.SwitchBaseInfo.myDbName));
	strncpy((char *)gCfgDataDL.SwitchBaseInfo.myTableName, argv[index++], sizeof(gCfgDataDL.SwitchBaseInfo.myTableName));

	return rtn;
}

int ParsePoolConfigDbName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));
	
	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.PoolInfo.myServerIp, argv[index++], sizeof(gCfgDataDL.PoolInfo.myServerIp));
	strncpy((char *)gCfgDataDL.PoolInfo.myUserName, argv[index++], sizeof(gCfgDataDL.PoolInfo.myUserName));
	strncpy((char *)gCfgDataDL.PoolInfo.myUserPw, argv[index++], sizeof(gCfgDataDL.PoolInfo.myUserPw));
	strncpy((char *)gCfgDataDL.PoolInfo.myDbName, argv[index++], sizeof(gCfgDataDL.PoolInfo.myDbName));
	strncpy((char *)gCfgDataDL.PoolInfo.myTableName, argv[index++], sizeof(gCfgDataDL.PoolInfo.myTableName));

	return rtn;
}

int ParseAlarmLogDbName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));
	
	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.AlarmInfo.myServerIp, argv[index++], sizeof(gCfgDataDL.AlarmInfo.myServerIp));
	strncpy((char *)gCfgDataDL.AlarmInfo.myUserName, argv[index++], sizeof(gCfgDataDL.AlarmInfo.myUserName));
	strncpy((char *)gCfgDataDL.AlarmInfo.myUserPw, argv[index++], sizeof(gCfgDataDL.AlarmInfo.myUserPw));
	strncpy((char *)gCfgDataDL.AlarmInfo.myDbName, argv[index++], sizeof(gCfgDataDL.AlarmInfo.myDbName));
	strncpy((char *)gCfgDataDL.AlarmInfo.myTableName, argv[index++], sizeof(gCfgDataDL.AlarmInfo.myTableName));

	return rtn;
}

int ParseCellIdBaseDbName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));
	
	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.DelCellIdBaseInfo.myServerIp, argv[index++], sizeof(gCfgDataDL.DelCellIdBaseInfo.myServerIp));
	strncpy((char *)gCfgDataDL.DelCellIdBaseInfo.myUserName, argv[index++], sizeof(gCfgDataDL.DelCellIdBaseInfo.myUserName));
	strncpy((char *)gCfgDataDL.DelCellIdBaseInfo.myUserPw, argv[index++], sizeof(gCfgDataDL.DelCellIdBaseInfo.myUserPw));
	strncpy((char *)gCfgDataDL.DelCellIdBaseInfo.myDbName, argv[index++], sizeof(gCfgDataDL.DelCellIdBaseInfo.myDbName));
	strncpy((char *)gCfgDataDL.DelCellIdBaseInfo.myTableName, argv[index++], sizeof(gCfgDataDL.DelCellIdBaseInfo.myTableName));

	return rtn;
}


int ParseMinerBaseColumName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.MinerColName.byKeyName, argv[index++], sizeof(gCfgDataDL.MinerColName.byKeyName));
	strncpy((char *)gCfgDataDL.MinerColName.byMinerIp, argv[index++], sizeof(gCfgDataDL.MinerColName.byMinerIp));
	strncpy((char *)gCfgDataDL.MinerColName.byMinerPort, argv[index++], sizeof(gCfgDataDL.MinerColName.byMinerPort));
	strncpy((char *)gCfgDataDL.MinerColName.byAlgo, argv[index++], sizeof(gCfgDataDL.MinerColName.byAlgo));
	strncpy((char *)gCfgDataDL.MinerColName.byOnline, argv[index++], sizeof(gCfgDataDL.MinerColName.byOnline));
	strncpy((char *)gCfgDataDL.MinerColName.byBTCWbsId, argv[index++], sizeof(gCfgDataDL.MinerColName.byBTCWbsId));
	//strncpy((char *)gCfgDataDL.MinerColName.byBTCCellNum, argv[index++], sizeof(gCfgDataDL.MinerColName.byBTCCellNum));
	strncpy((char *)gCfgDataDL.MinerColName.byLTCWbsId, argv[index++], sizeof(gCfgDataDL.MinerColName.byLTCWbsId));
	//strncpy((char *)gCfgDataDL.MinerColName.byLTCCellNum, argv[index++], sizeof(gCfgDataDL.MinerColName.byLTCCellNum));
	strncpy((char *)gCfgDataDL.MinerColName.byRegTime, argv[index++], sizeof(gCfgDataDL.MinerColName.byRegTime));
	strncpy((char *)gCfgDataDL.MinerColName.byLoginTime, argv[index++], sizeof(gCfgDataDL.MinerColName.byLoginTime));
	strncpy((char *)gCfgDataDL.MinerColName.byLogoutTime, argv[index++], sizeof(gCfgDataDL.MinerColName.byLogoutTime));
	strncpy((char *)gCfgDataDL.MinerColName.byPool1Id, argv[index++], sizeof(gCfgDataDL.MinerColName.byPool1Id));
	strncpy((char *)gCfgDataDL.MinerColName.byPool2Id, argv[index++], sizeof(gCfgDataDL.MinerColName.byPool2Id));
	strncpy((char *)gCfgDataDL.MinerColName.byPool3Id, argv[index++], sizeof(gCfgDataDL.MinerColName.byPool3Id));
	strncpy((char *)gCfgDataDL.MinerColName.byPath, argv[index++], sizeof(gCfgDataDL.MinerColName.byPath));

	return rtn;
}

int ParseCellBaseColumName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.CellColName.byKeyName, argv[index++], sizeof(gCfgDataDL.CellColName.byKeyName));
	//strncpy((char *)gCfgDataDL.CellColName.byDevId, argv[index++], sizeof(gCfgDataDL.CellColName.byDevId));
	strncpy((char *)gCfgDataDL.CellColName.byCellIp, argv[index++], sizeof(gCfgDataDL.CellColName.byCellIp));
	strncpy((char *)gCfgDataDL.CellColName.byCellPort, argv[index++], sizeof(gCfgDataDL.CellColName.byCellPort));
	strncpy((char *)gCfgDataDL.CellColName.bySwVer, argv[index++], sizeof(gCfgDataDL.CellColName.bySwVer));
	strncpy((char *)gCfgDataDL.CellColName.byHwVer, argv[index++], sizeof(gCfgDataDL.CellColName.byHwVer));
	strncpy((char *)gCfgDataDL.CellColName.byAlgo, argv[index++], sizeof(gCfgDataDL.CellColName.byAlgo));
	strncpy((char *)gCfgDataDL.CellColName.byChipCnt, argv[index++], sizeof(gCfgDataDL.CellColName.byChipCnt));
	strncpy((char *)gCfgDataDL.CellColName.byChipBTCHash, argv[index++], sizeof(gCfgDataDL.CellColName.byChipBTCHash));
	strncpy((char *)gCfgDataDL.CellColName.byChipLTCHash, argv[index++], sizeof(gCfgDataDL.CellColName.byChipLTCHash));
	strncpy((char *)gCfgDataDL.CellColName.byBTCFreq, argv[index++], sizeof(gCfgDataDL.CellColName.byBTCFreq));
	strncpy((char *)gCfgDataDL.CellColName.byLTCFreq, argv[index++], sizeof(gCfgDataDL.CellColName.byLTCFreq));	
	strncpy((char *)gCfgDataDL.CellColName.byOnline, argv[index++], sizeof(gCfgDataDL.CellColName.byOnline));
	strncpy((char *)gCfgDataDL.CellColName.szPoolList, argv[index++], sizeof(gCfgDataDL.CellColName.szPoolList));
	strncpy((char *)gCfgDataDL.CellColName.byTestResult, argv[index++], sizeof(gCfgDataDL.CellColName.byTestResult));

	//strncpy((char *)gCfgDataDL.CellColName.byBTCWbsId, argv[index++], sizeof(gCfgDataDL.CellColName.byBTCWbsId));
	//strncpy((char *)gCfgDataDL.CellColName.byBTCWbcId, argv[index++], sizeof(gCfgDataDL.CellColName.byBTCWbcId));
	//strncpy((char *)gCfgDataDL.CellColName.byBTCCpm, argv[index++], sizeof(gCfgDataDL.CellColName.byBTCCpm));
	//strncpy((char *)gCfgDataDL.CellColName.byBTCWbIp, argv[index++], sizeof(gCfgDataDL.CellColName.byBTCWbIp));
	//strncpy((char *)gCfgDataDL.CellColName.byBTCWbPort, argv[index++], sizeof(gCfgDataDL.CellColName.byBTCWbPort));
	//strncpy((char *)gCfgDataDL.CellColName.byLTCWbsId, argv[index++], sizeof(gCfgDataDL.CellColName.byLTCWbsId));
	//strncpy((char *)gCfgDataDL.CellColName.byLTCWbcId, argv[index++], sizeof(gCfgDataDL.CellColName.byLTCWbcId));
	//strncpy((char *)gCfgDataDL.CellColName.byLTCCpm, argv[index++], sizeof(gCfgDataDL.CellColName.byLTCCpm));
	//strncpy((char *)gCfgDataDL.CellColName.byLTCWbIp, argv[index++], sizeof(gCfgDataDL.CellColName.byLTCWbIp));
	//strncpy((char *)gCfgDataDL.CellColName.byLTCWbPort, argv[index++], sizeof(gCfgDataDL.CellColName.byLTCWbPort));
	strncpy((char *)gCfgDataDL.CellColName.byRegTime, argv[index++], sizeof(gCfgDataDL.CellColName.byRegTime));
	strncpy((char *)gCfgDataDL.CellColName.byLoginTime, argv[index++], sizeof(gCfgDataDL.CellColName.byLoginTime));
	strncpy((char *)gCfgDataDL.CellColName.byLogoutTime, argv[index++], sizeof(gCfgDataDL.CellColName.byLogoutTime));

	strncpy((char *)gCfgDataDL.CellColName.szBTC30MNcsend, argv[index++], sizeof(gCfgDataDL.CellColName.szBTC30MNcsend));
	strncpy((char *)gCfgDataDL.CellColName.szLTC30MNcsend, argv[index++], sizeof(gCfgDataDL.CellColName.szLTC30MNcsend));

	DEBUGMSG(1,("1 szPoolList:%s\r\n", (char *)gCfgDataDL.CellColName.szPoolList));

	return rtn;
}

int ParseMinerStatusColumName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;
	
	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.MinerStaColName.byMinerId, argv[index++], sizeof(gCfgDataDL.MinerStaColName.byMinerId));
	strncpy((char *)gCfgDataDL.MinerStaColName.byElapse, argv[index++], sizeof(gCfgDataDL.MinerStaColName.byElapse));
	strncpy((char *)gCfgDataDL.MinerStaColName.byCurPoolId, argv[index++], sizeof(gCfgDataDL.MinerStaColName.byCurPoolId));
	strncpy((char *)gCfgDataDL.MinerStaColName.byCurPoolStat, argv[index++], sizeof(gCfgDataDL.MinerStaColName.byCurPoolStat));
	strncpy((char *)gCfgDataDL.MinerStaColName.byUpdateTime, argv[index++], sizeof(gCfgDataDL.MinerStaColName.byUpdateTime));
	
	return rtn;
}

int ParseCellStatusColumName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;
	
	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.CellStaColName.byCellId, argv[index++], sizeof(gCfgDataDL.CellStaColName.byCellId));
	//strncpy((char *)gCfgDataDL.CellStaColName.byTestResult, argv[index++], sizeof(gCfgDataDL.CellStaColName.byTestResult));
	strncpy((char *)gCfgDataDL.CellStaColName.byEntryTemp, argv[index++], sizeof(gCfgDataDL.CellStaColName.byEntryTemp));
	strncpy((char *)gCfgDataDL.CellStaColName.byExitTemp, argv[index++], sizeof(gCfgDataDL.CellStaColName.byExitTemp));
	strncpy((char *)gCfgDataDL.CellStaColName.byLedStatus, argv[index++], sizeof(gCfgDataDL.CellStaColName.byLedStatus));
	strncpy((char *)gCfgDataDL.CellStaColName.byElapse, argv[index++], sizeof(gCfgDataDL.CellStaColName.byElapse));
	//strncpy((char *)gCfgDataDL.CellStaColName.byBtcWblost, argv[index++], sizeof(gCfgDataDL.CellStaColName.byBtcWblost));
	strncpy((char *)gCfgDataDL.CellStaColName.byBtcNcsend, argv[index++], sizeof(gCfgDataDL.CellStaColName.byBtcNcsend));
	//strncpy((char *)gCfgDataDL.CellStaColName.byBtcHwerr, argv[index++], sizeof(gCfgDataDL.CellStaColName.byBtcHwerr));
	strncpy((char *)gCfgDataDL.CellStaColName.byBtcAccept, argv[index++], sizeof(gCfgDataDL.CellStaColName.byBtcAccept));
	strncpy((char *)gCfgDataDL.CellStaColName.byBtcReject, argv[index++], sizeof(gCfgDataDL.CellStaColName.byBtcReject));
	//strncpy((char *)gCfgDataDL.CellStaColName.byBtcDiff, argv[index++], sizeof(gCfgDataDL.CellStaColName.byBtcDiff));
	//strncpy((char *)gCfgDataDL.CellStaColName.byLtcWblost, argv[index++], sizeof(gCfgDataDL.CellStaColName.byLtcWblost));
	strncpy((char *)gCfgDataDL.CellStaColName.byLtcNcsend, argv[index++], sizeof(gCfgDataDL.CellStaColName.byLtcNcsend));
	//strncpy((char *)gCfgDataDL.CellStaColName.byLtcHwerr, argv[index++], sizeof(gCfgDataDL.CellStaColName.byLtcHwerr));
	strncpy((char *)gCfgDataDL.CellStaColName.byLtcAccept, argv[index++], sizeof(gCfgDataDL.CellStaColName.byLtcAccept));
	strncpy((char *)gCfgDataDL.CellStaColName.byLtcReject, argv[index++], sizeof(gCfgDataDL.CellStaColName.byLtcReject));
	//strncpy((char *)gCfgDataDL.CellStaColName.byLtcDiff, argv[index++], sizeof(gCfgDataDL.CellStaColName.byLtcDiff));
	strncpy((char *)gCfgDataDL.CellStaColName.byChipsTemp, argv[index++], sizeof(gCfgDataDL.CellStaColName.byChipsTemp));
	strncpy((char *)gCfgDataDL.CellStaColName.byUpdateTime, argv[index++], sizeof(gCfgDataDL.CellStaColName.byUpdateTime));
	
	return rtn;
}

int ParseSwitchBaseColumnName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;
	
	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.SwitchBaseColName.bySwitchId, argv[index++], sizeof(gCfgDataDL.SwitchBaseColName.bySwitchId));
	//strncpy((char *)gCfgDataDL.SwitchBaseColName.byAllocPolicy, argv[index++], sizeof(gCfgDataDL.SwitchBaseColName.byAllocPolicy));
	//strncpy((char *)gCfgDataDL.SwitchBaseColName.byMaxAllocsize, argv[index++], sizeof(gCfgDataDL.SwitchBaseColName.byMaxAllocsize));
	//strncpy((char *)gCfgDataDL.SwitchBaseColName.byCpm, argv[index++], sizeof(gCfgDataDL.SwitchBaseColName.byCpm));
	strncpy((char *)gCfgDataDL.SwitchBaseColName.byStatusInterval, argv[index++], sizeof(gCfgDataDL.SwitchBaseColName.byStatusInterval));
	strncpy((char *)gCfgDataDL.SwitchBaseColName.byPoolidPri, argv[index++], sizeof(gCfgDataDL.SwitchBaseColName.byPoolidPri));
	//strncpy((char *)gCfgDataDL.SwitchBaseColName.byWbipBase, argv[index++], sizeof(gCfgDataDL.SwitchBaseColName.byWbipBase));
	//strncpy((char *)gCfgDataDL.SwitchBaseColName.byWbPort, argv[index++], sizeof(gCfgDataDL.SwitchBaseColName.byWbPort));
	strncpy((char *)gCfgDataDL.SwitchBaseColName.byUpdateTime, argv[index++], sizeof(gCfgDataDL.SwitchBaseColName.byUpdateTime));
	//strncpy((char *)gCfgDataDL.SwitchBaseColName.byNextCellid, argv[index++], sizeof(gCfgDataDL.SwitchBaseColName.byNextCellid));

	return rtn;
}

int ParsePoolConfigColumnName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;
	
	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.PoolConfigColName.byPoolId, argv[index++], sizeof(gCfgDataDL.PoolConfigColName.byPoolId));
	strncpy((char *)gCfgDataDL.PoolConfigColName.byPoolUrl, argv[index++], sizeof(gCfgDataDL.PoolConfigColName.byPoolUrl));
	strncpy((char *)gCfgDataDL.PoolConfigColName.byPoolUser, argv[index++], sizeof(gCfgDataDL.PoolConfigColName.byPoolUser));
	strncpy((char *)gCfgDataDL.PoolConfigColName.byPoolpwd, argv[index++], sizeof(gCfgDataDL.PoolConfigColName.byPoolpwd));
	strncpy((char *)gCfgDataDL.PoolConfigColName.byAlgo, argv[index++], sizeof(gCfgDataDL.PoolConfigColName.byAlgo));

	return rtn;
}

int ParseAlarmLogColumnName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;
	
	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.AlarmLogColName.byId, argv[index++], sizeof(gCfgDataDL.AlarmLogColName.byId));
	strncpy((char *)gCfgDataDL.AlarmLogColName.byModule, argv[index++], sizeof(gCfgDataDL.AlarmLogColName.byModule));
	strncpy((char *)gCfgDataDL.AlarmLogColName.byAlarmType, argv[index++], sizeof(gCfgDataDL.AlarmLogColName.byAlarmType));
	strncpy((char *)gCfgDataDL.AlarmLogColName.byLevel, argv[index++], sizeof(gCfgDataDL.AlarmLogColName.byLevel));
	strncpy((char *)gCfgDataDL.AlarmLogColName.byContent, argv[index++], sizeof(gCfgDataDL.AlarmLogColName.byContent));
	strncpy((char *)gCfgDataDL.AlarmLogColName.byUpdateTime, argv[index++], sizeof(gCfgDataDL.AlarmLogColName.byUpdateTime));

	DEBUGMSG(1,("2 byId@%p, byId:%s\r\n", (char *)gCfgDataDL.AlarmLogColName.byId, 
		(char *)gCfgDataDL.AlarmLogColName.byId));

	DEBUGMSG(1,("\r\n\r\n\r\n*****byId:%s, byModule:%s, byAlarmType:%s, byUpdateTime:%s\r\n", 
			(char *)gCfgDataDL.AlarmLogColName.byId,
			(char *)gCfgDataDL.AlarmLogColName.byModule,
			(char *)gCfgDataDL.AlarmLogColName.byAlarmType,
			(char *)gCfgDataDL.AlarmLogColName.byUpdateTime));

	return rtn;
}

int ParseDelCellIdBaseColumnName(int argc, char**argv, int iStartPos)
{
	int rtn = 0;
	int index = 0;
	
	if (argc < iStartPos || (argc - 1) < atoi(argv[index++]))
		return (rtn = 1);
	
	index = iStartPos;

	strncpy((char *)gCfgDataDL.DelCellIdBaColName.byCellId, argv[index++], sizeof(gCfgDataDL.DelCellIdBaColName.byCellId));

	//printf("--------------------------byCellId:%s\r\n", (char *)gCfgDataDL.DelCellIdBaColName.byCellId);

	return rtn;
}

/*int InitSwitchToDb()
{
	BYTE byIpBuf[BUFFER_LEN_16] = {0};

	gpSwitchDbInfo = malloc(sizeof(stSwitchDbInfo));
	gpSwitchDbInfo->DbSwi.iSwiId = gCfgDataDL.SwitchInfo.iSwiId;
	//gpSwitchDbInfo->DbSwi.iNum= gCfgDataDL.SwitchInfo.iNum;	
	//gpSwitchDbInfo->DbSwi.iCellNum = gCfgDataDL.SwitchInfo.iCellNum;
	gpSwitchDbInfo->DbSwi.iStatsInter = gCfgDataDL.SwitchInfo.iStatsInter;
	//snprintf((char *)byIpBuf, sizeof(byIpBuf), "%s0.0", (char *)gCfgDataDL.DLWb.byWbIp);
	//strncpy((char *)gpSwitchDbInfo->DbWbIp.byWbIp, (char *)byIpBuf, sizeof(gpSwitchDbInfo->DbWbIp.byWbIp));
	//strncpy((char *)gpSwitchDbInfo->DbWbIp.byWbPort,(char *) gCfgDataDL.DLWb.byWbPort, sizeof(gpSwitchDbInfo->DbWbIp.byWbPort));

	gpSwitchDbInfo->iUpdateTime = GetTime();
	
	return 0;
}*/


/****************************************************************
** Function name:      	StrIniRead
** Descriptions:   	

** input parameters:   	нч
** output parameters:   	нч
** Returned value:     	1:open error	
						2:fgets error
						3.parse error

****************************************************************/
int StrIniRead(void)
{
	FILE *ConfFile;
	BYTE byReadBuff[BUFFER_LEN_256] = {0};
	char delim[] = ";";
	int argc = 0;
	char *argv[BUFFER_LEN_256] = {0};
	int i = 0, rtn = 0, iIndex = 0;
	char Data = '#';
	char *pData = 0;

	DEBUGMSG(0, ("%s...\r\n", __FUNCTION__));

	if((ConfFile = fopen("./download/CfgData.ini" ,"r")) == NULL)
	{
		GetLocalTimeLog();
		ap_log_debug_log("\t\t%s, %s, %d, open ./CfgData.ini error.\r\n", __FILE__, __FUNCTION__, __LINE__);		
		return (rtn = 1);
	}
	DEBUGMSG(0,("2 ConfFile:%p----\r\n", ConfFile));
	
	while(feof(ConfFile) == 0)
	{
		memset(byReadBuff,0, sizeof(byReadBuff));

		if(fgets((char *)byReadBuff, sizeof(byReadBuff), ConfFile) == NULL)
		{
			fclose(ConfFile);
			return (rtn = 2);
		}		
		
		if ((pData = strchr((char *)byReadBuff, Data)) != NULL)
		{
			DEBUGMSG(0, ("aaa %s, pData:%s, %s\r\n", (char *)byReadBuff, pData+1, (char *)(cFindHeadInfo+iIndex)->bydata));
			if (strcmp(pData+1, (char *)((cFindHeadInfo+iIndex)->bydata)) == 0)
			{
				iIndex++;
			}
			continue;
		}
		DEBUGMSG(0,("@@@@%s, %d, iIndex:%d\r\n", (char *)byReadBuff, strlen((char *)((cFindHeadInfo+iIndex)->bydata)), iIndex));
		//if(strncmp((char *)((cFindHeadInfo+iIndex)->bydata), (char *)byReadBuff, strlen((char *)((cFindHeadInfo+iIndex)->bydata+10)) == 0))
		if(strncmp((char *)byReadBuff, (char *)(cFindHeadInfo+iIndex)->bydata, strlen((char *)((cFindHeadInfo+iIndex)->bydata))) == 0)
		{	DEBUGMSG(0,("^^^^^%s\r\n", (char *)((cFindHeadInfo+iIndex)->bydata)));
			memset(byReadBuff,0, sizeof(byReadBuff));
			if(fgets((char *)byReadBuff, sizeof(byReadBuff), ConfFile) == NULL)
			{
				fclose(ConfFile);
				return (rtn = 2);
			}
			
			DEBUGMSG(0, ("b %s\r\n", (char *)byReadBuff));

			if (Parse((char *)byReadBuff, &argc, argv, delim) != 0)
			{
				GetLocalTimeLog();
				ap_log_debug_log("\t\t%s, %s, %d, parse error!\r\n",  __FILE__, __FUNCTION__, __LINE__);
				rtn = 3;
				continue;
			}

			cFindHeadInfo[iIndex].DownloadFunc(argc, argv, 1);
			
			/*switch(iIndex)
			{
			case E_HEAD_INFO_SwitchInfo:
				ParseSwitchInfo(argc, argv, 1);
				break;
				
			case E_HEAD_INFO_CELLINFO:
				ParseCellInfo(argc, argv, 1);
				break;

			case E_HEAD_INFO_DIVIDINFO:
				ParseDividInfo(argc, argv, 1);
				break;
				
			case E_HEAD_INFO_MUTICAST:
				ParseMuticast(argc, argv, 1);
				break;

			default:
				ap_log_debug_log("Device type does not exist.\r\n");
				break;
			}*/
				
			for (i = 0; i < argc; i++)
			{
				DEBUGMSG(0, ("***%p, %s\r\n", *(argv+i), *(argv+i)));
				memset(argv+i, 0, sizeof(*argv));
			}
			iIndex++;
		}
	}

	fclose(ConfFile);

	//InitSwitchToDb();
	
	dbgdumpDownLoad();

	return rtn;
}

void dbgdumpDownLoad(void)
{
	int i = 0;//, j = 0;
	char buf[][BUFFER_LEN_128] = 
	{
		//"	||DividDisable--Start--End--Total--Share||",
		//"	||WbIp-------------WbPort||",
		"	||ServerIp---------UserName---------UserPw---------DbName---------TableName||",
		"	||ServerIp---------UserName---------UserPw---------DbName---------TableName||",
		"	||ServerIp---------UserName---------UserPw---------DbName---------TableName||",
		"	||ServerIp---------UserName---------UserPw---------DbName---------TableName||",
		"	||ServerIp---------UserName---------UserPw---------DbName---------TableName||",
		"	||ServerIp---------UserName---------UserPw---------DbName---------TableName||",
		//"	||ServerIp---------UserName---------UserPw---------DbName---------TableName||",
		//"	||ServerIp---------UserName---------UserPw---------DbName---------TableName||"
	};

	//printf("%s...\r\n", __FUNCTION__);
	//printf("%d\r\n", sizeof(buf)/sizeof(*buf));
	//printf(" 	||MinrNum:%d	CellNum:%d||\r\n", gCfgDataDL.SwitchInfo.iNum, gCfgDataDL.SwitchInfo.iCellNum);
	for (i = 0; i < sizeof(buf)/sizeof(*buf); i++)
	{
		printf("%s\r\n", buf[i]);
		/*if (i == 0)
		{
			printf(" 	  %d             %d      %d    %d      %d\r\n", 
				gCfgDataDL.DLDivid.bDividDisable, gCfgDataDL.DLDivid.iStart, 
				gCfgDataDL.DLDivid.iEnd, gCfgDataDL.DLDivid.iTotal, gCfgDataDL.DLDivid.iShare);
		}*/
		switch(i)
		{
		/*case 0:
			printf("  	  %s          %s\r\n", (char *)gCfgDataDL.DLWb.byWbIp, (char *)gCfgDataDL.DLWb.byWbPort);
			break;*/

		/*case 0:
			printf("  	  %s        %s           %s          %s          %s\r\n", 
			(char *)gCfgDataDL.MinerBaseInfo.myServerIp, (char *)gCfgDataDL.MinerBaseInfo.myUserName, (char *)gCfgDataDL.MinerBaseInfo.myUserPw, 
			(char *)gCfgDataDL.MinerBaseInfo.myDbName, (char *)gCfgDataDL.MinerBaseInfo.myTableName);	
			break;*/
			
		case 0:
			printf("  	  %s        %s           %s          %s          %s\r\n", 
			(char *)gCfgDataDL.CellBaseInfo.myServerIp, (char *)gCfgDataDL.CellBaseInfo.myUserName, (char *)gCfgDataDL.CellBaseInfo.myUserPw, 
			(char *)gCfgDataDL.CellBaseInfo.myDbName, (char *)gCfgDataDL.CellBaseInfo.myTableName);
			break;

		/*case 2:
			printf("  	  %s        %s           %s          %s          %s\r\n", 
			(char *)gCfgDataDL.MinerStatusInfo.myServerIp, (char *)gCfgDataDL.MinerStatusInfo.myUserName, (char *)gCfgDataDL.MinerStatusInfo.myUserPw, 
			(char *)gCfgDataDL.MinerStatusInfo.myDbName, (char *)gCfgDataDL.MinerStatusInfo.myTableName);
			break;*/
			
		case 1:
			printf("	  %s        %s           %s          %s          %s\r\n", 
			(char *)gCfgDataDL.CellStatusInfo.myServerIp, (char *)gCfgDataDL.CellStatusInfo.myUserName, (char *)gCfgDataDL.CellStatusInfo.myUserPw, 
			(char *)gCfgDataDL.CellStatusInfo.myDbName, (char *)gCfgDataDL.CellStatusInfo.myTableName);
			break;

		case 2:
			printf("  	  %s        %s           %s          %s     %s\r\n", 
			(char *)gCfgDataDL.SwitchBaseInfo.myServerIp, (char *)gCfgDataDL.SwitchBaseInfo.myUserName, (char *)gCfgDataDL.SwitchBaseInfo.myUserPw, 
			(char *)gCfgDataDL.SwitchBaseInfo.myDbName, (char *)gCfgDataDL.SwitchBaseInfo.myTableName);
			break;

		case 3:
			printf("  	  %s        %s           %s          %s       %s\r\n", 
			(char *)gCfgDataDL.PoolInfo.myServerIp, (char *)gCfgDataDL.PoolInfo.myUserName, (char *)gCfgDataDL.PoolInfo.myUserPw, 
			(char *)gCfgDataDL.PoolInfo.myDbName, (char *)gCfgDataDL.PoolInfo.myTableName);
			break;
			
		case 4:
			printf("	  %s        %s           %s          %s       %s\r\n", 
			(char *)gCfgDataDL.AlarmInfo.myServerIp, (char *)gCfgDataDL.AlarmInfo.myUserName, (char *)gCfgDataDL.AlarmInfo.myUserPw, 
			(char *)gCfgDataDL.AlarmInfo.myDbName, (char *)gCfgDataDL.AlarmInfo.myTableName);
			break;

		case 5:
			printf("  	  %s        %s           %s          %s          %s\r\n", 
			(char *)gCfgDataDL.DelCellIdBaseInfo.myServerIp, (char *)gCfgDataDL.DelCellIdBaseInfo.myUserName, (char *)gCfgDataDL.DelCellIdBaseInfo.myUserPw, 
			(char *)gCfgDataDL.DelCellIdBaseInfo.myDbName, (char *)gCfgDataDL.DelCellIdBaseInfo.myTableName);
			break;

		default:
			break;
		}
	}

	printf("  	||szSwiId:%s, iStatsInter:%d, szServerIp:%s, iServerPort:%d||\r\n", 
		gCfgDataDL.SwitchInfo.szSwiId, 
		//gpSwitchDbInfo->DbSwi.iNum, 
		//gpSwitchDbInfo->DbSwi.iCellNum, 
		gCfgDataDL.SwitchInfo.iStatsInter,
		gCfgDataDL.SwitchInfo.szServerIp,
		gCfgDataDL.SwitchInfo.iServerPort
		//(char *)gpSwitchDbInfo->DbWbIp.byWbIp,
		//(char *)gpSwitchDbInfo->DbWbIp.byWbPort,
		//gpSwitchDbInfo->iUpdateTime
		);
	
#if 0
	printf("  	||%s, %s, %s, %s, %s, %s, %s, %s\r\n  	  %s, %s, %s, %s, %s, %s, %s||\r\n", 
	(char *)gCfgDataDL.MinerColName.byKeyName,
	(char *)gCfgDataDL.MinerColName.byMinerIp,
	(char *)gCfgDataDL.MinerColName.byMinerPort,
	(char *)gCfgDataDL.MinerColName.byAlgo,
	(char *)gCfgDataDL.MinerColName.byOnline,
	(char *)gCfgDataDL.MinerColName.byBTCWbsId,
	(char *)gCfgDataDL.MinerColName.byBTCCellNum,
	(char *)gCfgDataDL.MinerColName.byBTCWbIp,
	(char *)gCfgDataDL.MinerColName.byBTCWbPort,
	/*(char *)gCfgDataDL.MinerColName.byLTCWbsId,
	(char *)gCfgDataDL.MinerColName.byLTCCellNum,
	(char *)gCfgDataDL.MinerColName.byLTCWbIp,
	(char *)gCfgDataDL.MinerColName.byLTCWbPort*/
	(char *)gCfgDataDL.MinerColName.byRegTime,
	(char *)gCfgDataDL.MinerColName.byLoginTime,
	(char *)gCfgDataDL.MinerColName.byLogoutTime,
	(char *)gCfgDataDL.MinerColName.byPool1Id,
	(char *)gCfgDataDL.MinerColName.byPool2Id,
	(char *)gCfgDataDL.MinerColName.byPool3Id
	//(char *)gCfgDataDL.MinerColName.byPoolName,
	//(char *)gCfgDataDL.MinerColName.byPoolStat
	);

	printf("  	||%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s\r\n  	  %s, %s, %s, %s, %s, %s, %s, %s, %s, %s||\r\n", 
	(char *)gCfgDataDL.CellColName.byKeyName,
	(char *)gCfgDataDL.CellColName.byCellIp,
	(char *)gCfgDataDL.CellColName.byCellPort,
	(char *)gCfgDataDL.CellColName.bySwVer,
	(char *)gCfgDataDL.CellColName.byHwVer,
	(char *)gCfgDataDL.CellColName.byChipBTCHash,
	(char *)gCfgDataDL.CellColName.byChipCnt,
	(char *)gCfgDataDL.CellColName.byAlgo,
	(char *)gCfgDataDL.CellColName.byOnline,
	(char *)gCfgDataDL.CellColName.byBTCWbsId,
	(char *)gCfgDataDL.CellColName.byBTCWbcId,
	(char *)gCfgDataDL.CellColName.byBTCCpm,
	(char *)gCfgDataDL.CellColName.byBTCWbIp,
	(char *)gCfgDataDL.CellColName.byBTCWbPort,
	(char *)gCfgDataDL.CellColName.byLTCWbsId,
	(char *)gCfgDataDL.CellColName.byLTCWbcId,
	(char *)gCfgDataDL.CellColName.byLTCCpm,
	(char *)gCfgDataDL.CellColName.byLTCWbIp,
	(char *)gCfgDataDL.CellColName.byLTCWbPort,
	(char *)gCfgDataDL.CellColName.byRegTime,
	(char *)gCfgDataDL.CellColName.byLoginTime,
	(char *)gCfgDataDL.CellColName.byLogoutTime);
#endif
}

