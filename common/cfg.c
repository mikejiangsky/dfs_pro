/**
 * @file cfg.c
 * @brief  读取配置文件信息
 * @author Mike
 * @version 1.0
 * @date
 */

 #include "cJSON.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "make_log.h"

#define LOGIN_LOG_MODULE "cgi"
#define LOGIN_LOG_PROC   "cfg"

/* -------------------------------------------*/
/**
 * @brief  从配置文件中得到相对应的参数
 *
 * @param profile   配置文件路径
 * @param title      配置文件title名称[title]
 * @param key       key
 * @param value    (out)  得到的value
 *
 * @returns
 *      0 succ, -1 fail
 */
/* -------------------------------------------*/
int get_pro_value(const char *profile, char *title, char *key, char *value)
{
    //异常处理
    if(profile == NULL || title == NULL || key == NULL || value == NULL)
    {
        return -1;
    }

    int retn = 0;
    //只读方式打开文件
    FILE *fp = fopen(profile, "rb");
    if(fp == NULL) //打开失败
    {
        perror("fopen");
        LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "fopen err\n");
        return -1;
    }

    fseek(fp, 0, SEEK_END);//光标移动到末尾
    long size = ftell(fp); //获取文件大小
    fseek(fp, 0, SEEK_SET);//光标移动到开头


    char *buf = (char *)calloc(1, size); //动态分配空间
    if(buf == NULL)
    {
        perror("calloc");
        LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "calloc err\n");
        fclose(fp); //关闭文件
        return -1;
    }

    //读取文件内容
    fread(buf, 1, size, fp);

    //解析一个json字符串为cJSON对象
    cJSON * root = cJSON_Parse(buf);
    if(NULL == root)
    {
        LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "root err\n");
        retn = -1;
        goto END;
    }

    //返回指定字符串对应的json对象
    cJSON * father = cJSON_GetObjectItem(root, title);
    if(NULL == father)
    {
        LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "father err\n");
        retn = -1;
        goto END;
    }

     cJSON * son = cJSON_GetObjectItem(father, key);
    if(NULL == son)
    {
        LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "son err\n");
        retn = -1;
        goto END;
    }

    LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "son->valuestring = %s\n", son->valuestring);
    strcpy(value, son->valuestring); //拷贝内容

    cJSON_Delete(root);//删除json对象

END:
    fclose(fp);
    free(buf);

    return retn;
}
