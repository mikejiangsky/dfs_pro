/**
 * @file 	upload_cgi.c
 * @brief   上传文件后台CGI程序
 * @author 	mike
 * @version 1.0
 * @date
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "fcgi_config.h"
#include "fcgi_stdio.h"
#include "make_log.h" //日志头文件
#include "util_cgi.h" //cgi后台通用接口，trim_space(), memstr()
#include "redis_keys.h"
#include "redis_op.h"
#include "deal_mysql.h" //mysql
#include "cfg.h" //配置文件
#include "cJSON.h"
#include "url_code.h" //url转码

//log 模块
#define DATA_LOG_MODULE "cgi"
#define DATA_LOG_PROC   "data"

//mysql 数据库配置信息 用户名， 密码， 数据库名称
static char mysql_user[128] = {0};
static char mysql_pwd[128] = {0};
static char mysql_db[128] = {0};

//redis 服务器ip、端口
static char redis_ip[30] = {0};
static char redis_port[10] = {0};

//读取配置信息
void read_cfg()
{
     //读取mysql数据库配置信息
    get_pro_value(CFG_PATH, "mysql", "user", mysql_user);
    get_pro_value(CFG_PATH, "mysql", "password", mysql_pwd);
    get_pro_value(CFG_PATH, "mysql", "database", mysql_db);
    LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "mysql:[user=%s,pwd=%s,database=%s]", mysql_user, mysql_pwd, mysql_db);

    //读取redis配置信息
    get_pro_value(CFG_PATH, "redis", "ip", redis_ip);
    get_pro_value(CFG_PATH, "redis", "port", redis_port);
    LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "redis:[ip=%s,port=%s]\n", redis_ip, redis_port);
}

/* -------------------------------------------*/
/**
 * @brief  从redis hash表获取文件信息，
 *         如果redis中没有数据，则从mysql中读取，同时给redis备份数据
 *
 * @param redis_conn    redis服务器连接句柄
 * @param key
 * @param field
 * @param value
 * @param mysql_conn    mysql服务器连接句柄
 * @param sql_cmd       sql语句
 *
 * @returns
 *          0 成功, -1 失败
 */
/* -------------------------------------------*/
int get_hash_value(redisContext *redis_conn, char *key, char *field, char *value, MYSQL *mysql_conn, char *sql_cmd)
{
    if(-1 == rop_hash_get(redis_conn, key, field, value) )
    {
        if( -1 == process_result_one(mysql_conn, sql_cmd, value) )//deal_mysql.h
        {
            return -1;
        }

        rop_hash_set(redis_conn, key, field, value);
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "rop_hash_set(%s, %s, %s)\n", key, field, value);
    }

    return 0;
}

/* -------------------------------------------*/
/**
 * @brief  从redis获取文件信息，然后组成json包，发送前端
 *         如果redis中没有数据，则从mysql中读取，同时给redis备份数据
 *
 * @param fromId    加载资源的起点
 * @param count     每一次显示资源的个数
 * @param cmd       命令分类，最新文件，共享，下载，分享文件列表
 * @param user      当前登陆用户
 */
