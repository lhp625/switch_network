/***********************************************************************
* $Id$
* Project:	 switch_network  
* File:list/uthash/my_uthash.c		 
* Description: brief Part of hashlist. 
*
* @version:		V1.0.0
* @date:		1th December 2014
*
* Copyright (C) 2014 SFARDS Co., Ltd. All rights reserved.
*-----------------------------------------------------------------
*
* Revise:  
* 		 date		 version 		author 		description
*  		2014/12/01	 1.0.0		zhengchao	create hashlist
*
*
*
**********************************************************************/
#include <stdio.h>                                                 /* gets */
#include <stdlib.h>                                                /* atoi, malloc */
#include <string.h>                                                /* strcpy */
#include <pthread.h>
#include "my_uthash.h"
#include "../../app/debug.h"

devlist_ist *gpMinerUsers = NULL;//, *gpCellUsers = NULL;
devlist_st *gpCellUsers = NULL;
devlist_ist *gpDelCellIds = NULL;
devlist_st *gpCellIds = NULL;
int giStartId = 0, giEndId = 0;
selnode_ist gSelCrriNode = {0};
selnode_st gSelCrrNode = {"0"};




pthread_mutex_t gMinerUtListLenLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCellUtListLenLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCellIdUtListLenLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gDelCellIdUtListLenLock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t gMinerInitNodeLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCellInitNodeLock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t gMinerLoginRemLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gMinerCommRemLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gMinerShellRemLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gMinerAlarmRemLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gMinerDatabaseRemLock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t gCellLoginRemLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCellCommRemLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCellShellRemLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCellAlarmRemLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCellDatabaseRemLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCellMonitorRemLock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t gCellIdLoginRemLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gCellIdShellRemLock = PTHREAD_MUTEX_INITIALIZER;

void add_user_str(devlist_st **users, const char *user_id, int id_len, void **val) {
	devlist_st *Node;

	DEBUGMSG(0,("%s...\n", __FUNCTION__));

	Node = (devlist_st*)malloc(sizeof(devlist_st));
	strncpy(Node->id, user_id, id_len);
	Node->val = *val;
		
	HASH_ADD_STR(*users, id, Node);                                /* id: name of key field */
}

void add_user_int(devlist_ist **users, int user_id, void **val) {
	devlist_ist *Node;

	DEBUGMSG(0,("%s...\n", __FUNCTION__));

	Node = (devlist_ist*)malloc(sizeof(devlist_ist));
	Node->id = user_id;
	Node->val = *val;
		
	HASH_ADD_INT(*users, id, Node);                                /* id: name of key field */
}

devlist_st *find_user_str(devlist_st **users, char *user_id) {
	devlist_st *Node;

	DEBUGMSG(0,("%s...\n", __FUNCTION__));

	HASH_FIND_STR(*users, user_id, Node);                          /* s: output pointer */
	return Node;
}

devlist_ist *find_user_int(devlist_ist **users, int user_id) {
	devlist_ist *Node;

	DEBUGMSG(0,("%s..., user_id:%d\n", __FUNCTION__, user_id));

	HASH_FIND_INT(*users, &user_id, Node);                          /* s: output pointer */

	DEBUGMSG(0,("@Node:%p\n", Node));
	
	return Node;
}

void delete_user_str(devlist_st **users, devlist_st *user) {
	HASH_DEL(*users, user);                                        /* user: pointer to deletee */
	free(user);
}

void delete_user_int(devlist_ist **users, devlist_ist *user) {
	HASH_DEL(*users, user);                                        /* user: pointer to deletee */
	free(user);
}

void delete_all_str(devlist_st **users) {
	devlist_st *current_user, *tmp;

	HASH_ITER(hh, *users, current_user, tmp) {
		HASH_DEL(*users,current_user);                             /* delete it (users advances to next) */
		free(current_user);                                        /* free it */
	}
}

void delete_all_int(devlist_ist **users) {
	devlist_ist *current_user, *tmp;

	HASH_ITER(hh, *users, current_user, tmp) {
		HASH_DEL(*users,current_user);                             /* delete it (users advances to next) */
		free(current_user);                                        /* free it */
	}
}

unsigned count_users_str(devlist_st *users){
	return HASH_COUNT(users);
}

unsigned count_users_int(devlist_ist *users){
	return HASH_COUNT(users);
}

devlist_st *find_all_str(devlist_st **users){
	devlist_st *current_user, *tmp;	
	HASH_ITER(hh, *users, current_user, tmp);

	return current_user;
}

devlist_ist *find_all_int(devlist_ist **users){
	devlist_ist *current_user, *tmp;	
	HASH_ITER(hh, *users, current_user, tmp);

	return current_user;
}

void print_users_str(devlist_st *users) {
	devlist_st *s;

	printf("%s........, @users:%p\n", __FUNCTION__, users);
	for(s = users; s != NULL; s = (devlist_st*)(s->hh.next)) {
		printf("id : %s, val: %s\n", s->id, (char *)s->val);
	}
}

void print_users_int(devlist_ist *users) {
	devlist_ist *s;

	printf("%s........, @users:%p\n", __FUNCTION__, users);
	for(s = users; s != NULL; s = (devlist_ist*)(s->hh.next)) {
		printf("id : %d, val: %s\n", s->id, (char *)s->val);
	}
}

