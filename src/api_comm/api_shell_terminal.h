#ifndef __API_SHELL_TERMINAL__
#define __API_SHELL_TERMINAL__

#include "../app/typedefs.h"

#define MAX_TESTS_PER_CLIENT    1000
#define CONNECTION_TIMEOUT      10000
#define MAX_COMMAND_BUF         100
#define MINER_LIST_PACK_NUM     512
#define CELL_LIST_PACK_NUM      1024
//#define ID_STACK_MAX               MAX_CLIENTS * MAX_TESTS_PER_CLIENT

typedef enum
{
    E_USER_STATE_CONNECTED,             //"Connected"
    E_USER_STATE_GOT_MESSAGE,           //"Client's message received"
    E_USER_STATE_ANSWER_SENT,           //"Answer sent to client"
    E_USER_STATE_MSG_SENT,              //"Message sent to server"
    E_USER_STATE_GOT_ANSWER             //"Answer received from server"
}E_USER_STATE;

typedef enum
{
	E_SHELL_ERROR_TIMEOUT,              //超时
	E_SHELL_ERROR_COMMAND,              //命令不存在
	E_SHELL_ERROR_GETMINER,             //get_miner 命令错误
	E_SHELL_ERROR_GETCELL,              //get_cell 命令错误
	E_SHELL_ERROR_DELMINER,             //del_miner命令错误
	E_SHELL_ERROR_DELCELL,              //del_cell命令错误
	E_SHELL_ERROR_ADJCPM,               //adj_cpm命令错误
	E_SHELL_ERROR_SET_COMM,	            //参数个数错误
	E_SHELL_ERROR_GETCELLID,            //get_cellid命令错误
	E_SHELL_ERROR_ADDPOOL,              //addpool 是错误的
	E_SHELL_ERROR_DELPOOL,              //delpool 是错误的
	E_SHELL_ERROR_SETPOOL,              //setpool是错误的
	E_SHELL_ERROR_SETCELL               //set_cell是错误的
}E_SHELL_ERROR;

typedef enum
{
	E_GET_SET_ERROR_PARAM_FEW = 1,          //参数太少
	E_GET_SET_ERROR_PARAM_MANY,             //参数太多
	E_GET_SET_ERROR_EMPTY,                  //链表空
	E_GET_SET_ERROR_START_ID,               //起始id不存在
	//E_GET_ERROR_START_OUTRAGE,          //超出起始位置范围
	E_GET_SET_ERROR_END_ID,                 //结束id不存在
	//E_GET_ERROR_END_OUTRAGE,            //超出结束位置范围
	E_GET_SET_ERROR_NO_LESS,                //起始id不能小于结束id
	E_GET_SET_ERROR_ALL                     //获得所有信息参数是get[] 0 0
}E_GET_SET_ERROR;

typedef enum
{
	E_DEL_ERROR_PARAM_FEW = 1,          //参数太少
	E_DEL_ERROR_PARAM_MANY,             //参数太多
	E_DEL_ERROR_EMPTY,                  //链表空
	E_DEl_ERROR_ID,                     //ID不存在
	E_DEL_ERROR_WB_INDEX,               //当前workbase的下标，已经超出了最大长度
	E_DEL_ERROR_ALGO,                   //无此算法
	E_DEL_ERROR_CONNCELL_INDEX,         //Cell下标
	E_DEL_ERROR_DB_OP,                  //数据库操作失败
	E_DEL_ERROR_DB_CONNECT              //数据库插入失败
	//E_DEL_ERROR_DEL_MINER_FLAG          //删除Miner标志位出现错误
}E_DEL_ERROR;

typedef enum
{
	E_POOLCNF_ERROR_POOLID = 1,          //pool id错误
	E_POOLCNF_ERROR_POOLID_NOZERO,       //pool_id不能为0
	E_POOLCNF_ERROR_ALGO,    		     //algo is error.
	E_POOLCNF_ERROR_POOLID_EXIST,        //pool id不存在
	E_POOLCNF_ERROR_DUAL,                //dual暂时不支持。
	E_POOLCNF_ERROR_SEC_STAGE,           //第二级解析错误
	E_POOLCNF_ERROR_TOO_MANYPOOLID,      //算法后面的pool_id太多
	E_POOLCNF_ERROR_TOO_FEWPOOLID        //算法后面的pool_id太少
	//E_POOLCNF_ERROR_EMPTY,               //链表空
	//E_POOLCNF_ERROR_ID_OVER_LIST,        //id超过链表
	//E_POOLCNF_ERROR_ID                   //id不存在
}E_POOLCNF_ERROR;

