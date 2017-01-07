#ifndef _DEAL_MYSQL_H_
#define _DEAL_MYSQL_H_

#include <mysql/mysql.h> //数据库

#define MYSQL_USER          "root"
#define MYSQL_PWD           "123456"
#define MYSQL_DATABASE      "dfs"
#define SQL_MAX_LEN         (512)   //sql语句长度


/* -------------------------------------------*/
/**
 * @brief  打印操作数据库出错时的错误信息
 *
 * @param conn       (in)    连接数据库的句柄
 * @param title      (int)   用户错误提示信息
 *
 */
/* -------------------------------------------*/
void print_error(MYSQL *conn, const char *title);

/* -------------------------------------------*/
/**
 * @brief  连接数据库
 *
 * @param user_name	 (in)   数据库用户
 * @param passwd     (in)   数据库密码
 * @param db_name    (in)   数据库名称
 *
 * @returns   
 *          成功：连接数据库的句柄
 *			失败：NULL
 */
/* -------------------------------------------*/
MYSQL* msql_conn(char *user_name, char* passwd, char *db_name);


/* -------------------------------------------*/
/**
 * @brief  处理数据库查询结构
 *
 * @param conn	     (in)   连接数据库的句柄
 * @param res_set    (in)   数据库查询后的结果集
 *
 */
/* -------------------------------------------*/
void process_result_set(MYSQL *conn, MYSQL_RES *res_set);

#endif