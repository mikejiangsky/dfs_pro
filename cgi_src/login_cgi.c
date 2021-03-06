/**
 * @file login_cgi.c
 * @brief   登陆后台CGI程序  
 * @author Mike
 * @version 1.0
 * @date 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fcgi_stdio.h"
#include "fcgi_config.h"
#include "util_cgi.h"//query_parse_key_value()
#include "cJSON.h"
#include "make_log.h" //日志头文件
#include "deal_mysql.h"	//数据库, msql_conn(), MYSQL_USER, MYSQL_PWD, MYSQL_DATABASE
#include "cfg.h" //读取配置信息
#include "url_code.h" //url编码

#define LOGIN_LOG_MODULE "cgi"
#define LOGIN_LOG_PROC   "login"

//mysql 数据库配置信息 用户名， 密码， 数据库名称
static char mysql_user[128] = {0};
static char mysql_pwd[128] = {0};
static char mysql_db[128] = {0};

//返回前端登陆情况， 000代表成功， 001代表失败
int return_login_status(char *status_num)
{
    char *out;
    cJSON *root = cJSON_CreateObject();  //创建json项目
    cJSON_AddStringToObject(root, "code", status_num);// {"code":"000"}
    out = cJSON_Print(root);//cJSON to string(char *)

    printf(out);//数据不是打印到终端，而是给web服务器
    free(out);
    return 0;
}

/* -------------------------------------------*/
/**
 * @brief  判断用户登陆情况
 *
 * @param username 		用户名
 * @param pwd 			密码
 *
 * @returns
 *      成功: 0
 *      失败：-1
 */
 /* -------------------------------------------*/
int check_username(char *username, char *pwd)
{
    char sql_cmd[SQL_MAX_LEN] = {0};
    int retn = 0;

    //connect the database
    //MYSQL *conn = msql_conn(MYSQL_USER, MYSQL_PWD, MYSQL_DATABASE);
    MYSQL *conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (conn == NULL)
    {
        return -1;
    }

    //设置数据库编码
    mysql_query(conn, "set names utf8");

    //sql
    sprintf(sql_cmd, "select password from user where u_name=\"%s\"", username);


    //deal result
    char tmp[PWD_LEN];
    process_result_one(conn, sql_cmd, tmp);
    if(strcmp(tmp, pwd) == 0)
    {
        retn = 0;
    }
    else
    {
        retn = -1;
    }

    mysql_close(conn);

    return retn;
}


int main(int argc, char *argv[])
{
    char login_type[10] ={0};			//状态
    char username[USER_NAME_LEN] = {0};	//用户名
    char password[PWD_LEN] = {0};		//密码
    int retn = 0;

     //读取mysql数据库配置信息
     get_pro_value(CFG_PATH, "mysql", "user", mysql_user);
     get_pro_value(CFG_PATH, "mysql", "password", mysql_pwd);
     get_pro_value(CFG_PATH, "mysql", "database", mysql_db);
     LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "mysql:[user=%s,pwd=%s,database=%s]", mysql_user, mysql_pwd, mysql_db);

    while (FCGI_Accept() >= 0)
	{
		// cgi程序里，printf(), 实际上是给web服务器发送内容，不是打印到屏幕上
		// 但是，下面这句话，不会打印到网页上，这句话的作用，指明CGI给web服务器传输的文本格式为html
		// 如果不指明CGI给web服务器传输的文本格式，后期printf()是不能给web服务器传递信息
        printf("Content-type: text/html\r\n");
        printf("\r\n");
		
		//清空
        memset(username, 0, USER_NAME_LEN);
        memset(password, 0, PWD_LEN);
        memset(login_type, 0, 10);

		// 获取URL地址 "?" 后面的内容
        char *buf = getenv("QUERY_STRING");
        char query[BURSIZE] = {0};
        strcpy(query, buf);
        urldecode(query); //url解码

        //解析url query 类似 abc=123&bbb=456 字符串
        query_parse_key_value(query, "user", username, NULL);
        query_parse_key_value(query, "pwd", password, NULL);
        query_parse_key_value(query, "type", login_type, NULL);

        LOG(LOGIN_LOG_MODULE, LOGIN_LOG_PROC, "login:[user=%s,pwd=%s,type=%s]", username, password, login_type);


        //登陆判断，成功返回0，失败返回-1
        retn = check_username(username, password);
        if (retn == 0) //登陆成功
        {
            //返回前端登陆情况， 000代表成功
            return_login_status("000");
        }
        else
        {
            //返回前端登陆情况， 001代表失败
            return_login_status("001");
        }
        

    }

	return 0;
}
