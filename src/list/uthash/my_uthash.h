#ifndef MY_UTHASH_H
#define MY_UTHASH_H

#include "uthash.h"

#ifndef HASH_MALLOC
#define HASH_MALLOC malloc
#endif

typedef struct {
	char id[32];                                                   /* key */
	void *val;
	UT_hash_handle hh;                                             /* makes this structure hashable */
	UT_hash_handle ah;
}devlist_st;

typedef struct {
	int id;
	void *val;
	UT_hash_handle hh;                                             /* makes this structure hashable */
	UT_hash_handle ah;
}devlist_ist;

typedef struct {                                                   /*选择查找相应id的结构体*/
	//int StartId;
	//int EndId;
	char szStartId[32];
	char szEndId[32];
	int SelLen;                                                    /*满足select的数据长度*/
	devlist_st *SelCrrUsers;
}selnode_st;

typedef struct {                                                   /*选择查找相应id的结构体*/
	int StartId;
	int EndId;
	int SelLen;                                                    /*满足select的数据长度*/
	devlist_ist *SelCrrUsers;
}selnode_ist;

#if 0
typedef struct {  
	int id1;                                                       /* usual key */  
	int id2;                                                       /* alternative key */  
	UT_hash_handle hh1;                                            /* handle for first hash table */  
	UT_hash_handle hh2;                                            /* handle for second hash table */  
	UT_hash_handle ah;
}devlistdid_st; 
#endif

typedef struct
{
	pthread_mutex_t *ListLenLock;
	pthread_mutex_t *InitNodeLock;
	pthread_mutex_t *LoginRemLock;
	pthread_mutex_t *CommRemLock;
	pthread_mutex_t *ShellRemLock;
	pthread_mutex_t *AlarmRemLock;
	pthread_mutex_t *DatabaseRemLock;
	pthread_mutex_t *MonitorRemLock;

	devlist_ist **HashUsers;
	//selnode_ist *SelCrrNode;
	selnode_st *SelCrrNode;
	devlist_st **StrHUsers;
	//selnode_st *StrSelCrrNode;

	//int
	void (*AddUserInt)(devlist_ist **, int, void **);	
	devlist_ist *(*FindUserInt)(devlist_ist **, int);	
	void (*DeleteUserInt)(devlist_ist **, devlist_ist *);
	void (*DeleteAllInt)(devlist_ist **);
	unsigned (*CountUsersInt)(devlist_ist *);
	void (*PrintUsersInt)(devlist_ist *);
	devlist_ist *(*SelectcondInt)(devlist_ist **);

	//str
	void (*AddUserStr)(devlist_st **, const char *, int, void **);	
	devlist_st *(*FindUserStr)(devlist_st **, char *);	
	void (*DeleteUserStr)(devlist_st **, devlist_st *);
	void (*DeleteAllStr)(devlist_st **);
	unsigned (*CountUsersStr)(devlist_st *);
	void (*PrintUsersStr)(devlist_st *);
	devlist_st *(*SelectcondStr)(devlist_st **);	
}structUthash;

structUthash stMinerUtlist;
structUthash stCellUtlist;
structUthash stCellIdUtlist;
//structUthash stDelCellIdUtlist;

int inter_evenstr(void *userv);
int idcmp_str(void *_a, void *_b);

#endif

