#ifndef __API_ALARM_PRO__
#define __API_ALARM_PRO__

#include "../app/typedefs.h"

typedef enum
{
	E_ALARM_ERROR_EMPTY = 1,                  //Á´±í¿Õ
}E_ALARM_ERROR;

void *AlarmProcess(void *arg);

#endif

