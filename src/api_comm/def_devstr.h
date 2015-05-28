#ifndef DEF_DEVSTR_H
#define DEF_DEVSTR_H

#include "../download/download.h"
#include "../fifo_queue/circular_queue.h"
#include "../api_net/api_net.h"

#define ALGO_LEN                           3
#define MAX_CLIENTS                        20 
#define MAX_MINER_COMMAND_NUM              5
#define MAX_CELL_COMMAND_NUM               7//12//11
#define ERROR_NUM                          13
#define MAX_POOL_COUNT                     3
#define QUERY_COUNT                        15
#define COMM_ERR_NUM                       13
#define POOLCONF_ERR_NUM                   8
/************************************************
	unsigned   int   0～4294967295   
	int   2147483648～2147483647 
	unsigned long 0～4294967295
	long   2147483648～2147483647
	long long的最大值：9223372036854775807
	long long的最小值：-9223372036854775808
	unsigned long long的最大值：1844674407370955161
*************************************************/
#define BTC_SHARE                          0xFFFFFFFF             //4294967295
#define LTC_SHARE                          0xFFFF                 //65535
#define HASH_MAX_RANGE                     0xFFFFFFFFFFFFFFFF     //18446744073709551615
//#define HASH_MAX_RANGE                     0x7FFFFFFFFFFFFFFF//9223372036854775807

typedef enum 
{
	E_LOGIN_TYPE_MINER,
	E_LOGIN_TYPE_CELL
}E_LOGIN_TYPE;

typedef enum
{
	E_MINER_LOGIN_ERROR_OVERNUM = 1,
	E_CELL_LOGIN_ERROR_OVERNUM,
	E_MINER_LOGIN_ERROR_OVERALGO,
	E_CELL_LOGIN_ERROR_OVERALGO,
	E_LOGIN_ERROR_IPCONFLICT,
	E_LOGIN_ERROR_MACCONFLICT,
	E_LOGIN_ERROR_OVERBTC,
	E_LOGIN_ERROR_OVERLTC,
	E_LOGIN_ERROR_JSON
}E_LOGIN_ERROR;

typedef enum
{
	E_QUERY_TYPE_INSERT = 1,                                    //插入
	E_QUERY_TYPE_SELECT,                                        //查询
	E_QUERY_TYPE_UPDATE,                                        //更新
	E_QUERY_TYPE_ALTER,                                         //修改
	E_QUERY_TYPE_DELETE,                                        //删除
	E_QUERY_TYPE_CREATETABLE                                    //创建表
	//E_QUERY_TYPE_SELECT_MAX                                     //查询最大的
	//E_QUERY_TYPE_CREATEDB,                                      //创建数据库
	//E_QUERY_TYPE_CREATETABLE                                    //创建表
}E_QUERY_TYPE;

typedef enum 
{
	E_COMMAND_TYPE_STAT = 0,                                    //get_stat获取状态量
	E_COMMAND_TYPE_RELOGIN,                                     //relogin重新登陆
	E_COMMAND_TYPE_REBOOT,                                      //reboot重启	
	E_COMMAND_TYPE_RESETFLASH,                                  //resetflash清空flash
	E_COMMAND_TYPE_SETLED,                                      //set_led设置LED
	E_COMMAND_TYPE_SETPOOL                                      //setpool
}E_COMMAND_TYPE;

typedef enum 
{
	E_DEV_DATAUPDATE_NEWCELL = 1,                               //有新cell
	E_DEV_DATAUPDATE_CELLNEWDATA                                //cell有数据更新
}E_DEV_DATAUPDATE;

typedef struct
{
	BYTE algo[BUFFER_LEN_4];
}stAlgo;

typedef struct
{
	BYTE byLoginDevType[BUFFER_LEN_16];
	IF_PROC_INTLPPSTRINT_INT LoginFunc;
}stLoginType;

typedef struct
{
	BYTE byLoginData[BUFFER_LEN_32];
}stLoginOutputHead;

typedef struct
{
	BYTE byLoginError[BUFFER_LEN_64];
}stLoginOutputError;

typedef struct
{
	int iMinerId;                       //设备id
	//BYTE byMinerId[BUFFER_LEN_32];
	E_ALGO eMinerAlgo;                  //算法
	BOOL bMinerOnline;                  //Miner是否在线
	BYTE byPoolName[BUFFER_LEN_32];     //池名字
	BYTE byPoolStat[BUFFER_LEN_16];     //池状态
	long iMinerAlarmTimer;              //Miner通信中断定时器，如果超过3次发送次数，则把Online置为FALSE
	long iRegtime;                      //注册时间
	long iLoginTime;                    //登陆时间
	long iLogoutTime;                   //登出时间

	BYTE byAlarm[BUFFER_LEN_4];         //目前报警数量设为4
	//int iMinerTestNum;                             //测试用
}stMinerloginInput;