/* -------------------------------------------*/
void print_file_list_json(int fromId, int count, char *cmd, char *username)
{

    int i = 0;


    cJSON *root = NULL;
    cJSON *array =NULL;
    char *out;
    char filename[FILE_NAME_LEN] = {0};
    char create_time[FIELD_ID_SIZE] ={0};
    char picurl[PIC_URL_LEN] = {0};
    char suffix[8] = {0};
    char pic_name[PIC_NAME_LEN] = {0};
    char file_url[FILE_NAME_LEN] = {0};
    char fileid_list[VALUES_ID_SIZE]={0};
    char user[USER_NAME_LEN] = {0};
    char sql_cmd[SQL_MAX_LEN] = {0};

    int retn = 0;
    int endId = fromId + count - 1;//加载资源的结束位置
    int score = 0;
    char shared_status[2]= {0};


    //RVALUES为redis 表存放批量value字符串数组类型
    //typedef char (*RVALUES)[1024]
    RVALUES fileid_list_values = NULL;
    int value_num;
    redisContext *redis_conn = NULL;

    redis_conn = rop_connectdb_nopwd(redis_ip, redis_port);
    if (redis_conn == NULL)
    {
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "redis connected error");
        return;
    }

    MYSQL *mysql_conn = NULL; //数据库连接句柄

     //连接 mysql 数据库
    mysql_conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (mysql_conn == NULL)
    {
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "msql_conn connect err\n");
        return;
    }

    //设置数据库编码
    mysql_query(mysql_conn, "set names utf8");

    //LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "fromId:%d, count:%d",fromId, count);
    fileid_list_values = (RVALUES)malloc(count*VALUES_ID_SIZE);

    //数据库数据查询规则
    //1、数据先从redis中查询
    //2、如果redis中没有此数据，则再中mysql中查询，同时给redis备份
    //3、如果mysql也没有此数据，说明，数据库没有保存此数据

    if (strcmp(cmd, "shareFile") == 0)
    {//分享文件列表
        sprintf(fileid_list, "%s", FILE_PUBLIC_LIST);
    }
    else if (strcmp(cmd, "newFile") == 0)
    {//请求私有文件列表命令， 最新文件
        //获取用户id
        char user_id[10] = {0};
        //通过数据库查询用户id
        sprintf(sql_cmd, "select u_id from user where u_name=\"%s\"", username);
        if( -1 == get_hash_value(redis_conn, USER_USERID_HASH,  username, user_id, mysql_conn, sql_cmd) )
        {
            goto END;
        }

        sprintf(fileid_list, "%s%s", FILE_USER_LIST, user_id);
    }

    //获取文件列表
    //fileid_list, key值
    //fromId, endId, 范围，起点----终点
    //fileid_list_values保存每个vaule值
    //value_num取出列表元素个数
    retn = rop_range_list(redis_conn, fileid_list, fromId, endId, fileid_list_values, &value_num);
    if (retn < 0)
    {
        //list数据去重
        //得到链表中元素的个数
        if( rop_get_list_cnt(redis_conn, fileid_list) > 0)
        {
            goto END;
        }



        //LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "redis range %s error", FILE_PUBLIC_LIST);
        //查询mysql同一个用户下，所有的file id
        sprintf(sql_cmd, "select file_id from file_info where user=\"%s\"", username);

        if (mysql_query(mysql_conn, sql_cmd) != 0)
        {
            LOG(DATA_LOG_MODULE, DATA_LOG_PROC,"[-]%s error!", sql_cmd);
            goto END;
        }

        MYSQL_RES *res_set;
        res_set = mysql_store_result(mysql_conn);/*生成结果集*/
        if (res_set == NULL)
        {
            LOG(DATA_LOG_MODULE, DATA_LOG_PROC,"[-]%smysql_store_result error!", sql_cmd);
            goto END;
        }

        MYSQL_ROW row;
        uint i;

        if (mysql_errno(mysql_conn) != 0)
        {
            LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "mysql_fetch_row() failed");
            goto END;
        }

        // mysql_fetch_row从使用mysql_store_result得到的结果结构中提取一行，并把它放到一个行结构中。当数据用完或发生错误时返回NULL.
        while ((row = mysql_fetch_row(res_set)) != NULL)
        {

            //mysql_num_fields获取结果中列的个数
            for(i = 0; i < mysql_num_fields(res_set); i++)
            {
                if(row[i] != NULL)
                {
                    rop_list_push(redis_conn, fileid_list, row[i]);
                    //LOG(DATA_LOG_MODULE, DATA_LOG_PROC,"rop_list_push[%s] %s!", fileid_list, row[i]);
                }
            }
        }

        retn = rop_range_list(redis_conn, fileid_list, fromId, endId, fileid_list_values, &value_num);
        if (retn < 0)
        {
            LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "redis range %s error", FILE_PUBLIC_LIST);
            goto END;
        }

    }

    LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "value_num=%d\n", value_num);

    root = cJSON_CreateObject();
    array = cJSON_CreateArray();
    for (i = 0; i < value_num; i++)
    {
        //array[i]:
        cJSON* item = cJSON_CreateObject();

        //id， 文件id
        cJSON_AddStringToObject(item, "id", fileid_list_values[i]);

        //kind, 类型
        cJSON_AddNumberToObject(item, "kind", 2);

        //title_m： filename，文件名字
        sprintf(sql_cmd, "select filename from file_info where file_id=\"%s\"", fileid_list_values[i]);
        if( -1 == get_hash_value(redis_conn, FILEID_NAME_HASH, fileid_list_values[i], filename, mysql_conn, sql_cmd) )
        {
            goto END;
        }
        //LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "filename=%s\n", filename);
        cJSON_AddStringToObject(item, "title_m", filename);

        //title_s(username)， 用户名字
        sprintf(sql_cmd, "select user from file_info where file_id=\"%s\"", fileid_list_values[i]);
        if( -1 == get_hash_value(redis_conn, FILEID_USER_HASH, fileid_list_values[i], user, mysql_conn, sql_cmd) )
        {
            goto END;
        }
        cJSON_AddStringToObject(item, "title_s", user);

        //time， 创建时间
        sprintf(sql_cmd, "select createtime from file_info where file_id=\"%s\"", fileid_list_values[i]);
        if( -1 == get_hash_value(redis_conn, FILEID_TIME_HASH, fileid_list_values[i], create_time, mysql_conn, sql_cmd) )
        {
            goto END;
        }
        cJSON_AddStringToObject(item, "descrip", create_time);
        //LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "create_time=%s\n", create_time);

        //picurl_m 图片后缀
        //读取web_server服务器的ip和端口
        char web_server_ip[30] = {0};
        char web_server_port[10] = {0};
        get_pro_value(CFG_PATH, "web_server", "ip", web_server_ip);
        get_pro_value(CFG_PATH, "web_server", "port", web_server_port);

        memset(picurl, 0, PIC_URL_LEN);
        strcat(picurl, "http://");
        strcat(picurl, web_server_ip);
        strcat(picurl, ":");
        strcat(picurl, web_server_port);
        strcat(picurl, "/static/file_png/");


        get_file_suffix(filename, suffix);
        sprintf(pic_name, "%s.png", suffix);
        strcat(picurl, pic_name);
        cJSON_AddStringToObject(item, "picurl_m", picurl);

        //file url
        sprintf(sql_cmd, "select url from file_info where file_id=\"%s\"", fileid_list_values[i]);
        if( -1 == get_hash_value(redis_conn, FILEID_URL_HASH, fileid_list_values[i], file_url, mysql_conn, sql_cmd) )
        {
            goto END;
        }

        cJSON_AddStringToObject(item, "url", file_url);
        //LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "file_url=%s\n", file_url);

        score = rop_zset_get_score(redis_conn, FILE_HOT_ZSET, fileid_list_values[i]);
        //pv, 文件下载量，由于默认值从1开始，所以，需要减去1
        if(-1 ==  score)
        {
            sprintf(sql_cmd, "select pv from file_info where file_id=\"%s\"", fileid_list_values[i]);
            char tmp[20];
            if( -1 == process_result_one(mysql_conn, sql_cmd, tmp) )//deal_mysql.h
            {
                goto END;
            }

            int j = 0;
            score = atoi(tmp);

            for(j = 0; j < score; j++)
            {
                //将文件插入到FILE_HOT_ZSET中
                rop_zset_increment(redis_conn, FILE_HOT_ZSET, fileid_list_values[i]);
            }
        }
        cJSON_AddNumberToObject(item, "pv", score-1);
        //LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "pv=%d\n", score-1);


        //hot (文件共享状态)
         sprintf(sql_cmd, "select shared_status from file_info where file_id=\"%s\"", fileid_list_values[i]);
        if( -1 == get_hash_value(redis_conn, FILEID_SHARED_STATUS_HASH, fileid_list_values[i], shared_status, mysql_conn, sql_cmd) )
        {
            goto END;
        }
        cJSON_AddNumberToObject(item, "hot", atoi(shared_status));


        cJSON_AddItemToArray(array, item);

    }



    cJSON_AddItemToObject(root, "games", array);

    out = cJSON_Print(root);

    LOG(DATA_LOG_MODULE, DATA_LOG_PROC,"%s", out);
    printf("%s\n", out);

    free(fileid_list_values);
    free(out);