int inter_evenstr(void *userv) {
	//int idTemp = 0, iStartPos = 0, iEndPos = 0;
	char szdTemp[32] = {0}, szStartPos[32] = {0}, szEndPos[32] = {0};

	devlist_st *user = (devlist_st*)userv;
	//idTemp = atoi(user->id);
	strncpy(szdTemp, user->id, sizeof(szdTemp));
	//iStartPos = gSelCrrNode.StartId;
	strncpy(szStartPos, gSelCrrNode.szStartId, sizeof(szStartPos));
	//iEndPos = gSelCrrNode.EndId;
	strncpy(szEndPos, gSelCrrNode.szEndId, sizeof(szEndPos));
	//giStartId = giEndId = 0;

	//DEBUGMSG(0,("iStartPos:%d, iEndPos:%d\n", iStartPos, iEndPos));
	DEBUGMSG(1,("--------szStartPos:%s, szEndPos:%s\n", szStartPos, szEndPos));
	//return (((idTemp >= iStartPos) && (idTemp <= iEndPos)) ? 1:0);
	return (((strncmp(szdTemp, szStartPos, sizeof(szdTemp)) >= 0) && 
		(strncmp(szdTemp, szEndPos, sizeof(szdTemp)) <= 0)) ? 1:0);
}

int inter_evenint(void *userv) {
	int idTemp = 0, iStartPos = 0, iEndPos = 0;

	devlist_ist *user = (devlist_ist*)userv;
	idTemp = user->id;
	iStartPos = gSelCrriNode.StartId;
	iEndPos = gSelCrriNode.EndId;
	//giStartId = giEndId = 0;

	DEBUGMSG(0,("iStartPos:%d, iEndPos:%d\n", iStartPos, iEndPos));
	return (((idTemp >= iStartPos) && (idTemp <= iEndPos)) ? 1:0);
}

int idcmp_str(void *_a, void *_b) {
  devlist_st *a = (devlist_st*)_a;
  devlist_st *b = (devlist_st*)_b;
  
  return strcmp(a->id, b->id);
}

int idcmp_int(void *_a, void *_b) {
  devlist_ist *a = (devlist_ist*)_a;
  devlist_ist *b = (devlist_ist*)_b;
  
  return (a->id - b->id);
}

devlist_st *selectcond_str(devlist_st **users) {
	devlist_st *ausers = NULL;
	
	HASH_SELECT(ah, ausers, hh, *users, inter_evenstr);
	HASH_SRT(ah, ausers, idcmp_str);

	return ausers;
}

devlist_ist *selectcond_int(devlist_ist **users) {
	devlist_ist *ausers = NULL;
	
	HASH_SELECT(ah, ausers, hh, *users, inter_evenint);
	HASH_SRT(ah, ausers, idcmp_int);

	return ausers;
}

structUthash stMinerUtlist = {
	.ListLenLock = &gMinerUtListLenLock,
	.InitNodeLock = &gMinerInitNodeLock,
	.LoginRemLock = &gMinerLoginRemLock,
	.CommRemLock = &gMinerCommRemLock,
	.ShellRemLock = &gMinerShellRemLock,
	.AlarmRemLock = &gMinerAlarmRemLock,
	.DatabaseRemLock = &gMinerDatabaseRemLock,
	
	.HashUsers = &gpMinerUsers,
	.SelCrrNode = &gSelCrrNode,
	.AddUserInt = add_user_int,
	.FindUserInt = find_user_int,
	.DeleteUserInt = delete_user_int,
	.DeleteAllInt = delete_all_int,
	.CountUsersInt = count_users_int,
	.PrintUsersInt = print_users_int,
	.SelectcondInt = selectcond_int
};

structUthash stCellUtlist = {
	.ListLenLock = &gCellUtListLenLock,
	.InitNodeLock = &gCellInitNodeLock,	
	.LoginRemLock = &gCellLoginRemLock,
	.CommRemLock = &gCellCommRemLock,
	.ShellRemLock = &gCellShellRemLock,
	.AlarmRemLock = &gCellAlarmRemLock,
	.DatabaseRemLock = &gCellDatabaseRemLock,
	.MonitorRemLock = &gCellMonitorRemLock,
	
	//.HashUsers = &gpCellUsers,
	.StrHUsers = &gpCellUsers,
	.SelCrrNode = &gSelCrrNode,
	.AddUserInt = add_user_int,
	.FindUserInt = find_user_int,
	.DeleteUserInt = delete_user_int,
	.DeleteAllInt = delete_all_int,
	.CountUsersInt = count_users_int,
	.PrintUsersInt = print_users_int,
	.SelectcondInt = selectcond_int,

	.AddUserStr = add_user_str,
	.FindUserStr = find_user_str,
	.DeleteUserStr = delete_user_str,
	.DeleteAllStr = delete_all_str,
	.CountUsersStr = count_users_str,
	.PrintUsersStr = print_users_str,
	.SelectcondStr = selectcond_str
};

structUthash stCellIdUtlist = {
	.ListLenLock = &gCellIdUtListLenLock,
	.LoginRemLock = &gCellIdLoginRemLock,
	.ShellRemLock = &gCellIdShellRemLock,
	
	.StrHUsers = &gpCellIds,	
	.AddUserStr = add_user_str,
	.FindUserStr = find_user_str,
	.DeleteUserStr = delete_user_str,
	.DeleteAllStr = delete_all_str,
	.CountUsersStr = count_users_str,
	.PrintUsersStr = print_users_str,
	.SelectcondStr = selectcond_str
};

/*structUthash stDelCellIdUtlist = {
	.ListLenLock = &gDelCellIdUtListLenLock,
	
	.HashUsers = &gpDelCellIds,	
	.AddUserInt = add_user_int,
	.FindUserInt = find_user_int,
	.DeleteUserInt = delete_user_int,
	.DeleteAllInt = delete_all_int,
	.CountUsersInt = count_users_int,
	.PrintUsersInt = print_users_int,
	.SelectcondInt = selectcond_int
};*/


