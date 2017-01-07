/**
 * @file reg_cgi.c
 * @brief  注册事件后CGI程序
 * @author mike
 * @version 1.0
 * @date 2017年1月7日
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "fcgi_stdio.h" //FCGI_Accept()
#include "cJSON.h"      //json
#include "deal_mysql.h" //mysql
#include "util_cgi.h"
#include "redis_op.h"
#include "redis_keys.h"
#include "cfg.h" //配置文件

#define REG_LOG_MODULE       "cgi"
#define REG_LOG_PROC         "reg"

//处理数据库查询结构，把用户id保存在u_id
int process_result_get_uid(MYSQL *conn, MYSQL_RES *res_set, char *u_id)
{
    MYSQL_ROW row;
    uint i;
    ulong line = 0;

    if (mysql_errno(conn) != 0)
    {
        LOG(REG_LOG_MODULE, REG_LOG_PROC, "mysql_fetch_row() failed");
        return -1;
    }

    //mysql_num_rows接受由mysql_store_result返回的结果结构集，并返回结构集中的行数
    line = mysql_num_rows(res_set);
    LOG(REG_LOG_MODULE, REG_LOG_PROC, "%lu rows returned \n", line);
    if (line == 0)
    {
        return -1;
    }

    // mysql_fetch_row从结果结构中提取一行，并把它放到一个行结构中。当数据用完或发生错误时返回NULL.
    while ((row = mysql_fetch_row(res_set)) != NULL)
    {
        //mysql_num_fields获取结果中列的个数
        for (i = 0; i<mysql_num_fields(res_set); i++)
        {
            if (row[i] != NULL)
            {
                LOG(REG_LOG_MODULE, REG_LOG_PROC, "%d row is %s", i, row[i]);
                strcpy(u_id, row[i]);
                return 0;
            }
        }
    }

    return -1;
}

int main()
{
    char user[128];
    char flower_name[128];
    char pwd[128];
    char tel[128];
    char email[128];
    char *out;
    char sql_cmd[SQL_MAX_LEN] = {0};
    char user_id[10] = {0};

    //mysql 数据库配置信息 用户名， 密码， 数据库名称
    char mysql_user[128] = {0};
    char mysql_pwd[128] = {0};
    char mysql_db[128] = {0};

    //读取mysql数据库配置信息
    get_pro_value(CFG_PATH, "mysql", "user", mysql_user);
    get_pro_value(CFG_PATH, "mysql", "password", mysql_pwd);
    get_pro_value(CFG_PATH, "mysql", "database", mysql_db);
    LOG(REG_LOG_MODULE, REG_LOG_PROC, "mysql:[user=%s,pwd=%s,database=%s]", mysql_user, mysql_pwd, mysql_db);

    //redis 服务器ip、端口
    char redis_ip[30] = {0};
    char redis_port[10] = {0};
    //读取redis配置信息
    get_pro_value(CFG_PATH, "redis", "ip", redis_ip);
    get_pro_value(CFG_PATH, "redis", "port", redis_port);
    LOG(REG_LOG_MODULE, REG_LOG_PROC, "redis:[ip=%s,port=%s]\n", redis_ip, redis_port);


    while(FCGI_Accept() >= 0)
    {
        // cgi程序里，printf(), 实际上是给web服务器发送内容，不是打印到屏幕上
        // 但是，下面这句话，不会打印到网页上，这句话的作用，指明CGI给web服务器传输的文本格式为html
        // 如果不指明CGI给web服务器传输的文本格式，后期printf()是不能给web服务器传递信息
        printf("Content-type: text/html\r\n");
        printf("\r\n");

         // 获取URL地址 "?" 后面的内容
        char *query_string = getenv("QUERY_STRING");

        //解析url query 类似 abc=123&bbb=456 字符串
        query_parse_key_value(query_string, "user", user, NULL);
        query_parse_key_value(query_string, "flower_name", flower_name, NULL);
        query_parse_key_value(query_string, "pwd", pwd, NULL);
        query_parse_key_value(query_string, "tel", tel, NULL);
        query_parse_key_value(query_string, "email", email, NULL);

        //入mysql库
        MYSQL *mysql_conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
        if (mysql_conn == NULL)
        {
            LOG(REG_LOG_MODULE, REG_LOG_PROC, "msql_conn connect err\n");
            continue;
        }

        //当前时间戳
        struct timeval tv;
        struct tm* ptm;
        char time_str[128];

        //使用函数gettimeofday()函数来得到时间。它的精度可以达到微妙
        gettimeofday(&tv, NULL);
        ptm = localtime(&tv.tv_sec);//把从1970-1-1零点零分到当前时间系统所偏移的秒数时间转换为本地时间
        //strftime() 函数根据区域设置格式化本地时间/日期，函数的功能将时间格式化，或者说格式化一个时间字符串
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", ptm);

        //sql 语句
        sprintf(sql_cmd, "insert into %s (u_name, nicheng, password, phone, createtime, email) values ('%s', '%s', '%s', '%s', '%s', '%s')",
            "user", user, flower_name, pwd, tel, time_str ,email);

        if (mysql_query(mysql_conn, sql_cmd) != 0)
        {
            print_error(mysql_conn, "插入失败");
            LOG(REG_LOG_MODULE, REG_LOG_PROC, "插入失败\n");
        }

        //查询u_id 用户ID
        sprintf(sql_cmd, "select u_id from user where u_name=\"%s\"", user);
        if (mysql_query(mysql_conn, sql_cmd) != 0)
        {
            LOG(REG_LOG_MODULE, REG_LOG_PROC,"[-]%s error!", sql_cmd);
            continue;
        }
        else
        {
            MYSQL_RES *res_set;
            res_set = mysql_store_result(mysql_conn);/*生成结果集*/
            if (res_set == NULL)
            {
                LOG(REG_LOG_MODULE, REG_LOG_PROC,"[-]%smysql_store_result error!", sql_cmd);
                continue;
            }

            process_result_get_uid(mysql_conn, res_set, user_id);
            LOG(REG_LOG_MODULE, REG_LOG_PROC, "[+]get u_id succ = %s\n", user_id);
        }

        if (mysql_conn != NULL)
        {
            mysql_close(mysql_conn); //断开数据库连接
        }

        //入redis库
        redisContext *redis_conn = rop_connectdb_nopwd(redis_ip, redis_port);
        if (redis_conn == NULL)
        {
            continue;
        }


        //将用户ID 和 USERNAME关系表建立
        rop_hash_set(redis_conn, USER_USERID_HASH, user, user_id);
        LOG(REG_LOG_MODULE, REG_LOG_PROC,"rop_hash_set[%s] %s %s!", USER_USERID_HASH, user, user_id);

        if (redis_conn != NULL)
        {
            rop_disconnect(redis_conn);
        }

        //给前端反馈的信息
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "code", "000"); // {"code":"000"}
        out = cJSON_Print(root); //cJSON to string(char *)

        printf(out);//数据不是打印到终端，而是给web服务器
        free(out);
    }

    return 0;
}