END:
    if(redis_conn != NULL)
    {
        rop_disconnect(redis_conn);
    }

    if (mysql_conn != NULL)
    {
        mysql_close(mysql_conn); //断开数据库连接
    }
}

//设置此文件为已经分享
int move_file_to_public_list(char *file_id)
{
    int retn = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    redisContext *redis_conn = NULL;

    redis_conn = rop_connectdb_nopwd(redis_ip, redis_port);
    if (redis_conn == NULL)
    {
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "redis connected error");
        retn = -1;
        goto END;
    }

    MYSQL *mysql_conn = NULL; //数据库连接句柄

    //连接 mysql 数据库
    mysql_conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (mysql_conn == NULL)
    {
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "msql_conn connect err\n");
        retn = -1;
        goto END;
    }

    //设置数据库编码
    mysql_query(mysql_conn, "set names utf8");

    // 1 将此file id 添加到 publish list 中
    rop_list_push(redis_conn, FILE_PUBLIC_LIST, file_id);


    // 2 文件引用计数加1
    char count[10];

    //获取文件引用计数count
    sprintf(sql_cmd, "select count from file_info where file_id='%s'", file_id);
    if( -1 == get_hash_value(redis_conn, FILE_REFERENCE_COUNT_HASH, file_id, count, mysql_conn, sql_cmd) )
    {
        retn = -1;
        goto END;
    }
    LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "count = %s\n", count);

    //更新mysql数据
    sprintf(sql_cmd, "update file_info set count = %d where file_id='%s'", atoi(count)+1, file_id);

    if (mysql_query(mysql_conn, sql_cmd) != 0) //执行sql语句
    {
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "执行sql语句失败\n");
        retn = -1;
        goto END;
    }

    //更新hash表
    rop_hincrement_one_field(redis_conn,FILE_REFERENCE_COUNT_HASH , file_id, 1);

    // 3 文件分享值设置为 分享状态 1

     //更新mysql数据
    sprintf(sql_cmd, "update file_info set shared_status = %d where file_id='%s'", 1, file_id);

    if (mysql_query(mysql_conn, sql_cmd) != 0) //执行sql语句
    {
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "执行sql语句失败\n");
        retn = -1;
        goto END;
    }

    //更新hash表
    rop_hash_set(redis_conn, FILEID_SHARED_STATUS_HASH, file_id, "1");


