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
	unsigned   int   0��4294967295   
	int   2147483648��2147483647 
	unsigned long 0��4294967295
	long   2147483648��2147483647
	long long�����ֵ��9223372036854775807
	long long����Сֵ��-9223372036854775808
	unsigned long long�����ֵ��1844674407370955161
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
	E_QUERY_TYPE_INSERT = 1,                                    //����
	E_QUERY_TYPE_SELECT,                                        //��ѯ
	E_QUERY_TYPE_UPDATE,                                        //����
	E_QUERY_TYPE_ALTER,                                         //�޸�
	E_QUERY_TYPE_DELETE,                                        //ɾ��
	E_QUERY_TYPE_CREATETABLE                                    //������
	//E_QUERY_TYPE_SELECT_MAX                                     //��ѯ����
	//E_QUERY_TYPE_CREATEDB,                                      //�������ݿ�
	//E_QUERY_TYPE_CREATETABLE                                    //������
}E_QUERY_TYPE;

typedef enum 
{
	E_COMMAND_TYPE_STAT = 0,                                    //get_stat��ȡ״̬��
	E_COMMAND_TYPE_RELOGIN,                                     //relogin���µ�½
	E_COMMAND_TYPE_REBOOT,                                      //reboot����	
	E_COMMAND_TYPE_RESETFLASH,                                  //resetflash���flash
	E_COMMAND_TYPE_SETLED,                                      //set_led����LED
	E_COMMAND_TYPE_SETPOOL                                      //setpool
}E_COMMAND_TYPE;

typedef enum 
{
	E_DEV_DATAUPDATE_NEWCELL = 1,                               //����cell
	E_DEV_DATAUPDATE_CELLNEWDATA                                //cell�����ݸ���
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
	int iMinerId;                       //�豸id
	//BYTE byMinerId[BUFFER_LEN_32];
	E_ALGO eMinerAlgo;                  //�㷨
	BOOL bMinerOnline;                  //Miner�Ƿ�����
	BYTE byPoolName[BUFFER_LEN_32];     //������
	BYTE byPoolStat[BUFFER_LEN_16];     //��״̬
	long iMinerAlarmTimer;              //Minerͨ���ж϶�ʱ�����������3�η��ʹ��������Online��ΪFALSE
	long iRegtime;                      //ע��ʱ��
	long iLoginTime;                    //��½ʱ��
	long iLogoutTime;                   //�ǳ�ʱ��

	BYTE byAlarm[BUFFER_LEN_4];         //Ŀǰ����������Ϊ4
	//int iMinerTestNum;                             //������
}stMinerloginInput;

typedef struct
{
	//
	//long lDevId;                        //�豸id
	BYTE szCellId[BUFFER_LEN_28];       //�豸id
	int iCellId;                        //cellʶ��id
	BYTE szDevId[BUFFER_LEN_28];        //�豸id
	BYTE szSwVer[BUFFER_LEN_16];        //����汾
	BYTE szHwVer[BUFFER_LEN_16];        //Ӳ���汾	
	
	UINT iSwVer;                        //����汾
	UINT iHwVer;                        //Ӳ���汾	
	E_ALGO eCellAlgo;                   //�㷨
	UINT iChipCnt;                      //оƬ����
	LLONG iChipBTCHash;                 //����оƬ�����
	LLONG iChipLTCHash;                 //����оƬ�����
	UINT iChipBTCFreq;                  //оƬ��Ƶ
	UINT iChipLTCFreq;                  //оƬ��Ƶ
	BOOL bCellOnline;                   //Cell�Ƿ�����
	BYTE szTestResult[BUFFER_LEN_28];	//оƬ�����
	long iCellAlarmTimer;               //Cellͨ���ж϶�ʱ�����������3�η��ʹ��������Online��ΪFALSE
	long iRegtime;                      //ע��ʱ��
	long iLoginTime;                    //��½ʱ��
	long iLogoutTime;                   //�ǳ�ʱ��

	LLONG lBTCRatedKHash;               //��������㷽����iChipBTCHash*iChipCnt/VALUES_1000
	LLONG lLTCRatedKHash;               //��������㷽����iChipLTCHash*iChipCnt/VALUES_1000

	BYTE byAlarm[BUFFER_LEN_4];         //Ŀǰ����������Ϊ4
	//int iCellTestNum;                                //������
}stCellloginInput;

