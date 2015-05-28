#include <stdio.h>                                                                                            
#include "cJSON.h"

//�������飬����ֵ����һ��JSON��item������ʹ��������Ϊ��ʾ
char * makeArray(int iSize)
{
    cJSON * root =  cJSON_CreateArray();                                                             
    if(NULL == root)
    {
        printf("create json array faild\n");
        return NULL;
    }
    int i = 0;
    
    for(i = 0; i < iSize; i++)
    {
        cJSON_AddNumberToObject(root, "hehe", i);
    }
    char * out = cJSON_Print(root);
    cJSON_Delete(root);

    return out;
}

//�����ոյ�CJSON����
void parseArray(char * pJson)
{
	int iCnt = 0;

    if(NULL == pJson)
    {                                                                                                
        return ;
    }
    cJSON * root = NULL;
    if((root = cJSON_Parse(pJson)) == NULL)
    {
        return ;
    }
    int iSize = cJSON_GetArraySize(root);
    for(iCnt = 0; iCnt < iSize; iCnt++)
    {
        cJSON * pSub = cJSON_GetArrayItem(root, iCnt);
        if(NULL == pSub)
        {
            continue;
        }
        int iValue = pSub->valueint;
        printf("value[%2d] : [%d]\n", iCnt, iValue);
    }   
    cJSON_Delete(root);
    return;
}

int main()
{
    char * p = makeArray(10);
    if(NULL == p)
    {
        return 0;
    }
    printf("%s\n", p);
    parseArray(p);                                                                                             

    return 0;
}



