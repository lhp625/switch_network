/***************************************************************************
                          TYPEDEFS.h  -  description
                             -------------------
    begin                : 四 08月 20 2014
    copyright           : (C) 2014 by zhengchao
    email                
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#if	!defined(_TYPEDEFS)
#define	_TYPEDEFS

#include <time.h>

typedef	char				CHAR;
typedef	unsigned char		UCHAR;
typedef	int					INT;
typedef	unsigned int		UINT;
typedef	long				LONG;
typedef	unsigned long		ULONG;
typedef	long long			LLONG;
typedef	unsigned long long	ULLONG;


typedef	UCHAR *				FPTR;

typedef	float				FLOAT;
typedef	double				DOUBLE;
typedef	long double			LDOUBLE;


typedef	char				INT8;		/* Signed 8-bit integer */
typedef	unsigned char		UINT8;		/* Unsigned 8-bit integer */
typedef	short int			INT16;		/* Signed 16-bit integer */
typedef	unsigned short int	UINT16;		/* Unsigned 16-bit integer	*/
typedef	long int			INT32;		/* Signed 32-bit integer */
typedef	unsigned long int	UINT32;		/* Unsigned 32-bit integer */

typedef	float				FLOAT32;	/* 32-bit IEEE single precision */
typedef	double				FLOAT64;	/* 64-bit IEEE double precision */
typedef	long double			FLOAT80;	/* 80-bit IEEE max precision */

typedef	void *				PTR;		/* Pointer to any data type */
typedef	UINT8 *				PTR8;		/* Pointer to 8-bit data */
typedef	UINT16 *			PTR16;		/* Pointer to 16-bit data */
typedef	UINT32 *			PTR32;		/* Pointer to 32-bit data */

typedef	unsigned char		BYTE;		/* 8-bit data */
typedef	unsigned short      WORD;		/* 16-bit data */
typedef	unsigned long 	    DWORD;		/* 32-bit data */

typedef	BYTE *				BYTE_PTR;	/* Pointer to 8-bit data */
typedef	WORD *				WORD_PTR;	/* Pointer to 16-bit data */
typedef	DWORD *				DWORD_PTR;	/* Pointer to 32-bit data */

typedef char *      		LPSTR;
typedef const char * 		LPCSTR;
typedef WORD            	HANDLE;

typedef void        		TASKPROC;

typedef BYTE * 				LPBYTE;
typedef BYTE *          	PBYE;
typedef WORD *      		LPWORD;
typedef WORD *          	PWORD;
typedef DWORD *     		LPDWORD;
typedef DWORD *         	PDWORD;

typedef INT	*				PINT;
typedef char **				LPPSTR;

#ifndef NULL
#define NULL 				((void *)0)
#endif

typedef INT (*IF_PROC_INT_INT)(INT);
typedef INT (*IF_PROC_PTR_INT)(PTR);
typedef INT (*IF_PROC_PTRPTRINT_INT)(PTR, PTR, INT);
typedef INT (*IF_PROC_PTRPTRPTRINT_INT)(PTR, PTR, PTR, INT);
typedef INT (*IF_PROC_PTRPTRPTRINTINT_INT)(PTR, PTR, PTR, INT, INT);
typedef INT (*IF_PROC_PTRPTRPTRINTPINT_INT)(PTR, PTR, PTR, INT, PINT);
typedef INT (*IF_PROC_INTLPPSTRINT_INT)(INT, LPPSTR, INT);
typedef INT (*IF_PROC_INTLPPSTRPTRINT_INT)(INT, LPPSTR, PTR, INT);
typedef INT (*IF_PROC_INTLPPSTRPTRINTPTR_INT)(INT, LPPSTR, PTR, INT, PTR);
typedef INT (*IF_PROC_PTRPTRPTRPTRINT_INT)(PTR, PTR, PTR, LPSTR, INT);
typedef INT (*IF_PROC_PTRPTRINTPINT_INT)(PTR, PTR, INT, PINT);
typedef INT (*IF_PROC_INTLPPSTRPTRINTPTRINT_INT)(INT, LPPSTR, PTR, INT, PTR, INT);

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE  0
#endif

typedef char BOOL;

typedef	union
{
	struct
	{
		UINT16	lower16 ;			/* Lower 16-bits access mode */
		UINT16	upper16 ;			/* Upper 16-bits access mode */
	}s;								/* Union definition */
	UINT32		whole32 ;			/* 32-bit access mode */
} VALUE32, *VALUE32P ;			/* 32-bit address manipulation */