typedef struct
{
	BYTE byTestResult[BUFFER_LEN_28];   //оƬ�����
	int iEntryTemp;                     //����¶�
	int iExitTemp;                      //�����¶�
	int iElapse;                        //����ʱ��
	ULLONG ullBTCReject;                //BTC nonce�ܾ���
	ULLONG ullBTCAccept;                //BTC nonce������
	ULLONG ullLTCReject;                //LTC nonce�ܾ���
	ULLONG ullLTCAccept;                //LTC nonce������
	int iBTCNcIndex;                    //BTCNcsend�±�
	int iLTCNcIndex;                    //LTCNcsend�±�
	ULLONG llBTCNcsend[BUFFER_LEN_32];  //in 30Min BTC hashsum
	ULLONG llLTCNcsend[BUFFER_LEN_32];  //in 30Min BTC hashsum
               
	//ʵ���������㷽������λ��khash/s��iBTCRealKHash=(|lBTCNcsend[Crr] - lBTCNcsend[30M before]| * BTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000)
	ULLONG llBTCRealKHash; 
	//ʵ���������㷽������λ��khash/s��iLTCRealKHash=(|lLTCNcsend[Crr] - lLTCNcsend[30M before]| * LTC_SHARE) / (INTERVAL_30_SECOND * VALUES_1000)
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
	int iWbIndex;                       //workbase������е��±꣬�������bWbNoFlag
	int iWbsId;                         //��ӦstResource�е�bWbNo(workbase���)
	int iConnCellNum;                   //ͬһ��ip��port�ָ�����Cell	
	stWbIpPort WbIpPort;				//ip port
}stResMiner;

typedef struct
{
	int iWbIndex;                       //workbase������е��±�
	int iConnCellIndex;                 //Cell�±꣬�������pbWbCellNoFlag
	int iWbNo;                          //workbase���
	int iConnCellNum;                   //ͬһ��ip��port�ָ�����Cell	
	int iWbcId;                         //��Ӧ	stResource�е�	pWbCellNo(ÿ��workbase�е�cell���	)					
	stWbIpPort WbIpPort;                //ip port
}stResCell;

typedef struct
{
	pthread_mutex_t OnlineLock;         //�Ƿ�������
	pthread_mutex_t AlarmTimerLock;     //������
	pthread_mutex_t IpPortLock;         //ip��port��
	pthread_mutex_t PoolConfLock;       //pool������
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
	E_QUERY_TYPE eMinerMySql;           //֪ͨ���ݿ����½������в������ݸ���
	E_COMMAND_TYPE eMinerComm;          //֪ͨCommandPackProcess��ʱˢ��ʲô���Ĭ����get_stat
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
	E_QUERY_TYPE eCellMySql;                            //֪ͨ���ݿ����½������в������ݸ���
	E_COMMAND_TYPE eCellComm;                           //֪ͨCommandPackProcess��ʱˢ��ʲô���Ĭ����get_stat
	UINT uSeqId;                                        //�û��������к�,���ڽ���ʱ��
	stPoolFixedConf CellPoolConf;
	stCellStatus CellStat;
	E_DEV_DATAUPDATE eCellDataSer;                      //���͵�dataserver�����ݱ�־λ
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
	int LoginRecvCount;                                 //��¼���ܵ������ݸ���
	int CmdRecvCount;                                   //����ӿڽ��ܵ������ݸ���
}stSwiInfo;

typedef struct
{
	//long lDevId;                                        //dev_id�豸Ψһ��ʶ
	BYTE szDevId[BUFFER_LEN_28];                        //dev_id�豸Ψһ��ʶ
	int iId;                                            //switchʶ��miner����cell��id
}stIdAllocTab;