END:
    if(redis_conn != NULL)
    {
        rop_disconnect(redis_conn);
    }

    if (mysql_conn != NULL)
    {
        mysql_close(mysql_conn); //断开数据库连接
    }

    return retn;
}

//某个文件下载标志位加1
int increase_file_pv(char *file_id)
{
    int retn = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    redisContext *redis_conn = NULL;

    redis_conn = rop_connectdb_nopwd(redis_ip, redis_port);
    if (redis_conn == NULL)
    {
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "redis connected error");
        retn = -1;
        goto END;
    }

    MYSQL *mysql_conn = NULL; //数据库连接句柄

    //连接 mysql 数据库
    mysql_conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (mysql_conn == NULL)
    {
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "msql_conn connect err\n");
        retn = -1;
        goto END;
    }

    //设置数据库编码
    mysql_query(mysql_conn, "set names utf8");




    int score = rop_zset_get_score(redis_conn, FILE_HOT_ZSET, file_id);
    //pv, 如果缓冲没有数据，需要从mysql中读取
    if(-1 ==  score)
    {
        sprintf(sql_cmd, "select pv from file_info where file_id=\"%s\"", file_id);
        char tmp[20];
        if( -1 == process_result_one(mysql_conn, sql_cmd, tmp) )//deal_mysql.h
        {
            retn = -1;
            goto END;
        }

        int j = 0;
        score = atoi(tmp);

        for(j = 0; j < score; j++)
        {
            //将文件插入到FILE_HOT_ZSET中
            rop_zset_increment(redis_conn, FILE_HOT_ZSET, file_id);
        }
    }

    //更新数据
    sprintf(sql_cmd, "update file_info set pv = %d where file_id='%s'", score+1, file_id);
    if (mysql_query(mysql_conn, sql_cmd) != 0) //执行sql语句
    {
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "执行sql语句失败\n");
        retn = -1;
        goto END;
    }
    rop_zset_increment(redis_conn, FILE_HOT_ZSET, file_id);
    LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "pv = %d\n", score+1);


