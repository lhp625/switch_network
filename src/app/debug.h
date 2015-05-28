#ifndef DEBUG_H
#define DEBUG_H

void dbgDumpmem(char * str, char * mem, int len);
#define _DEBUG_
//#define _DEBUG_MANY_DEV_
//#define _DB_DEl_ENABLE_

#ifdef _DEBUG_

#define DEBUGMSG(enable, printfexp)	((void)((enable) ? (printf printfexp), 1 : 0))
#define DEBUGMEM(enable, dmpexp)	((void)((enable) ? (dbgDumpmem dmpexp), 1 : 0))

#else

#define DEBUGMSG(enable, printfexp)
#define DEBUGMEM(enable, dmpexp)

#endif

void timeprintf();

#endif