typedef struct
{
	int iDelCellId;                                     //��ɾ��ʱ��������CellId
	E_QUERY_TYPE eDelCellIdMySql;                       //֪ͨ���ݿ����½������в������ݸ���
}stDelIdAllocTab;

typedef struct
{
	BOOL bUserFlag[MAX_CLIENTS];                                //�û��������־λ
	//BOOL bMinerShellFlag[MAX_MINER_COMMAND_NUM][MAX_CLIENTS];
	BOOL bCellShellFlag[MAX_CELL_COMMAND_NUM][MAX_CLIENTS];
	BOOL bRecvSeqFlag[MAX_CLIENTS];                             //�û�ID��־λ
	UINT uUserSeq[MAX_CLIENTS];                                 //�û��������к�,���ڽ���ʱ��
	pthread_mutex_t *CommShellFlagLock;                         //command��shell֮�俪����ر���
	pthread_mutex_t *UserSeqLock;                               //�����������к���
	BOOL bMassFlag;                                             //Ⱥ����־λ
	int iSendLen;                                               // �����˼������ݵĳ���
	int iRecvLen;                                               //�����˼������ݵĳ���	
}stCommShellFlag;

typedef struct
{
	BOOL bServerCommFlag;                                       //�û��������־λ
	//BOOL bMinerShellFlag[MAX_MINER_COMMAND_NUM][MAX_CLIENTS];
	BOOL bCellMonitorFlag[MAX_CELL_COMMAND_NUM];
	BOOL bRecvSeqFlag;                                          //�û�ID��־λ
	UINT uUserSeq;                                              //�û��������к�,���ڽ���ʱ��
	BOOL bMassFlag;                                             //Ⱥ����־λ
	//BOOL bPartFlag;                                             //�����豸���ͱ�־λ
	BYTE szIdbufRtn[BUFFER_LEN_1024];                            //�Ѵ���cell_id��command layer���ظ�monitor layer
	int iBufPos;                                                //bufλ��
	int iSendLen;                                               // �����˼������ݵĳ���
	int iRecvLen;                                               //�����˼������ݵĳ���
	int iOnlineCount;                                           //command sendAll ʱ ͳ�Ƶ������豸����

	pthread_mutex_t *CommMonitorFlagLock;						//command��shell֮�俪����ر���
	pthread_mutex_t *UserSeqLock;								//�����������к���
	pthread_mutex_t *OnlineCountLock;                           //iOnlineCount��
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
E_COMMAND_TYPE geCommLoopType;			                //����ʲô����command
stCommShellFlag CommShellFlag;
stCommMonitorFlag CommMonitorFlag;
stCommandErr gUserComdErr[COMM_ERR_NUM];
stCommandErr gPoolConfErr[POOLCONF_ERR_NUM];

//��ʱ��������
int gCommandType;

//��½������û��ֵ
BOOL bLoginQueValue;

pthread_mutex_t gLoginMonitorLock;
int gNewCellSendLen;                                    //New Cell�Ѿ����ͳ���


//�ص����ƻ��������Ϣ�����ֻ�ܱ���3��
/*int gBTCPoolCount, gLTCPoolCount;
stPoolConfInfo gBTCPoolConf[MAX_POOL_COUNT];
stPoolConfInfo gLTCPoolConf[MAX_POOL_COUNT];
int PoolIdPri[MAX_POOL_COUNT*2];*/
stPoolFixedConf gPoolDefaultConf;                        //�صĹ̶���С��ÿ���㷨�������
stPoolFixedConf gPoolSpecifyConf;                        //����ָ��cell��ָ����أ�ÿ���㷨�������

//�鲥ip,port
//struct sockaddr_in  gMulticastAddr4; 
//char gszMulticastIp[BUFFER_LEN_16] = {"239.255.0.0"};        //�鲥��ip	
//int	giMulticastPort = 3500;                                   //�鲥port	

int ParseAlgo(int argc, char**argv, int iStartPos, E_ALGO *peAlgo);
int ParseShellCommand(int argc, char **argv, int iStartPos);
stCommandErr gCommandErr[ERROR_NUM];
//int PoolToFixediBuffer();

#endif