typedef enum
{
	E_SET_MASS_ALL_SUCC,				  //全部成功
	E_SET_MASS_ALL_INTER,				  //全部中断
	E_SET_MASS_PART_FAIL,                 //部分通信失败
}E_SET_MASS;

typedef struct
{
	char cmd[BUFFER_LEN_256];
	E_USER_STATE state;
} server_userdata;

typedef struct
{
	BYTE cmd[BUFFER_LEN_32];
	IF_PROC_INTLPPSTRPTRINT_INT UserCmdFunc;
}stUserCmd;

typedef struct
{
    BYTE byUserCmdBuf[BUFFER_LEN_512K]; //发送缓冲
    int iSubindex;                      //分包下标		
    int iNumPack;                       //包个数
    int iRemain;                        //完整包后的剩余数据
    BOOL bSubcontFlag;                  //分包开始标志位
    int iNumStart;                      //接受到的链表起始位置
    int iNumEnd;                        //接受到的链表结束位置
}stUserSendBuf;



void InitShell();
int ShellParseSend(void *conn);
int ParseAddPool(int argc, char **argv, int iListLen, void *eAlgoTemp);
int ParseDelPool(int argc, char **argv, int iListLen, void *vPoolIdTemp, int type);
int ParseSetPool(int argc, char **argv, int iStartPos, void *vPoolErr);

int PackErrorTimeOut(void *src, void *dest, int destLen);
int PackErrorGetMiner(void *src, void *dest, int destLen);
int PackErrorGetCell(void *src, void *dest, int destLen);
int PackErrorDelMiner(void *src, void *dest, int destLen);
int PackErrorDelCell(void *src, void *dest, int destLen);
int PackErrorAdjCpm(void *src, void *dest, int destLen);
int PackErrorSetComm(void *src, void *dest, int destLen);
int PackErrorGetCellId(void *src, void *dest, int destLen);
int PackErrorAddPool(void *src, void *dest, int destLen);
int PackErrorDelPool(void *src, void *dest, int destLen);
int PackErrorSetPool(void *src, void *dest, int destLen);
int PackErrorSetCell(void *src, void *dest, int destLen);
int PackSwitchInfo(int argc, char **argv, void *dest, int destLen);
int PackHelp(int argc, char **argv, void *dest, int destLen);
//int PackGetMiner(int argc, char **argv, void *dest, int destLen);
int PackGetCell(int argc, char **argv, void *dest, int destLen);
//int PackDelMiner(int argc, char **argv, void *dest, int destLen);
int PackDelCell(int argc, char **argv, void *dest, int destLen);
//int PackAdjCpm(int argc, char **argv, void *dest, int destLen);
int PackComm(int argc, char **argv, void *dest, int destLen);
int PackCellRebootToComm(int argc, char **argv, void *dest, int destLen);
int PackCellLedToComm(int argc, char **argv, void *dest, int destLen);
int PackStartCellTestToComm(int argc, char **argv, void *dest, int destLen);
int PackCellReloginToComm(int argc, char **argv, void *dest, int destLen);
//int PackMinerReloginToComm(int argc, char **argv, void *dest, int destLen);
int PackGetAllCellId(int argc, char **argv, void *dest, int destLen);
int PackGetAllDelCellId(int argc, char **argv, void *dest, int destLen);
int PackGetPoolConf(int argc, char **argv, void *dest, int destLen);
int PackGetDefaultPoolConf(int argc, char **argv, void *dest, int destLen);
int PackGetPooSpecifyConf(int argc, char **argv, void *dest, int destLen);
int PackCellSetFreqToComm(int argc, char **argv, void *dest, int destLen);
int PackAddPool(int argc, char **argv, void *dest, int destLen);
int PackDelPool(int argc, char **argv, void *dest, int destLen);
int PackSetPool(int argc, char **argv, void *dest, int destLen);
int PackSetCell(int argc, char **argv, void *dest, int destLen);
int PackCellRestFlashToComm(int argc, char **argv, void *dest, int destLen);

void ShellTeiminalDestroy();

void *ShellTerminalProcess(void *arg);

#endif

