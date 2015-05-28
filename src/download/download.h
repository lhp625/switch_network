#ifndef DOWNLOAD_H
#define DOWNLOAD_H

#include "../app/typedefs.h"
#include "../ap_log.h"

typedef enum 
{
	E_HEAD_INFO_SwitchInfo,
	E_HEAD_INFO_CELLINFO,
	E_HEAD_INFO_DIVIDINFO,
	E_HEAD_INFO_MUTICAST
}E_HEAD_INFO;

typedef struct
{
	BYTE bydata[BUFFER_LEN_32];
	IF_PROC_INTLPPSTRINT_INT DownloadFunc;
}stFindHeadInfo;

typedef struct
{
	//int iSwiId;
	BYTE szSwiId[BUFFER_LEN_64];
	//BYTE szSwiId;                                              //mac and short name
	//int iNum;
	//int iCellNum;			                                   //每个miner挂载几个cell
	int iStatsInter;                                           //状态信息间隔
	BYTE szPoolIdPri[BUFFER_LEN_64];                           //按顺序保存池的id
	BYTE szServerIp[BUFFER_LEN_16];			                   //server
	int iServerPort;		                                   //port	
}stSwitchInfo;

typedef struct
{
	BYTE szMulticastIp[BUFFER_LEN_16];                         //组播的ip	
	int iMulticastPort;                                        //组播port	
}stMulticast;

typedef struct
{
	BYTE szMoniIp[BUFFER_LEN_16];			                   //remotehost_str
	int iMoniPort;		                                       //client_port
	BYTE szToken[BUFFER_LEN_64];                               //token
}stMonitorInfo;

typedef struct
{
	int iNum;
}stCellInfo;

typedef struct
{
	BYTE byWbIp[BUFFER_LEN_16];			                       //发送workbase的组播ip	
	BYTE byWbPort[BUFFER_LEN_8];		                       //发送workbase的组播port	
}stWbIpPort;

typedef struct
{
	BYTE byKeyName[BUFFER_LEN_16];                             //主键名称
	BYTE byMinerIp[BUFFER_LEN_16];
	BYTE byMinerPort[BUFFER_LEN_16];
 	BYTE byAlgo[BUFFER_LEN_16];                                                
	BYTE byOnline[BUFFER_LEN_16];
	BYTE byBTCWbsId[BUFFER_LEN_16];
	//BYTE byBTCCellNum[BUFFER_LEN_16];
	//BYTE byBTCWbIp[BUFFER_LEN_16];
	//BYTE byBTCWbPort[BUFFER_LEN_16];
	
	BYTE byLTCWbsId[BUFFER_LEN_16];
	//BYTE byLTCCellNum[BUFFER_LEN_16];
	/*BYTE byLTCWbIp[BUFFER_LEN_16];
	BYTE byLTCWbPort[BUFFER_LEN_16];*/

	BYTE byRegTime[BUFFER_LEN_16];
	BYTE byLoginTime[BUFFER_LEN_16];
	BYTE byLogoutTime[BUFFER_LEN_16];
	BYTE byPool1Id[BUFFER_LEN_16];
	BYTE byPool2Id[BUFFER_LEN_16];
	BYTE byPool3Id[BUFFER_LEN_16];
	BYTE byPath[BUFFER_LEN_16];
}stMinerDbColumnName;

