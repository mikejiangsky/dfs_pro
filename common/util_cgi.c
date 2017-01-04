/**
 * @file util_cgi.c
 * @brief  cgi后台通用接口
 * @author 
 * @version 1.0
 * @date 
 */
 
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "make_log.h"

#define UTIL_LOG_MODULE     "cgi"
#define UTIL_LOG_PROC       "util"

/**
 * @brief  去掉一个字符串两边的空白字符
 *
 * @param inbuf确保inbuf可修改
 *
 * @returns   
 *      0 成功
 *      -1 失败
 */
int trim_space(char *inbuf)
{
    int i = 0;
    int j = strlen(inbuf) - 1;

    char *str = inbuf;

    int count = 0;

    if (str == NULL ) 
	{
        //LOG(UTIL_LOG_MODULE, UTIL_LOG_PROC, "inbuf   == NULL\n");
        return -1;
    }


    while (isspace(str[i]) && str[i] != '\0') 
	{
        i++;
    }

    while (isspace(str[j]) && j > i) 
	{
        j--;
    }

    count = j - i + 1;

    strncpy(inbuf, str + i, count);

    inbuf[count] = '\0';

    return 0;
}

/**
 * @brief  在字符串full_data中查找字符串substr第一次出现的位置
 *
 * @param full_data 	源字符串首地址
 * @param full_data_len 源字符串长度
 * @param substr        匹配字符串首地址
 *
 * @returns   
 *      成功: 匹配字符串首地址
 *      失败：NULL
 */
char* memstr(char* full_data, int full_data_len, char* substr) 
{ 
	//异常处理
    if (full_data == NULL || full_data_len <= 0 || substr == NULL) 
	{ 
        return NULL; 
    } 

    if (*substr == '\0')
	{ 
        return NULL; 
    } 

	//匹配子串的长度
    int sublen = strlen(substr); 

    int i; 
    char* cur = full_data; 
    int last_possible = full_data_len - sublen + 1; //减去匹配子串后的长度
    for (i = 0; i < last_possible; i++) 
	{ 
        if (*cur == *substr) 
		{ 
            if (memcmp(cur, substr, sublen) == 0) 
			{ 
                //found  
                return cur; 
            } 
        }
		
        cur++; 
    } 

    return NULL; 
} 