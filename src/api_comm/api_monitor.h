#ifndef __API_MONITOR__
#define __API_MONITOR__

#include "../app/typedefs.h"
#include "def_devstr.h"
#include "../list/uthash/my_uthash.h"
#include "../api_net/connection_pool/connection_pool.h"

#define CLIENT_SIZE                           1
#define TCP_CLIENT_TIMEOUT                    10000
#define MONITOR_CELL_LIST_PACK_NUM            250//50//40
#define MONITOR_REPLY_COMM                    5

typedef enum
{
	E_MONITOR_COMMAND_TYPE_VALIDATOR = 0,    //validator
	E_MONITOR_COMMAND_TYPE_CELLINFO,         //cell.info
	E_MONITOR_COMMAND_TYPE_REBOOT,           //cell.reboot
	E_MONITOR_COMMAND_TYPE_SETLED,           //cell.setled
	E_MONITOR_COMMAND_TYPE_SET_POOL,         //cell.setpool
	E_MONITOR_COMMAND_TYPE_SET_DEFAULTPOOL,  //cell.setdefaultpool
	E_MONITOR_COMMAND_TYPE_CELL_SEARCH       //cell.search

}E_MONITOR_COMMAND_TYPE;

typedef struct
{
	BYTE byReCmdBuf[BUFFER_LEN_256];         //接受缓冲
} stClientUserData;

typedef struct
{
    BYTE bySeCmdBuf[BUFFER_LEN_512K];        //发送缓冲
    int iSubindex;                          //分包下标		
    int iNumPack;                           //包个数
    int iRemain;                            //完整包后的剩余数据
    BOOL bSubcontFlag;                      //分包开始标志位
    int iNumStart;                          //接受到的链表起始位置
    int iNumEnd;                            //接受到的链表结束位置    
    devlist_st *pNextNode;                  //分包后下一个结点的信息
    //int iNewCellSendLen;                    //Cell发送长度
}stClientSendData;

typedef enum
{
	E_SET_MONITOR_ALL_SUCC,				     //全部成功
	E_SET_MONITOR_ALL_INTER,				 //全部中断
	E_SET_MONITOR_PART_FAIL,                 //部分通信失败
	E_SET_MONITOR_CELLID_NOEXIST             //cell_id不存在
}E_SET_MONITOR;

typedef struct
{
	UINT *pIdSeq;                            //发送id序列号
	LPSTR pBuf;                              //传入的数据缓冲区
	structUthash *pList;                     //链表
	stMonitor *pMonitorComm;                 //命令指针
	stSwitchDbInfo *pSwiDbInfo;              //switch的信息
	UINT *pSendInterval;                     //发送时间间隔，每30秒发送switch.validator，每60秒发送cell.info
	int iCommLen;                            //command 长度 
	CHAR szParseDest[BUFFER_LEN_20K];        //因为解析在command，不在Monitor这层，解析完的数据回传到发送BUF
}stMonitorSrc;



struct ap_net_conn_pool_t *tcp_client;
stClientUserData gCliUserData;
stClientSendData gCliSendDaTa;
stMonitorSrc MoniSrc;

void InitMonitor();

void TCPClientDestroy();
//int ClientCreated(void *conn);
//int ClientClose(void *conn);
//int client_callback(struct ap_net_connection_t *conn, int signal_type);

//void *TCPClient(void *arg);

int NoPackMonitor(int argc, char **argv, void *dest, int destLen, void *pParData);
int PackSwitchvalidator(void *pSrc, void *dest, int destLen, int *pSendLen);
int PackCellAllInfo(void *pSrc, void *dest, int destLen, int *pSendLen);
int PackCellRebootMonitorToComm(int argc, char **argv, void *dest, int destLen, void *pParData);
int PackCellSetLedMonitorToComm(int argc, char **argv, void *dest, int destLen, void *pParData);
int PackCellAddPoolMonitorToComm(int argc, char **argv, void *dest, int destLen, void *pParData);
int PackCellDelPoolMonitorToComm(int argc, char **argv, void *dest, int destLen, void *pParData);
int PackSetdefaultPoolMonitorToComm(int argc, char **argv, void *dest, int destLen, void *pParData);
int PackSetPoolMonitorToComm(int argc, char **argv, void *dest, int destLen, void *pParData);

int NoParseMonitor(void *pJson, void *dest, int destLen, int *pSize);
int ParseRebootCommand(void *pJson, void *dest, int destLen, int *pSize);
int ParseSetLedCommand(void *pJson, void *dest, int destLen, int *pSize);
int ParseAddPoolCommand(void *pJson, void *dest, int destLen, int *pSize);
int ParseDelPoolCommand(void *pJson, void *dest, int destLen, int *pSize);
int ParseSetdefaultPoolCommand(void *pJson, void *dest, int destLen, int *pSize);
int ParseSetPoolCommand(void *pJson, void *dest, int destLen, int *pSize);
int ParseCellSearch(void * pJson, void *dest, int destLen, int *pSize);
int ParseRefuse(void *pJson, void *dest, int destLen, int *pSize);

int ParseReplay(void * pJson, void *dest, int destLen, int *pSize);

//int PackCellNewInfo(void *pSrc, void *dest, int destLen, stCellConn *val);

int client_callback(struct ap_net_connection_t *conn, int signal_type);
void *MonitorProcess(void *arg);

int dbgdumpGetPooSpecifyConf();

#endif

