/***********************************************************************
* $Id$
* Project:	 SWITCH	  
* File:app/debug.c	 
* Description: brief Part of debug api. 
*
* @version:		V1.0.0
* @date:		20th August 2014
*
* Copyright (C) 2014 SFARDS Co., Ltd. All rights reserved.
*-----------------------------------------------------------------
*
* Revise:  
* 		 date		 version author 		description
*  		2014/08/	 1.0.0	???		create debug api
*
*
*
**********************************************************************/
#include <stdio.h>
#include "./debug.h"
#include <time.h>
#include <string.h>

void dbgDumpmem(char *str, char *mem, int len)
{
	int i;

	if(str != NULL)
		printf("%s", str);

	for(i = 0; i < len; i ++)
		printf("%02x  ", mem[i]);

	printf("\r\n");
}


void timeprintf()
{
	time_t now;
	struct tm *tm_p;

	time(&now);
	tm_p = localtime(&now);

	printf("[%d-%02d-%02d %02d:%02d:%02d]\r\n",
	    tm_p->tm_year + 1900,
	   	tm_p->tm_mon + 1,
	    tm_p->tm_mday,
	    tm_p->tm_hour,
	    tm_p->tm_min, 
	    tm_p->tm_sec);
}



