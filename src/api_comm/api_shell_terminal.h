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
	E_SHELL_ERROR_TIMEOUT,              //��ʱ
	E_SHELL_ERROR_COMMAND,              //�������
	E_SHELL_ERROR_GETMINER,             //get_miner �������
	E_SHELL_ERROR_GETCELL,              //get_cell �������
	E_SHELL_ERROR_DELMINER,             //del_miner�������
	E_SHELL_ERROR_DELCELL,              //del_cell�������
	E_SHELL_ERROR_ADJCPM,               //adj_cpm�������
	E_SHELL_ERROR_SET_COMM,	            //������������
	E_SHELL_ERROR_GETCELLID,            //get_cellid�������
	E_SHELL_ERROR_ADDPOOL,              //addpool �Ǵ����
	E_SHELL_ERROR_DELPOOL,              //delpool �Ǵ����
	E_SHELL_ERROR_SETPOOL,              //setpool�Ǵ����
	E_SHELL_ERROR_SETCELL               //set_cell�Ǵ����
}E_SHELL_ERROR;

typedef enum
{
	E_GET_SET_ERROR_PARAM_FEW = 1,          //����̫��
	E_GET_SET_ERROR_PARAM_MANY,             //����̫��
	E_GET_SET_ERROR_EMPTY,                  //�����
	E_GET_SET_ERROR_START_ID,               //��ʼid������
	//E_GET_ERROR_START_OUTRAGE,          //������ʼλ�÷�Χ
	E_GET_SET_ERROR_END_ID,                 //����id������
	//E_GET_ERROR_END_OUTRAGE,            //��������λ�÷�Χ
	E_GET_SET_ERROR_NO_LESS,                //��ʼid����С�ڽ���id
	E_GET_SET_ERROR_ALL                     //���������Ϣ������get[] 0 0
}E_GET_SET_ERROR;

typedef enum
{
	E_DEL_ERROR_PARAM_FEW = 1,          //����̫��
	E_DEL_ERROR_PARAM_MANY,             //����̫��
	E_DEL_ERROR_EMPTY,                  //�����
	E_DEl_ERROR_ID,                     //ID������
	E_DEL_ERROR_WB_INDEX,               //��ǰworkbase���±꣬�Ѿ���������󳤶�
	E_DEL_ERROR_ALGO,                   //�޴��㷨
	E_DEL_ERROR_CONNCELL_INDEX,         //Cell�±�
	E_DEL_ERROR_DB_OP,                  //���ݿ����ʧ��
	E_DEL_ERROR_DB_CONNECT              //���ݿ����ʧ��
	//E_DEL_ERROR_DEL_MINER_FLAG          //ɾ��Miner��־λ���ִ���
}E_DEL_ERROR;

typedef enum
{
	E_POOLCNF_ERROR_POOLID = 1,          //pool id����
	E_POOLCNF_ERROR_POOLID_NOZERO,       //pool_id����Ϊ0
	E_POOLCNF_ERROR_ALGO,    		     //algo is error.
	E_POOLCNF_ERROR_POOLID_EXIST,        //pool id������
	E_POOLCNF_ERROR_DUAL,                //dual��ʱ��֧�֡�
	E_POOLCNF_ERROR_SEC_STAGE,           //�ڶ�����������
	E_POOLCNF_ERROR_TOO_MANYPOOLID,      //�㷨�����pool_id̫��
	E_POOLCNF_ERROR_TOO_FEWPOOLID        //�㷨�����pool_id̫��
	//E_POOLCNF_ERROR_EMPTY,               //�����
	//E_POOLCNF_ERROR_ID_OVER_LIST,        //id��������
	//E_POOLCNF_ERROR_ID                   //id������
}E_POOLCNF_ERROR;

typedef enum
{
	E_SET_MASS_ALL_SUCC,				  //ȫ���ɹ�
	E_SET_MASS_ALL_INTER,				  //ȫ���ж�
	E_SET_MASS_PART_FAIL,                 //����ͨ��ʧ��
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
    BYTE byUserCmdBuf[BUFFER_LEN_512K]; //���ͻ���
    int iSubindex;                      //�ְ��±�		
    int iNumPack;                       //������
    int iRemain;                        //���������ʣ������
    BOOL bSubcontFlag;                  //�ְ���ʼ��־λ
    int iNumStart;                      //���ܵ���������ʼλ��
    int iNumEnd;                        //���ܵ����������λ��
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