typedef struct
{
	BYTE byKeyName[BUFFER_LEN_16];                              //主键名称
	BYTE byDevId[BUFFER_LEN_16];                                //设备id
	BYTE byCellIp[BUFFER_LEN_16];
	BYTE byCellPort[BUFFER_LEN_16];
	BYTE bySwVer[BUFFER_LEN_16];
	BYTE byHwVer[BUFFER_LEN_16];
	BYTE byAlgo[BUFFER_LEN_16];
	BYTE byChipCnt[BUFFER_LEN_16];
	BYTE byChipBTCHash[BUFFER_LEN_16];
	BYTE byChipLTCHash[BUFFER_LEN_16];
	BYTE byBTCFreq[BUFFER_LEN_16];
 	BYTE byLTCFreq[BUFFER_LEN_16];                                                
	BYTE byOnline[BUFFER_LEN_16];
	BYTE szPoolList[BUFFER_LEN_16];
	BYTE byTestResult[BUFFER_LEN_16];
	//BYTE byBTCWbsId[BUFFER_LEN_16];
	//BYTE byBTCWbcId[BUFFER_LEN_16];
	/*BYTE byBTCCpm[BUFFER_LEN_16];
	BYTE byBTCWbIp[BUFFER_LEN_16];
	BYTE byBTCWbPort[BUFFER_LEN_16];*/	
	//BYTE byLTCWbsId[BUFFER_LEN_16];
	//BYTE byLTCWbcId[BUFFER_LEN_16];
	/*BYTE byLTCCpm[BUFFER_LEN_16];
	BYTE byLTCWbIp[BUFFER_LEN_16];
	BYTE byLTCWbPort[BUFFER_LEN_16];*/
	BYTE byRegTime[BUFFER_LEN_16];
	BYTE byLoginTime[BUFFER_LEN_16];
	BYTE byLogoutTime[BUFFER_LEN_16];
	
	BYTE szBTC30MNcsend[BUFFER_LEN_16];
	BYTE szLTC30MNcsend[BUFFER_LEN_16];
}stCellDbColumnName;

typedef struct
{
	BYTE byMinerId[BUFFER_LEN_16];
	BYTE byElapse[BUFFER_LEN_16];
	BYTE byCurPoolId[BUFFER_LEN_16];
	BYTE byCurPoolStat[BUFFER_LEN_16];
	BYTE byUpdateTime[BUFFER_LEN_16];
}stMinerStatusDbColumnName;

typedef struct
{
	BYTE byCellId[BUFFER_LEN_16];
	//BYTE byTestResult[BUFFER_LEN_16];
	BYTE byEntryTemp[BUFFER_LEN_16];
	BYTE byExitTemp[BUFFER_LEN_16];
	BYTE byLedStatus[BUFFER_LEN_16];
	BYTE byElapse[BUFFER_LEN_16];
	//BYTE byBtcWblost[BUFFER_LEN_16];
	BYTE byBtcNcsend[BUFFER_LEN_16];
	//BYTE byBtcHwerr[BUFFER_LEN_16];
	BYTE byBtcAccept[BUFFER_LEN_16];
	BYTE byBtcReject[BUFFER_LEN_16];
	//BYTE byBtcDiff[BUFFER_LEN_16];
	//BYTE byLtcWblost[BUFFER_LEN_16];
	BYTE byLtcNcsend[BUFFER_LEN_16];
	//BYTE byLtcHwerr[BUFFER_LEN_16];
	BYTE byLtcAccept[BUFFER_LEN_16];
	BYTE byLtcReject[BUFFER_LEN_16];
	//BYTE byLtcDiff[BUFFER_LEN_16];
	BYTE byChipsTemp[BUFFER_LEN_16];
	BYTE byUpdateTime[BUFFER_LEN_16];	
}stCellStatusDbColumnName;

typedef struct
{
	BYTE bySwitchId[BUFFER_LEN_16];
	//BYTE byAllocPolicy[BUFFER_LEN_16];
	//BYTE byMaxAllocsize[BUFFER_LEN_16];
	//BYTE byCpm[BUFFER_LEN_16];
	BYTE byStatusInterval[BUFFER_LEN_16];
	//BYTE byWbipBase[BUFFER_LEN_16];
	//BYTE byWbPort[BUFFER_LEN_16];
	BYTE byPoolidPri[BUFFER_LEN_16];
	BYTE byUpdateTime[BUFFER_LEN_16];
	//BYTE byNextCellid[BUFFER_LEN_16];
}stSwitchBaseDbColumnName;

typedef struct 
{
	BYTE byPoolId[BUFFER_LEN_16];
	BYTE byPoolUrl[BUFFER_LEN_16];
	BYTE byPoolUser[BUFFER_LEN_16];
	BYTE byPoolpwd[BUFFER_LEN_16];
	BYTE byAlgo[BUFFER_LEN_16];
}stPoolConfigDbColumnName;

