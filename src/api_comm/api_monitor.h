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
	BYTE byReCmdBuf[BUFFER_LEN_256];         //���ܻ���
} stClientUserData;

typedef struct
{
    BYTE bySeCmdBuf[BUFFER_LEN_512K];        //���ͻ���
    int iSubindex;                          //�ְ��±�		
    int iNumPack;                           //������
    int iRemain;                            //���������ʣ������
    BOOL bSubcontFlag;                      //�ְ���ʼ��־λ
    int iNumStart;                          //���ܵ���������ʼλ��
    int iNumEnd;                            //���ܵ����������λ��    
    devlist_st *pNextNode;                  //�ְ�����һ��������Ϣ
    //int iNewCellSendLen;                    //Cell���ͳ���
}stClientSendData;

typedef enum
{
	E_SET_MONITOR_ALL_SUCC,				     //ȫ���ɹ�
	E_SET_MONITOR_ALL_INTER,				 //ȫ���ж�
	E_SET_MONITOR_PART_FAIL,                 //����ͨ��ʧ��
	E_SET_MONITOR_CELLID_NOEXIST             //cell_id������
}E_SET_MONITOR;

typedef struct
{
	UINT *pIdSeq;                            //����id���к�
	LPSTR pBuf;                              //��������ݻ�����
	structUthash *pList;                     //����
	stMonitor *pMonitorComm;                 //����ָ��
	stSwitchDbInfo *pSwiDbInfo;              //switch����Ϣ
	UINT *pSendInterval;                     //����ʱ������ÿ30�뷢��switch.validator��ÿ60�뷢��cell.info
	int iCommLen;                            //command ���� 
	CHAR szParseDest[BUFFER_LEN_20K];        //��Ϊ������command������Monitor��㣬����������ݻش�������BUF
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

