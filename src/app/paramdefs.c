/***********************************************************************
* $Id$
* Project:	 switch_network
* File:		 api_comm/api_command.c		 
* Description: brief Part of control command file. 
*
* @version:		V1.0.0
* @date:		5th September 2014
*
* Copyright (C) 2014 SFARDS Co., Ltd. All rights reserved.
*-----------------------------------------------------------------
*
* Revise:  
* 		 date		 version 		author 		description
*  		2014/09/05	 1.0.0		zhengchao	create control command
*
*
*
**********************************************************************/
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/types.h>  
#include <unistd.h>  
#include <stdlib.h>  
#include "typedefs.h"
#include "debug.h"
#include "../ap_log.h"


//clock_t glStartTime = 0;

//将字符空格转换成'\0' 
int Parse(char *string, int *argc, char**argv, char *delim)
{
	int iDelimTemp = 0;
	int iPos = 0, i = 0, len = 0, rtnValue = 0;

	DEBUGMSG(0, ("%s...\n", __FUNCTION__));
	
	iDelimTemp = *delim;
	len = strlen(string);
	DEBUGMSG(0,("len:%d\n", len));
	while (iPos < len)
	{		
		if (i < 1)
			*(argv+i) = string;
		else
		{
			*(argv+i) = strchr(string+iPos, iDelimTemp);		
			if (*(argv+i) == NULL)
			{
				break;
			}
			
			iPos = *(argv+i) - string;		
			**(argv+i) = '\0';
			(*(argv+i))++;
			iPos++;
		}

		if ((i++) > len)
		{
			rtnValue = 1;
			break;
		}
	}
	*argc = i;
	
	return rtnValue;
}

int MSleep(long ms)
{
    struct timeval tv;
	
    tv.tv_sec = ms/1000;                		/*SECOND*/
    tv.tv_usec = ms%1000 * 1000;  				/*MICROSECOND*/
	
    return select(0, NULL, NULL, NULL, &tv);
}

struct tm *GetLocalTime()
{
	struct tm *ptm;
	time_t now;

	time(&now);
	ptm = localtime(&now);
	
	return ptm;
}

long GetTime()
{
	long seconds = 0 ;
	
	return (seconds = time((time_t*)NULL));
}

int GetLocalTimeString(char *buf, int bufLen, long timeVal)
{
	struct tm *ptm;

	//ptm = GetLocalTime();
	ptm = localtime(&timeVal);
	
	return snprintf(buf, bufLen, "[%d-%02d-%02d %02d:%02d:%02d] ",
					ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
}

void GetLocalTimeLog()
{
	struct tm *ptm;

	ptm = GetLocalTime(); 
	ap_log_debug_log("[%d-%02d-%02d %02d:%02d:%02d]\n",
					ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
}

stRunTime RunTime()
{
	long finish;
	long duration;
	stRunTime TimeTemp;

	memset(&TimeTemp, 0, sizeof(stRunTime));
	finish = GetTime();
	duration = finish - glStartTime;// / CLOCKS_PER_SEC;
	//DEBUGMSG(0,("duration:%f, %d, glStartTime:%f\n", duration, (int)duration, ((float)glStartTime)/ CLOCKS_PER_SEC));

	if (duration >= SECOND_60)
	{
		TimeTemp.iMinute = ((int)duration) / SECOND_60;
		TimeTemp.iSecond = ((int)duration) % SECOND_60;
		DEBUGMSG(1,("iMinute:%d, iSecond:%d\n", TimeTemp.iMinute, TimeTemp.iSecond));
		
		if (TimeTemp.iMinute >=  MINUTE_60)
		{
			TimeTemp.iHour = TimeTemp.iMinute / MINUTE_60;
			TimeTemp.iMinute = TimeTemp.iMinute % MINUTE_60;
			DEBUGMSG(1,("iHour:%d, iMinute:%d\n", TimeTemp.iHour, TimeTemp.iMinute));
			
			if (TimeTemp.iHour > HOUR_24)
			{
				TimeTemp.iDay= TimeTemp.iHour / HOUR_24;
				TimeTemp.iHour = TimeTemp.iHour % HOUR_24;
			}
		}
	}
	else
	{
		TimeTemp.iSecond = ((int)duration);
	}
	DEBUGMSG(1,("D:%d, H:%d, M:%d, S:%d\n", TimeTemp.iDay, TimeTemp.iHour, TimeTemp.iMinute, TimeTemp.iSecond));
	
	return TimeTemp ;
}

unsigned long ip2long(const char *addr)
{
	DEBUGMSG(1,("addr:%s\n", addr));
	return ntohl(inet_addr(addr));
}

char *long2ip(const unsigned long ip)
{
	struct in_addr addr;

	addr.s_addr = htonl(ip);
	return inet_ntoa(addr);
}

int my_itoa(int val, char* buf, const int radix)
{
	char* p;
	int a;                              //every digit
	int len;
	char* b;                            //start of the digit char
	char temp;

	p = buf;
	if (val < 0)
	{
		*p++ = '-';
		val = 0 - val;
	}
	b = p;
	do
	{
		a = val % radix;
		val /= radix;
		*p++ = a + '0';
	} while (val > 0);
	len = (int)(p - buf);
	*p-- = 0;

	//swap
	do
	{
		temp = *p;
		*p = *b;
		*b = temp;
		--p;
		++b;
	} while (b < p);
	
	return len;
}

int get_mac(char *pbufDest)
{  
	FILE *stream;  
	//FILE *wstream;
	char buf[64]= "0";
	int i = 0, j = 0;
	int rtnRead = 0;

	//初始化buf  
	memset(buf, '\0', sizeof(buf) );
	//将"ifconfig | grep HWaddr | awk 'NR==1'| awk '{print $5}'"命令的输出 
	//通过管道读取（“r”参数）到FILE* stream
	stream = popen("ifconfig | grep HWaddr | awk 'NR==1'| awk '{print $5}'", "r");	

	//将刚刚FILE* stream的数据流读取到buf中
	rtnRead = fread(buf, sizeof(char), sizeof(buf),  stream); 
	if(rtnRead == 0)
	{
		perror("read file field.\n");
	}
	else
	{
		//printf("buf:%s", buf);
		
		for (i = 0, j = 0; i < strlen(buf); i++)
		{
			//printf("[%c]", buf[i]);
			if (buf[i] == '\n' || buf[i] == ':')
			{
				continue;
			}
			//printf("%02x ", buf[i]);
			pbufDest[j++] = buf[i];
			//printf("[%d]:%c   =   [%d]:%c\n", j-1, pbufDest[j-1], i, buf[i]);
		}
		//printf("\n");
		//printf("%s\n", pbufDest);
	}
	pclose(stream);  

	return 0;
}


