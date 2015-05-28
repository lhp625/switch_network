#ifndef API_COMMAND_H
#define API_COMMAND_H

#include "../app/typedefs.h"
#include "../api_net/api_net.h"
#include "../fifo_queue/circular_queue.h"
//#include "api_login.h"
#include "../list/uthash/my_uthash.h"
#include "def_devstr.h"

#define MAX_COMMAND_NUM				1
#define SEND_ID_LENGTH				(1ull<<32) - 1

#define MAX_TIMES_3					3 
#define COMM_RTNERROR_MONITOR       5
#define SETLED_TYPE                 4
#define REBOOT_TYPE                 6

typedef enum
{
	E_COMMAND_ERROR_TYPE = 1,				                    //类型错误(miner/cell)
	E_COMMAND_ERROR_PARAM_NUM,				                    //参数个数错误
	E_COMMAND_ERROR_ID,						                    //ID不存在
	E_COMMAND_ERROR_FOURTH_PARAM,			                    //第四个参数不存在
	E_COMMAND_ERROR_FIFTH_PARAM,			                    //第五个参数不存在
	E_COMMAND_ERROR_ALGO,					                    //无此算法
	E_COMMAND_ERROR_REBOOT,					                    //无此重启功能
	E_COMMAND_ERROR_LED,						                //无此LED功能
	E_COMMAND_ERROR_EMPTY,                                      //链表空
	E_COMMAND_ERROR_PARTID,                                     //还有id需要处理
	E_COMMAND_ERROR_JSON,                                       //获得一个错误的Json格式
	E_COMMAND_ERROR_POOL,                                       //处理pool发生的错误
	E_COMMAND_ERROR_IDNUM                                       //id个数错误
}E_COMMAND_ERROR;

typedef enum
{
	E_RECV_COMMAND_ERROR_PARAM_NUM = 1,                         //参数个数错误
	E_RECV_COMMAND_ERROR_IDFIRST_PARAM,                         //ID中第一个自定义参数不存在
	E_RECV_COMMAND_ERROR_IDSECOND_PARAM,                        //ID中第二个自定义参数不存在
	E_RECV_COMMAND_ERROR_IDTHRID_PARAM,                         //ID中第三个自定义参数不存在
	E_RECV_COMMAND_ERROR_USER_SEQ                               //用户发送序列号错误
}E_RECV_COMMAND_ERROR;

typedef struct
{
	CHAR (*pszstrHead)[BUFFER_LEN_8];
	LPSTR pstrControl;
	IF_PROC_PTRPTRPTRINTPINT_INT CommandPackFunc;
	IF_PROC_PTRPTRPTRINTINT_INT CommandSendAllFunc;
	IF_PROC_PTRPTRPTRINTINT_INT CommandSendSingleFunc;
	LPSTR pSrcBuf;                                              //打包发送时用到的数据
	LPSTR pDevId;                                               //MinerId或者CellId
	IF_PROC_INTLPPSTRINT_INT CommandParseFunc;
}stIdCommandPack;

typedef struct
{
	LPSTR pstrNodeHead;										
	structUthash *pList;                                        //链表
	structApiNet *pApiNet;                                      //网络通信
	stIdCommandPack *pIdCommandPack;                            //命令
	INT *pCommandLen;                                           //命令所占字节
	//INT *pCommandIndex;                                                                 //命令索引
	char (*pShellFlag)[MAX_CLIENTS];                            // shell Terminal启动标志位
	LONG *pSendCommTimer;                                       // 定时器
	char *pMonitorFlag;                                         // monitor启动标志位
}stIdControlInput;

typedef struct
{
	BYTE byReboot[BUFFER_LEN_8];
}stReboot;

typedef struct
{
	BYTE bySetLed[BUFFER_LEN_8];
}stSetLed;

typedef struct
{
	stQueue **pCirQue;
	stApiNetConnPool **pPool;
}stWorkProc;

//在解析cell_id[1,2,...]时，把多个id保存到buf中
typedef struct
{
	int iPartLen;                                              //收到总的id个数
	int iPartListLen;                                          //除去错误id后的id个数
	//int iPartIdBuf[BUFFER_LEN_256];                          //buf
	char iPartIdBuf[BUFFER_LEN_256][BUFFER_LEN_32];
}stPartData;

typedef struct
{
	BYTE cmd[BUFFER_LEN_100];
	IF_PROC_PTRPTRINTPINT_INT UserCmdFunc;
	IF_PROC_PTRPTRPTRINT_INT CommPackFunc;       //从Monitor 打包数据到command
}stJsonCommand;

typedef struct
{
	UINT *pIdSeq;                            //发送id序列号
	LPSTR pBuf;                              //传入的数据缓冲区
	structUthash *pList;                     //链表
	stJsonCommand *pCommandComm;                   //命令指针
	//stSwitchDbInfo *pSwiDbInfo;              //switch的信息
	//UINT *pSendInterval;                     //发送时间间隔，每30秒发送switch.validator，每60秒发送cell.info
	int iCommLen;                            //command 长度 
	//CHAR szParseDest[BUFFER_LEN_20K];        //因为解析在command，不在Monitor这层，解析完的数据回传到发送BUF
}stCommandSrc;


