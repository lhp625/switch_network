#ifndef API_LOGIN_H
#define API_LOGIN_H

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include "../app/typedefs.h"
#include "../fifo_queue/fifo_queue.h"
#include "../list/uthash/my_uthash.h"

#define MINER_LOGIN_COMMANDLINE     6
#define CELL_ALLOCID_COMMANDLINE    6
#define CELL_LOGIN_COMMANDLINE      14//15

typedef struct
{
	BYTE cmd[BUFFER_LEN_100];
	IF_PROC_PTRPTRINTPINT_INT UserCmdFunc;
	IF_PROC_PTRPTRPTRINT_INT CommPackFunc;       //从Monitor 打包数据到command
}stLogin;

typedef struct
{
	UINT *pIdSeq;                            //发送id序列号
	LPSTR pBuf;                              //传入的数据缓冲区
	structUthash *pList;                     //链表
	stLogin *pLoginComm;                     //命令指针
	//stSwitchDbInfo *pSwiDbInfo;              //switch的信息
	//UINT *pSendInterval;                     //发送时间间隔，每30秒发送switch.validator，每60秒发送cell.info
	int iCommLen;                            //command 长度 
	CHAR szParseDest[BUFFER_LEN_20K];        //因为解析在command，不在Monitor这层，解析完的数据回传到发送BUF
}stLoginSrc;

int QueueWrite(queue *que, char *buf);
/*int ResBLToMiner(void *val, void *pRes, int MinerNum);
int ResBLToCell(void *val, void *pRes, int MinerNum);
int ResBTCToMiner(void *val, void *pRes, int MinerNum);
int ResLTCToMiner(void *val, void *pRes, int MinerNum);
int ResBTCToCell(void *val, void *pRes, int MinerNum);
int ResLTCToCell(void *val, void *pRes, int MinerNum);
int DbResBTCToMiner(void *val, void *pRes, int MinerNum);
int DbResLTCToMiner(void *val, void *pRes, int MinerNum);
int DbResBLToMiner(void *val, void *pRes, int MinerNum);
int DbResBTCToCell(void *val, void *pRes, int MinerNum);
int DbResLTCToCell(void *val, void *pRes, int MinerNum);
int DbResBLToCell(void *val, void *pRes, int MinerNum);*/

int PackJsonCellLogin(void *pSrc, void *pDev, void *dest, int destLen);

int ParseJsonCellLogin(void * pJson, void *dest, int destLen, int *pId);

int ParseMinerLogin(int argc,char * * argv,int iStartPos);
int ParseCellLogin(int argc,char * * argv,int iStartPos);

//int AllocDbToMinerRes(int argc, char**argv, int iStartPos, E_ALGO eAlgo, stResMiner **pResource);
//int AllocDbToCellRes(int argc, char**argv, int iStartPos, E_ALGO eAlgo, stResCell **pResource);

//int IncrCrrentAlgoLen(E_ALGO eAlgo, stAssiAlgoRes *pDevAssi);
void InitLogin();
void InitLoginQue();
void *LoginParseProcess(void *arg);
//void *LoginRecvProcess(void *arg);

void dbgdumpLoginRecvLen(void);
void dbgdumpLoginRecv(void);
void dbgdumpLoginSend(void);

#endif