typedef struct
{
	//
	//long lDevId;                        //设备id
	BYTE szCellId[BUFFER_LEN_28];       //设备id
	int iCellId;                        //cell识别id
	BYTE szDevId[BUFFER_LEN_28];        //设备id
	BYTE szSwVer[BUFFER_LEN_16];        //软件版本
	BYTE szHwVer[BUFFER_LEN_16];        //硬件版本	
	
	UINT iSwVer;                        //软件版本
	UINT iHwVer;                        //硬件版本	
	E_ALGO eCellAlgo;                   //算法
	UINT iChipCnt;                      //芯片个数
	LLONG iChipBTCHash;                 //单个芯片额定算力
	LLONG iChipLTCHash;                 //单个芯片额定算力
	UINT iChipBTCFreq;                  //芯片主频
	UINT iChipLTCFreq;                  //芯片主频
	BOOL bCellOnline;                   //Cell是否在线
	BYTE szTestResult[BUFFER_LEN_28];	//芯片检测结果
	long iCellAlarmTimer;               //Cell通信中断定时器，如果超过3次发送次数，则把Online置为FALSE
	long iRegtime;                      //注册时间
	long iLoginTime;                    //登陆时间
	long iLogoutTime;                   //登出时间

	LLONG lBTCRatedKHash;               //额定算力计算方法，iChipBTCHash*iChipCnt/VALUES_1000
	LLONG lLTCRatedKHash;               //额定算力计算方法，iChipLTCHash*iChipCnt/VALUES_1000

	BYTE byAlarm[BUFFER_LEN_4];         //目前报警数量设为4
	//int iCellTestNum;                                //测试用
}stCellloginInput;

typedef struct
{
	BYTE byTestResult[BUFFER_LEN_28];   //芯片检测结果
	int iEntryTemp;                     //入口温度
	int iExitTemp;                      //出口温度
	int iElapse;                        //运行时间
	ULLONG ullBTCReject;                //BTC nonce拒绝数
	ULLONG ullBTCAccept;                //BTC nonce接受数
	ULLONG ullLTCReject;                //LTC nonce拒绝数
	ULLONG ullLTCAccept;                //LTC nonce接受数
	int iBTCNcIndex;                    //BTCNcsend下标
	int iLTCNcIndex;                    //LTCNcsend下标
	ULLONG llBTCNcsend[BUFFER_LEN_32];  //in 30Min BTC hashsum
	ULLONG llLTCNcsend[BUFFER_LEN_32];  //in 30Min BTC hashsum
               
	//实际算力计算方法，单位：khash/s，iBTCRealKHash=(|lBTCNcsend[Crr] - lBTCNcsend[30M before]| * BTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000)
	ULLONG llBTCRealKHash; 
	//实际算力计算方法，单位：khash/s，iLTCRealKHash=(|lLTCNcsend[Crr] - lLTCNcsend[30M before]| * LTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000)
	ULLONG llLTCRealKHash;
}stCellStatus;

typedef struct
{
	int sock; 	                        /**< less than 0 if no bind was made */
	union 		                        /**< local address. filled on connect() */
	{
		struct sockaddr_in	addr4;      /* IPv4 */
		struct sockaddr_in6 addr6;      /* IPv6 */
	};
}stParentListener;

typedef struct
{
	int iWbIndex;                       //workbase分配表中的下标，方便查找bWbNoFlag
	int iWbsId;                         //对应stResource中的bWbNo(workbase编号)
	int iConnCellNum;                   //同一个ip和port分给几个Cell	
	stWbIpPort WbIpPort;				//ip port
}stResMiner;

typedef struct
{
	int iWbIndex;                       //workbase分配表中的下标
	int iConnCellIndex;                 //Cell下标，方便查找pbWbCellNoFlag
	int iWbNo;                          //workbase编号
	int iConnCellNum;                   //同一个ip和port分给几个Cell	
	int iWbcId;                         //对应	stResource中的	pWbCellNo(每个workbase中的cell编号	)					
	stWbIpPort WbIpPort;                //ip port
}stResCell;

typedef struct
{
	pthread_mutex_t OnlineLock;         //是否在线锁
	pthread_mutex_t AlarmTimerLock;     //报警锁
	pthread_mutex_t IpPortLock;         //ip和port锁
	pthread_mutex_t PoolConfLock;       //pool设置锁
}stDevDataLock;