typedef	union
{
	struct
	{
		UINT16	lw ;					/* Lower 16-bits access mode */
		UINT16	uw ;					/* Upper 16-bits access mode */
	} s ;									/* Union definition */
	UINT32		ul ;					/* 32-bit access mode */
} ULS, *ULSP ;							/* 32-bit address manipulation */


#define LOWCHAR(_byte)              ((BYTE)(_byte & 0x0f))
#define HIGHCHAR(_byte)             ((BYTE)((_byte & 0xf0) >> 4))


#define	LOWBYTE(word)               ((word) & 0xff)
#define	HIGHBYTE(word)              lowbyte((word) >> 8)

#define	dim(x)                      (sizeof(x) / sizeof(x[0]))

#define BUFFER_LEN_2                2
#define BUFFER_LEN_4                4
#define BUFFER_LEN_8                8
#define BUFFER_LEN_16               16
#define BUFFER_LEN_20               20
#define BUFFER_LEN_24               24
#define BUFFER_LEN_28               28
#define BUFFER_LEN_32               32
#define BUFFER_LEN_64               64
#define BUFFER_LEN_96               96
#define BUFFER_LEN_100              100
#define BUFFER_LEN_128              128	
#define BUFFER_LEN_256              256	
#define BUFFER_LEN_512              512
#define BUFFER_LEN_576              576
#define BUFFER_LEN_1024             1024
#define BUFFER_LEN_2048             2048
#define BUFFER_LEN_4096             4096
#define BUFFER_LEN_8192             8192
#define BUFFER_LEN_10K              10*1024      //10k
#define BUFFER_LEN_20K              20*1024      //20k
#define BUFFER_LEN_32K              32*1024      //32k
#define BUFFER_LEN_64K              64*1024      //64k
#define BUFFER_LEN_128K             128*1024     //128k
#define BUFFER_LEN_256K             256*1024     //256k
#define BUFFER_LEN_512K             512*1024     //512k

#define MAX_BUF_NUM                 15000//10001        //1000

#define COMMANDLINE_0               0
#define COMMANDLINE_1               1
#define COMMANDLINE_2               2
#define COMMANDLINE_3               3
#define COMMANDLINE_4               4
#define COMMANDLINE_5               5
#define COMMANDLINE_6               6
#define COMMANDLINE_7               7
#define COMMANDLINE_8               8
#define COMMANDLINE_10              10
#define COMMANDLINE_11              11
#define COMMANDLINE_13              13
#define COMMANDLINE_18              18
#define COMMANDLINE_19              19
#define COMMANDLINE_21              21

#define HOUR_24                     24
#define MINUTE_60                   60
#define SECOND_60                   60

#define INTERVAL_30_SECOND          30
#define INTERVAL_60_SECOND          60
#define INTERVAL_180_SECOND         180

#define VALUES_1000                 1000

#define MAC_LEN                     12

#if 0
#if	!defined(min)
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#endif
#endif

typedef enum 
{
	E_ALGO_BTC,
	E_ALGO_LTC,
	E_ALGO_DUAL
}E_ALGO;

typedef struct
{
	BYTE cmd[BUFFER_LEN_100];
	IF_PROC_PTRPTRINT_INT UserCmdFunc;
	//IF_PROC_INTLPPSTRPTRINTPTR_INT CommPackFunc;       //从Monitor 打包数据到command
}stCommand;

typedef struct
{
	BYTE cmd[BUFFER_LEN_100];
	IF_PROC_PTRPTRINT_INT UserCmdFunc;
}stCommandErr;

long glStartTime;

typedef struct
{
	int iDay;
	int iHour;
	int iMinute;
	int iSecond;
}stRunTime;
//stRunTime gRunTime;

typedef struct
{
	BYTE cmd[BUFFER_LEN_100];
	IF_PROC_PTRPTRINTPINT_INT UserCmdFunc;
	IF_PROC_INTLPPSTRPTRINTPTR_INT CommPackFunc;       //从Monitor 打包数据到command
}stMonitor;

typedef struct
{
	BYTE cmd[BUFFER_LEN_100];
	IF_PROC_INTLPPSTRPTRINTPTRINT_INT SetCommFunc;     //解析设置不同类型的参数，如pool_id,cell_id
}stParam;


struct tm *GetLocalTime();
int MSleep(long ms);
int Parse(char *string, int *argc, char**argv, char *delim);
int AlgoEnumToString(E_ALGO eAlgo, char *dest, int destLen);
long GetTime();
int GetLocalTimeString(char *buf, int bufLen, long);
void GetLocalTimeLog();
stRunTime RunTime();
unsigned long ip2long(const char *addr);
char *long2ip(const unsigned long ip);
int get_mac(char *pbufDest);

#endif