stQueue *gpCommandQue;//*gpMinerStatusDbQue, *gpCellStatusDbQue;
LONG giSendMinerCommTimer[MAX_MINER_COMMAND_NUM];
LONG giSendCellCommTimer[MAX_CELL_COMMAND_NUM];
stIdCommandPack idMinerControlInput[MAX_MINER_COMMAND_NUM];
stIdCommandPack idCellControlInput[MAX_CELL_COMMAND_NUM];
//char gMinerSrcBuf[MAX_MINER_COMMAND_NUM][BUFFER_LEN_256];
char gCellSrcBuf[MAX_CELL_COMMAND_NUM][BUFFER_LEN_256];
char gMinerId[MAX_MINER_COMMAND_NUM][BUFFER_LEN_32];            //MinerId
char gCellId[MAX_CELL_COMMAND_NUM][BUFFER_LEN_32];              //CellId
const stIdControlInput idControlInput[MAX_COMMAND_NUM];
UINT gSendSeq;                                                  //全局变量，除get_alive以外，每发送一包序列号加1
char szRetBuf[COMM_RTNERROR_MONITOR][BUFFER_LEN_16];
stPartData ParData;
stSetLed gSetLed[SETLED_TYPE];
stReboot gReboot[REBOOT_TYPE];


void InitCommand();
int AlgoEnumToString(E_ALGO eAlgo, char *dest, int destLen);
int SetSpecifyPoolMonitor
(int argc, char **argv, void *destPackShell, int destLenPackShell, void *destErr, int destLenErr);
int SetSpecifyPoolFindCellId
(int argc, char **argv, void *destPackShell, int destLenPackShell, void *destErr, int destLenErr);

int PackMinerGetStatCommand(void *pMinCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackMinerReloginCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackMinerRebootCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackMinerAlgoCommand(void *pMinCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackMinerGetdetailCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackCellGetStatCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackCellReloginCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackCellRebootCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackCellTemperatureCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackCellAddMinerCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackCellDelMinerCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackCellSetLedCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackCellUpdateSwCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackCellStartTestCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackCellGetTestCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackCellGetTestResultCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackCellSetFreqCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackCellSetFreqCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int PackCellResetFlashCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int ParseShellMinerSetAlgoCommand(int argc, char **argv, int iStartPos);
int ParseShellMinerReloginCommand(int argc, char **argv, int iStartPos);
int ParseShellMinerGetdetailCommand(int argc, char **argv, int iStartPos);
int ParseShellCellRebootCommand(int argc, char **argv, int iStartPos);
int ParseShellCellGetTemperatureCommand(int argc, char **argv, int iStartPos);
int ParseShellCellStartTestCommand(int argc, char **argv, int iStartPos);
int ParseShellCellSetLedCommand(int argc, char **argv, int iStartPos);
int ParseShellCellTftpCommand(int argc, char **argv, int iStartPos);
int ParseShellCellGetTestResultCommand(int argc, char **argv, int iStartPos);
int ParseShellCellCustomCommand(int argc, char **argv, int iStartPos);
int ParseShellCellReloginCommand(int argc, char **argv, int iStartPos);
int ParseShellCellSetFreqCommand(int argc, char **argv, int iStartPos);
int ParseShellCellResetFlashCommand(int argc, char **argv, int iStartPos);
int ParseShellSetPoolCommand(int argc, char **argv, int iStartPos);
int NoParseShellCommand(int argc, char **argv, int iStartPos);
int ParseMonitorCommand(int argc, char **argv, int iStartPos, stPartData *pParData);

int PackJsonCellGetStatCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int JsonPackRebootCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int JsonPackSetLedCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int JsonPackNoParamCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int JsonPackSetFreqCommand(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);
int JsonPackSetPool(void *pCoon, void *pCommand, void *dest, int destLen, int *pLen);

int JsonParseStat(void * pJson, void *dest, int destLen, int *pId);
int JsonParseResult(void * pJson, void *dest, int destLen, int *pId);

int SendMinerSingle(void *pList, void *pApiNet, void *pCommand, int iSrcIndex, int CommandSock);
int SendMinerAll(void *pList, void *pApiNet, void *pCommand, int iSrcIndex, int CommandSock);
int SendCellSingle(void *pList, void *pApiNet, void *pCommand, int iSrcIndex, int CommandSock);
int SendCellAll(void *pList, void *pApiNet, void *pCommand, int iSrcIndex, int CommandSock);

void *CommandPackProcess(void *arg);
//void *CommandRecvProcess(void *arg);
void *CommandParseProcess(void *arg);
void *PollRecvProcess(void *arg);

void dbgdumpCommRecv(void);
void dbgdumpCommSend(void);

#endif