typedef struct 
{
	int iPoolId;
	BYTE szUrl[BUFFER_LEN_256];
	BYTE szPort[BUFFER_LEN_8];
	BYTE szUser[BUFFER_LEN_64];
	BYTE szPasswd[BUFFER_LEN_32];
	E_ALGO eAlgo;
}stPoolConfInfo;

/*typedef struct
{
	int iBTCPoolCount;
	int iLTCPoolCount;
	stPoolConfInfo stBTCPoolConf[MAX_POOL_COUNT];
	stPoolConfInfo stLTCPoolConf[MAX_POOL_COUNT];
}stNodePoolCrrConf;*/

typedef struct
{
	//int iBTCPoolIdPri[MAX_POOL_COUNT];
	//int iLTCPoolIdPri[MAX_POOL_COUNT];
	int iBTCPoolCount;
	int iLTCPoolCount;
	//BYTE szBTCPoolIdPri[BUFFER_LEN_32];
	//BYTE szLTCPoolIdPri[BUFFER_LEN_32];
	stPoolConfInfo stBTCPoolConf[MAX_POOL_COUNT];
	stPoolConfInfo stLTCPoolConf[MAX_POOL_COUNT];
}stPoolFixedConf;


typedef struct
{
	stMinerloginInput MinerData;
	stResMiner *pMinerRes;
	stParentListener MinerParent;
    union 								/**< far side of the connection */
    {
        sa_family_t af; 				/**< remote peer's address family: AF_INET*. */
        struct sockaddr_in  addr4; 		/**< IPv4 remote address */
        struct sockaddr_in6 addr6; 		/**< IPv6 remote address */
    } remote;

	stDevDataLock MinerDataLock;
	E_LOGIN_TYPE eClinetType;
	E_QUERY_TYPE eMinerMySql;           //通知数据库有新结点或者有部分数据更新
	E_COMMAND_TYPE eMinerComm;          //通知CommandPackProcess定时刷新什么命令，默认是get_stat
}stMinerConn;

typedef struct
{
	stCellloginInput CellData;
	//stPoolConfInfo BTCPoolConf[MAX_POOL_COUNT];
	//stPoolConfInfo LTCPoolConf[MAX_POOL_COUNT];
	//stResCell *pCellRes;
	//BYTE byDividXnonce[BUFFER_LEN_32];
	stParentListener CellParent;	
    union 								                /**< far side of the connection */
    {
        sa_family_t af; 				                /**< remote peer's address family: AF_INET*. */
        struct sockaddr_in  addr4; 		                /**< IPv4 remote address */
        struct sockaddr_in6 addr6; 		                /**< IPv6 remote address */
    } remote;
	
	stDevDataLock CellDataLock;
	E_LOGIN_TYPE eClinetType;
	E_QUERY_TYPE eCellMySql;                            //通知数据库有新结点或者有部分数据更新
	E_COMMAND_TYPE eCellComm;                           //通知CommandPackProcess定时刷新什么命令，默认是get_stat
	UINT uSeqId;                                        //用户发送序列号,用于解析时用
	stPoolFixedConf CellPoolConf;
	stCellStatus CellStat;
	E_DEV_DATAUPDATE eCellDataSer;                      //发送到dataserver的数据标志位
}stCellConn;

typedef struct
{
	int iConnIdx;
	stMinerConn *MinerConn;
}stMinerNode;

typedef struct
{
	IF_PROC_PTRPTRINT_INT ResToAlgoFunc;
	int *pCrrAlgoLen;
	int *pSumAlgoLen;
	//E_LOGIN_ERROR *pAlgoLenErr;
	IF_PROC_PTRPTRINT_INT DbResToAlgoFunc;
}stAssiAlgoRes;

/*typedef struct
{
	pthread_mutex_t *pMinerOnliLock;
	pthread_mutex_t *pCellOnliLock;
}stOnliLock;

typedef struct
{
	pthread_mutex_t *pMinerAlarmTimerLock;
	pthread_mutex_t *pCellAlarmTimerLock;
}stDevAlarmTimerLock;*/

typedef struct
{
	int LoginRecvCount;                                 //登录接受到的数据个数
	int CmdRecvCount;                                   //命令接口接受到的数据个数
}stSwiInfo;

typedef struct
{
	//long lDevId;                                        //dev_id设备唯一标识
	BYTE szDevId[BUFFER_LEN_28];                        //dev_id设备唯一标识
	int iId;                                            //switch识别miner或者cell的id
}stIdAllocTab;