END:
    if(redis_conn != NULL)
    {
        rop_disconnect(redis_conn);
    }

    if (mysql_conn != NULL)
    {
        mysql_close(mysql_conn); //断开数据库连接
    }

    return retn;
}

int main()
{
    char fromId[5]; //fromId：已经加载的资源个数
    char count[5];  //加载资源的起点
    char cmd[20];   //命令分类，最新文件，共享，下载，分享文件列表
    char user[USER_NAME_LEN];   //用户
    char fileId[FILE_NAME_LEN]; //文件在stroage的路径id

    //读取配置信息
    read_cfg();

    while (FCGI_Accept() >= 0)
    {
        // cgi程序里，printf(), 实际上是给web服务器发送内容，不是打印到屏幕上
        // 但是，下面这句话，不会打印到网页上，这句话的作用，指明CGI给web服务器传输的文本格式为html
        // 如果不指明CGI给web服务器传输的文本格式，后期printf()是不能给web服务器传递信息
        printf("Content-type: text/html\r\n"
                "\r\n");

        memset(fromId, 0, 5);
        memset(count, 0, 5);
        memset(cmd, 0, 20);
        memset(user, 0, USER_NAME_LEN);
        memset(fileId, 0, FILE_NAME_LEN);

         // 获取URL地址 "?" 后面的内容
        char *buf = getenv("QUERY_STRING");
        char query[BURSIZE] = {0};
        strcpy(query, buf);
        urldecode(query); //url解码
        LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "[query_string=%s]\n", query);

        //命令分类，最新文件，共享，下载，分享文件列表
        query_parse_key_value(query, "cmd", cmd, NULL);


        if (strcmp(cmd, "newFile") == 0)
        {   //请求私有文件列表命令， 最新文件
            query_parse_key_value(query, "fromId", fromId, NULL);
            query_parse_key_value(query, "count", count, NULL);
            query_parse_key_value(query, "user", user, NULL);
            LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "=== fromId:%s, count:%s, cmd:%s, user:%s", fromId, count, cmd, user);

             print_file_list_json(atoi(fromId), atoi(count), cmd, user);

        }
        else if (strcmp(cmd, "shareFile") == 0)
        {//请求共享文件列表

        }
        else if (strcmp(cmd, "increase") == 0)
        {//文件被点击下载

            //得到点击的fileId
            query_parse_key_value(query, "fileId", fileId, NULL);
            LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "=== fileId:%s,cmd:%s", fileId,  cmd);

            //str_replace(fileId, "%2F", "/");

            //下载标志位加1
            increase_file_pv(fileId);

        }
        else if (strcmp(cmd, "shared") == 0)
        {//文件被点击分享
            //得到点击的fileId
            query_parse_key_value(query, "fileId", fileId, NULL);
            LOG(DATA_LOG_MODULE, DATA_LOG_PROC, "=== fileId:%s,cmd:%s, user:%s", fileId,  cmd, user);
            //str_replace(fileId, "%2F", "/");

            //设置此文件为已经分享
            move_file_to_public_list(fileId);

        }


    } /* while */

    return 0;
}