typedef struct
{
	BYTE byId[BUFFER_LEN_16];
	BYTE byModule[BUFFER_LEN_16];
	BYTE byAlarmType[BUFFER_LEN_16];
	BYTE byLevel[BUFFER_LEN_16];
	BYTE byContent[BUFFER_LEN_16];
	BYTE byUpdateTime[BUFFER_LEN_16];
}stAlarmLogDbColumnName;

typedef struct
{
	BYTE myServerIp[BUFFER_LEN_16];                             //mysql server ip
	BYTE myUserName[BUFFER_LEN_32];                             //mysql user name
	BYTE myUserPw[BUFFER_LEN_32];                               //mysql user password
	BYTE myDbName[BUFFER_LEN_32];                               //mysql databases name
	BYTE myTableName[BUFFER_LEN_32];                            //mysql table name
}stMyConnInfo;

typedef struct
{
	long iUpdateTime;                                           //字段更新时间
	stSwitchInfo DbSwi;
	//stWbIpPort DbWbIp;
}stSwitchDbInfo;

typedef struct
{
	BYTE byCellId[BUFFER_LEN_16];
}stDelCellIdBaseColumnName;

typedef struct 
{
	stSwitchInfo SwitchInfo;
	stMulticast CellMulticast;
	stMonitorInfo MonitorInfo;
	stCellInfo CellInfo;
	//stDividXnonce DLDivid;
	stWbIpPort DLWb;
	
	stMyConnInfo MinerBaseInfo;
	stMyConnInfo CellBaseInfo;
	stMyConnInfo MinerStatusInfo;
	stMyConnInfo CellStatusInfo;	
	stMyConnInfo SwitchBaseInfo;
	stMyConnInfo DelCellIdBaseInfo;
	stMyConnInfo PoolInfo;
	stMyConnInfo AlarmInfo;
	stMinerDbColumnName MinerColName;
	stCellDbColumnName CellColName;
	stMinerStatusDbColumnName MinerStaColName;
	stCellStatusDbColumnName CellStaColName;
	stSwitchBaseDbColumnName SwitchBaseColName;
	stPoolConfigDbColumnName PoolConfigColName;
	stAlarmLogDbColumnName AlarmLogColName;
	stDelCellIdBaseColumnName DelCellIdBaColName;
}stCfgDataDL, *pstCfgDataDL;

stCfgDataDL gCfgDataDL;
stSwitchDbInfo *gpSwitchDbInfo;

int ParseSwitchInfo(int argc, char**argv, int iStartPos);
int ParseMulticastInfo(int argc, char**argv, int iStartPos);
int ParseMonitorInfo(int argc, char**argv, int iStartPos);
int ParseCellInfo(int argc, char**argv, int iStartPos);
int ParseDividInfo(int argc, char**argv, int iStartPos);
int ParseMuticast(int argc, char**argv, int iStartPos);
int ParseMinerBaseDbName(int argc, char**argv, int iStartPos);
int ParseCellBaseDbName(int argc, char**argv, int iStartPos);
int ParseMinerStatusDbName(int argc, char**argv, int iStartPos);
int ParseCellStatusDbName(int argc, char**argv, int iStartPos);
int ParseSwitchBaseDbName(int argc, char**argv, int iStartPos);
int ParsePoolConfigDbName(int argc, char**argv, int iStartPos);
int ParseAlarmLogDbName(int argc, char**argv, int iStartPos);
int ParseCellIdBaseDbName(int argc, char**argv, int iStartPos);
int ParseMinerBaseColumName(int argc, char**argv, int iStartPos);
int ParseCellBaseColumName(int argc, char**argv, int iStartPos);
int ParseMinerStatusColumName(int argc, char**argv, int iStartPos);
int ParseCellStatusColumName(int argc, char**argv, int iStartPos);
int ParseSwitchBaseColumnName(int argc, char**argv, int iStartPos);
int ParsePoolConfigColumnName(int argc, char**argv, int iStartPos);
int ParseAlarmLogColumnName(int argc, char**argv, int iStartPos);
int ParseDelCellIdBaseColumnName(int argc, char**argv, int iStartPos);

int StrIniRead(void);

void dbgdumpDownLoad(void);

#endif