typedef struct
{
	int iDelCellId;                                     //被删除时，保留的CellId
	E_QUERY_TYPE eDelCellIdMySql;                       //通知数据库有新结点或者有部分数据更新
}stDelIdAllocTab;

typedef struct
{
	BOOL bUserFlag[MAX_CLIENTS];                                //用户命令开启标志位
	//BOOL bMinerShellFlag[MAX_MINER_COMMAND_NUM][MAX_CLIENTS];
	BOOL bCellShellFlag[MAX_CELL_COMMAND_NUM][MAX_CLIENTS];
	BOOL bRecvSeqFlag[MAX_CLIENTS];                             //用户ID标志位
	UINT uUserSeq[MAX_CLIENTS];                                 //用户发送序列号,用于解析时用
	pthread_mutex_t *CommShellFlagLock;                         //command和shell之间开启与关闭锁
	pthread_mutex_t *UserSeqLock;                               //接受数据序列号锁
	BOOL bMassFlag;                                             //群发标志位
	int iSendLen;                                               // 发送了几包数据的长度
	int iRecvLen;                                               //接受了几包数据的长度	
}stCommShellFlag;

typedef struct
{
	BOOL bServerCommFlag;                                       //用户命令开启标志位
	//BOOL bMinerShellFlag[MAX_MINER_COMMAND_NUM][MAX_CLIENTS];
	BOOL bCellMonitorFlag[MAX_CELL_COMMAND_NUM];
	BOOL bRecvSeqFlag;                                          //用户ID标志位
	UINT uUserSeq;                                              //用户发送序列号,用于解析时用
	BOOL bMassFlag;                                             //群发标志位
	//BOOL bPartFlag;                                             //部分设备发送标志位
	BYTE szIdbufRtn[BUFFER_LEN_1024];                            //把错误cell_id从command layer返回给monitor layer
	int iBufPos;                                                //buf位置
	int iSendLen;                                               // 发送了几包数据的长度
	int iRecvLen;                                               //接受了几包数据的长度
	int iOnlineCount;                                           //command sendAll 时 统计的在线设备个数

	pthread_mutex_t *CommMonitorFlagLock;						//command和shell之间开启与关闭锁
	pthread_mutex_t *UserSeqLock;								//接受数据序列号锁
	pthread_mutex_t *OnlineCountLock;                           //iOnlineCount锁
}stCommMonitorFlag;

typedef struct
{
	BYTE type[BUFFER_LEN_8];
}stType;

typedef enum
{
	E_COMM_TYPE_SHELL = 1,
	E_COMM_TYPE_MONITOR,
}E_COMM_TYPE;

stApiNetConnPool *gpLoginPool;
stApiNetPoll *gpPoller;
stAlgo Algo[ALGO_LEN];
stQueue *gpLoginQue;
stSwiInfo SwitchInfo;
//stOnliLock OnlineLock;
//stDevAlarmTimerLock DevAlarmTimerLock;
E_COMMAND_TYPE geCommLoopType;			                //开启什么样的command
stCommShellFlag CommShellFlag;
stCommMonitorFlag CommMonitorFlag;
stCommandErr gUserComdErr[COMM_ERR_NUM];
stCommandErr gPoolConfErr[POOLCONF_ERR_NUM];

//临时命令类型
int gCommandType;

//登陆队列有没有值
BOOL bLoginQueValue;

pthread_mutex_t gLoginMonitorLock;
int gNewCellSendLen;                                    //New Cell已经发送长度


//池的限制缓冲相关信息，最多只能保存3个
/*int gBTCPoolCount, gLTCPoolCount;
stPoolConfInfo gBTCPoolConf[MAX_POOL_COUNT];
stPoolConfInfo gLTCPoolConf[MAX_POOL_COUNT];
int PoolIdPri[MAX_POOL_COUNT*2];*/
stPoolFixedConf gPoolDefaultConf;                        //池的固定大小，每种算法最多三个
stPoolFixedConf gPoolSpecifyConf;                        //对于指定cell的指定矿池，每种算法最多三个

//组播ip,port
//struct sockaddr_in  gMulticastAddr4; 
//char gszMulticastIp[BUFFER_LEN_16] = {"239.255.0.0"};        //组播的ip	
//int	giMulticastPort = 3500;                                   //组播port	

int ParseAlgo(int argc, char**argv, int iStartPos, E_ALGO *peAlgo);
int ParseShellCommand(int argc, char **argv, int iStartPos);
stCommandErr gCommandErr[ERROR_NUM];
//int PoolToFixediBuffer();

#endif

