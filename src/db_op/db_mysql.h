#ifndef DB_MYSQL__H
#define DB_MYSQL__H

#include "/usr/include/mysql/mysql.h"                           //上面必须是mysql.h的绝对地址，一般在mysql下的include目录下
#include "../app/typedefs.h"
#include "../api_comm/def_devstr.h"

#define MINER_DB_COMMANDLINE               13//14
#define CELL_DB_COMMANDLINE                19//20
#define SWITCH_DB_COMMANDLINE              4
//#define SWITCH_DB_POOLID_COMMANDLINE       6
#define DELCELLID_DB_COMMANDLINE           1
#define POOL_DB_COMMANDLINE                5

#define QUERY_LEN                          6

/*typedef enum
{
	E_MYSQLERROR_TYPE_NODB = 1049,                                             //数据库不存在
	E_MYSQLERROR_TYPE_NOTABLE = 1051,                                        //数据表不存在
	E_MYSQLERROR_TYPE_NOFIELD = 1054                                         //字段不存在
}E_MYSQLERROR_TYPE;*/

typedef struct
{
	BYTE bySqlType1[BUFFER_LEN_16];
	IF_PROC_PTRPTRPTRPTRINT_INT QueryPackFunc1;
	IF_PROC_PTRPTRPTRPTRINT_INT QueryPackFunc2;
	IF_PROC_PTRPTRPTRPTRINT_INT QueryPackFunc3;
	IF_PROC_PTRPTRPTRPTRINT_INT QueryPackFunc4;	
	//BYTE bySqlType2[BUFFER_LEN_16];
}stMysqlQuery;

typedef struct
{
	int (*dbConnect)(MYSQL *, stMyConnInfo *);
	int (*dbDisconnect)(MYSQL *);
	int (*dbDoQuery)(MYSQL *, const char *, E_QUERY_TYPE, IF_PROC_INTLPPSTRINT_INT);
	int (*dbReconnect)(MYSQL *, stMyConnInfo *);
}stMysql;

/*typedef struct
{
	BOOL bIniMinerBase;                                         //miner_base初始化OK
	BOOL bIniCellBase;                                          //cell_base初始化OK
}stInitDB;*/

typedef struct
{
	MYSQL *pMySql;
	BOOL *pbInitOK;
	stMyConnInfo *pConnInfo;
	void *pColName;
	stMysqlQuery *pQuery;
	IF_PROC_INTLPPSTRINT_INT InitDbToDevFunc1;
	IF_PROC_INTLPPSTRINT_INT InitDbToDevFunc2;
	pthread_mutex_t *pMySqlLock;
}stConnectDataBase;

/*MYSQL myCell;
MYSQL mySwitch;
BOOL bIniCell;
BOOL bIniSwitch;*/

//MYSQL myMinerBase;
MYSQL myCellBase;
//MYSQL myMinerStatus;
MYSQL myCellStatus;
MYSQL mySwitchBase;
MYSQL myPoolConfig;
MYSQL myAlarmLog;
MYSQL myDelCellIdBase;
//BOOL bIniMinerBase; 										   //miner_base初始化OK
BOOL bIniCellBase;											   //cell_base初始化OK
//BOOL bIniMinerStatus; 
BOOL bIniCellStatus; 
BOOL bIniSwitchBase;
BOOL bIniPoolConfig;
BOOL bIniAlarmLog;
BOOL bIniDelCellIdBase;

pthread_mutex_t gCellBaseLock;
pthread_mutex_t gCellStatusLock;
pthread_mutex_t gSwitchBaseLock;
pthread_mutex_t gPoolConfigLock;
pthread_mutex_t gAlarmLogLock;

stMysql MysqlFunc;

//stInitDB bInitDBOK;
stMysqlQuery MinerSqlQuery[QUERY_LEN];
stMysqlQuery CellSqlQuery[QUERY_LEN];
stMysqlQuery MinerStatusQuery[QUERY_LEN];
stMysqlQuery CellStatusQuery[QUERY_LEN];
stMysqlQuery DelCellIdBaseQuery[QUERY_LEN];
stMysqlQuery AlarmLogQuery[QUERY_LEN];
stMysqlQuery PoolConfigQuery[QUERY_LEN];
stMysqlQuery SwitchBaseQuery[QUERY_LEN];

int gCrrMaxCellId;                                             //从database获取最后关闭switch后，最大的CellId，0是无效的
int gCrrMaxPoolId;                                             //从database获得最大的pool id

int IncrCrrentAlgoLen(E_ALGO eAlgo, stAssiAlgoRes *pDevAssi);
int InitDbAndDev();
int InitDbToSwitch(int argc, char**argv, int iStartPos);
int InitDbToMaxCellId(int argc, char**argv, int iStartPos);
int InitDbToMiner(int argc, char**argv, int iStartPos);
int InitDbToDelCellIdBase(int argc, char**argv, int iStartPos);
int InitDbToCell(int argc, char**argv, int iStartPos);
int InitDbToPool(int argc, char**argv, int iStartPos);
int InitDbToMaxPoolId(int argc, char**argv, int iStartPos);
int InitDbToDefaultPool(int argc, char**argv, int iStartPos);

int NotInitDb(int argc, char**argv, int iStartPos);

int MysqlConnect(MYSQL *db, stMyConnInfo *pInfo);
int MysqlDisconnect(MYSQL *db);
int MysqlDoQuery(MYSQL *db, const char *pQueInfo, E_QUERY_TYPE eMysqlQuery, IF_PROC_INTLPPSTRINT_INT InitFunc);
/*int PackMinerBaseInsert(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackMinerBaseSelectSingle(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackMinerBaseUpdate(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackMinerBaseAlter(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackMinerDeleteOneRow(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);*/
//int PackMinerBaseCreate(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackCellBaseInsert(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackCellBaseSelectSingle(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackCellBaseUpdate(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackCellBaseUpdatePool(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackCellBaseUpdateRealKHash(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackCellBaseUpdateFreq(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackCellBaseAlter(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackCellDeleteOneRow(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackCellBaseSelectCellMax(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackCellBaseCreate(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
/*int PackMinerStatusInsert(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackMinerStatusSelectSingle(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackMinerStatusUpdate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackMinerStatusAlter(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);*/
//int PackMinerStatusCreate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackCellStatusInsert(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackCellStatusSelectSingle(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackCellStatusUpdate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackCellStatusAlter(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackCellStatusCreate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackSwitchBaseInsert(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackSwitchBaseSelectSingle(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackSwitchBaseUpdate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackSwitchBaseAlter(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackSwitchBaseCreate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackPoolConfigInsert(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackPoolConfigSelectSingle(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackPoolConfigSelectPoolMax(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackPoolConfigUpdate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackPoolConfigAlter(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackPoolDeleteOneRow(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackPoolConfigCreate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackAlarmLogInsert(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackAlarmLogSelectSingle(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackAlarmLogUpdate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackAlarmLogAlter(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackAlarmLogCreate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackDelCellIdBaseInsert(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackDelCellIdBaseSelectSingle(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackDelCellIdBaseUpdate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackDelCellIdBaseAlter(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);
int PackDelCellIdDeleteOneRow(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackDelCellIdBaseCreate(void *pComm, void *pInfo, void *pColName, char *dest, int destLen);

int PackSelectAll(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int PackDeleteAll(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
//int PackCreateDb(void *pCoon, void *pInfo, void *pColName, char *dest, int destLen);
int CalculateRatedKHash(stCellConn *pCellConn);

void *MysqlProcess(void *arg);

#endif

