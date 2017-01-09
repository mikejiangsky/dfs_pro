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
#include "url_code.h" //url转码

//log 模块
#define UPLOAD_LOG_MODULE "cgi"
#define UPLOAD_LOG_PROC   "upload"

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
    LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "mysql:[user=%s,pwd=%s,database=%s]", mysql_user, mysql_pwd, mysql_db);

    //读取redis配置信息
    get_pro_value(CFG_PATH, "redis", "ip", redis_ip);
    get_pro_value(CFG_PATH, "redis", "port", redis_port);
    LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "redis:[ip=%s,port=%s]\n", redis_ip, redis_port);
}

/* -------------------------------------------*/
/**
 * @brief  得到上传文件所属用户的用户名
 * @return 0 -succ
 *         -1 fail
 */
/* -------------------------------------------*/
int get_username(char *user)
{
    // 获取URL地址 "?" 后面的内容
    char *buf = getenv("QUERY_STRING");
    char query_string[BURSIZE] = {0};
    strcpy(query_string, buf);
    urldecode(query_string); //url解码
    LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "query_string = [%s]", query_string);


    //得到用户名
    query_parse_key_value(query_string, "user", user, NULL);
    if (strlen(user) == 0)
    {
        LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "get user has no value!!", user);
        return -1;
    }
    else
    {
        LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "get user = [%s]", user);
    }

    return 0;
}

/* -------------------------------------------*/
/**
 * @brief  解析上传的post数据 保存到本地临时路径
 *         同时得到文件名称
 *
 * @param len       (in)    post数据的长度
 * @param file_name (out)   文件的文件名
 *
 * @returns
 *          0 succ, -1 fail
 */
/* -------------------------------------------*/
int recv_save_file(int len, char *filename)
{
    int retn = 0;
    int i, ch;
    char *file_buf = NULL;
    char *begin = NULL;
    char *end = NULL;
    char *p, *q, *k;

    char content_text[TEMP_BUF_MAX_LEN] = {0}; //文件头部信息
    char boundary[TEMP_BUF_MAX_LEN] = {0};	   //分界线信息

    //==========> 开辟存放文件的 内存 <===========
    file_buf = (char *)malloc(len);
    if (file_buf == NULL)
    {
        printf("malloc error! file size is to big!!!!\n");
        return -1;
    }

    begin = file_buf;
    p = begin;
    for (i = 0; i < len; i++)
    {
        if ((ch = getchar()) < 0)
        {
            printf("Error: Not enough bytes received on standard input<p>\n");
            retn = -1;
            goto END;
        }
        //putchar(ch);
        *p = ch;
        p++;
    }

    //===========> 开始处理前端发送过来的post数据格式 <============
    end = p;//内存的尾部

    p = begin;

    //get boundary 得到分界线
    p = strstr(begin, "\r\n");
    if (p == NULL)
    {
        printf("wrong no boundary!\n");
        retn = -1;
        goto END;
    }

    //拷贝分界线
    strncpy(boundary, begin, p-begin);
    boundary[p-begin] = '\0';	//字符串结束符
    //printf("boundary: [%s]\n", boundary);

    p += 2;//\r\n
    //已经处理了p-begin的长度
    len -= (p-begin);

    //get content text head
    begin = p;

    p = strstr(begin, "\r\n");
    if(p == NULL)
    {
        printf("ERROR: get context text error, no filename?\n");
        retn = -1;
        goto END;
    }
    strncpy(content_text, begin, p-begin);
    content_text[p-begin] = '\0';
    //printf("content_text: [%s]\n", content_text);

    p += 2;//\r\n
    len -= (p-begin);

    //get filename
    // filename="123123.png"
    //           ↑
    q = begin;
    q = strstr(begin, "filename=");

    q += strlen("filename=");
    q++;	//跳过第一个"

    k = strchr(q, '"');
    strncpy(filename, q, k-q);	//拷贝文件名
    filename[k-q] = '\0';

    //去掉一个字符串两边的空白字符
    trim_space(filename);	//util_cgi.h
    //printf("filename: [%s]\n", filename);

    //get file
    begin = p;
    p = strstr(begin, "\r\n");
    p += 4;//\r\n\r\n
    len -= (p-begin);

    //下面才是文件的真正内容
    begin = p;
    // now begin -->file's begin
    //find file's end
    p = memstr(begin, len, boundary);//util_cgi.h
    if (p == NULL)
    {
        p = end-2;    //\r\n
    }
    else
    {
        p = p -2;//\r\n
    }

    //begin---> file_len = (p-begin)

    //=====> 此时begin-->p两个指针的区间就是post的文件二进制数据
    //======>将数据写入文件中,其中文件名也是从post数据解析得来  <===========

    int fd = 0;
    fd = open(filename, O_CREAT|O_WRONLY, 0644);
    if (fd < 0)
    {
        printf("open %s error\n", filename);
        retn = -1;
        goto END;
    }

    //ftruncate会将参数fd指定的文件大小改为参数length指定的大小
    ftruncate(fd, (p-begin));
    write(fd, begin, (p-begin));
    close(fd);

    LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "upload [%s] succ!", filename);
    printf("upload [%s] succ!\r\n", filename);

END:
    free(file_buf);
    return retn;
}

/* -------------------------------------------*/
/**
 * @brief  将一个本地文件上传到 后台分布式文件系统中
 *
 * @param filename  (in) 本地文件的路径
 * @param fileid    (out)得到上传之后的文件ID路径
 *
 * @returns
 *      0 succ, -1 fail
 */
/* -------------------------------------------*/
int upload_to_dstorage(char *filename, char *fileid)
{
    int retn = 0;

    pid_t pid;
    int fd[2];

    //无名管道的创建
    if (pipe(fd) < 0)
    {
        printf("pip error\n");
        retn = -1;
        goto END;
    }

    //创建进程
    pid = fork();
    if (pid < 0)//进程创建失败
    {
        printf("fork error\n");
        retn = -1;
        goto END;
    }

    if(pid == 0) //子进程
    {
        //关闭读端
        close(fd[0]);

        //将标准输出 重定向 写管道
        dup2(fd[1], STDOUT_FILENO); //dup2(fd[1], 1);

        //读取fdfs client 配置文件的路径
        char fdfs_cli_conf_path[256] = {0};
        get_pro_value(CFG_PATH, "dfs_path", "client", fdfs_cli_conf_path);

        //通过execlp执行fdfs_upload_file
        execlp("fdfs_upload_file", "fdfs_upload_file", fdfs_cli_conf_path, filename, NULL);

        //执行失败
        LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "execlp fdfs_upload_file error\n");

        close(fd[1]);
    }
    else //父进程
    {
        //关闭写端
        close(fd[1]);

        //从管道中去读数据
        read(fd[0], fileid, TEMP_BUF_MAX_LEN);

        //去掉一个字符串两边的空白字符
        trim_space(fileid);

        if (strlen(fileid) == 0)
        {
            LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC,"[upload FAILED!]\n");
            retn = -1;
            goto END;
        }

        LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "get [%s] succ!\n", fileid);

        wait(NULL);	//等待子进程结束，回收其资源
        close(fd[0]);
    }

END:
    return retn;
}

/* -------------------------------------------*/
/**
 * @brief  封装文件存储在分布式系统中的 完整 url
 *
 * @param fileid        (in)    文件分布式id路径
 * @param fdfs_file_url (out)   文件的完整url地址
 *
 * @returns
 *      0 succ, -1 fail
 */
/* -------------------------------------------*/
int make_file_url(char *fileid, char *fdfs_file_url)
{
    int retn = 0;

    char *p = NULL;
    char *q = NULL;
    char *k = NULL;

    char fdfs_file_stat_buf[TEMP_BUF_MAX_LEN] = {0};
    char fdfs_file_host_name[HOST_NAME_LEN] = {0};	//storage所在服务器ip地址

    pid_t pid;
    int fd[2];

    //无名管道的创建
    if (pipe(fd) < 0)
    {
        printf("pip error\n");
        retn = -1;
        goto END;
    }

    //创建进程
    pid = fork();
    if (pid < 0)//进程创建失败
    {
        printf("fork error\n");
        retn = -1;
        goto END;
    }

    if(pid == 0) //子进程
    {
        //关闭读端
        close(fd[0]);

        //将标准输出 重定向 写管道
        dup2(fd[1], STDOUT_FILENO); //dup2(fd[1], 1);

        //读取fdfs client 配置文件的路径
        char fdfs_cli_conf_path[256] = {0};
        get_pro_value(CFG_PATH, "dfs_path", "client", fdfs_cli_conf_path);

        execlp("fdfs_file_info", "fdfs_file_info", fdfs_cli_conf_path, fileid, NULL);

        //执行失败
        LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "execlp fdfs_file_info error\n");

        close(fd[1]);
    }
    else //父进程
    {
        //关闭写端
        close(fd[1]);

        //从管道中去读数据
        read(fd[0], fdfs_file_stat_buf, TEMP_BUF_MAX_LEN);
        //LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "get file_ip [%s] succ\n", fdfs_file_stat_buf);

        wait(NULL);	//等待子进程结束，回收其资源
        close(fd[0]);

        //拼接上传文件的完整url地址--->http://host_name/group1/M00/00/00/D12313123232312.png
        p = strstr(fdfs_file_stat_buf, "source ip address: ");

        q = p + strlen("source ip address: ");
        k = strstr(q, "\n");

        strncpy(fdfs_file_host_name, q, k-q);
        fdfs_file_host_name[k-q] = '\0';

        //printf("host_name:[%s]\n", fdfs_file_host_name);

        //读取storage_web_server服务器的端口
        char storage_web_server_port[20] = {0};
        get_pro_value(CFG_PATH, "storage_web_server", "port", storage_web_server_port);
        strcat(fdfs_file_url, "http://");
        strcat(fdfs_file_url, fdfs_file_host_name);
        strcat(fdfs_file_url, ":");
        strcat(fdfs_file_url, storage_web_server_port);
        strcat(fdfs_file_url, "/");
        strcat(fdfs_file_url, fileid);

        printf("[%s]\n", fdfs_file_url);
        LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "file url is: %s\n", fdfs_file_url);
    }

END:
    return retn;
}

/* -------------------------------------------*/
/**
 * @brief  将该文件的FastDFS路径、名称、上传者存入mysql中
 *
 * @param fileid
 * @param fdfs_file_url
 * @param filename
 * @param user
 *
 * @returns
 *          0 succ, -1 fail
 */
/* -------------------------------------------*/
int store_fileinfo_to_mysql(char *fileid, char *fdfs_file_url, char *filename, char *user)
{
    int retn = 0;
    MYSQL *mysql_conn = NULL; //数据库连接句柄

    time_t now;;
    char create_time[TIME_STRING_LEN];
    char suffix[SUFFIX_LEN];
    char sql_cmd[SQL_MAX_LEN] = {0};

    //连接 mysql 数据库
    mysql_conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (mysql_conn == NULL)
    {
        LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "msql_conn connect err\n");
        return -1;
    }

    //设置数据库编码
    mysql_query(mysql_conn, "set names utf8");

    //获取当前时间
    now = time(NULL);
    strftime(create_time, TIME_STRING_LEN-1, "%Y-%m-%d %H:%M:%S", localtime(&now));

    //得到文件后缀字符串 如果非法文件后缀,返回"null"
    get_file_suffix(filename, suffix);

    //sql 语句
    /*
    -- file_id 文件id
    -- url 文件url
    -- filename 文件名字
    -- createtime 文件创建时间
    -- user	文件所属用户
    -- type 文件类型： png, zip, mp4……
    -- shared_status 共享状态, 0为没有共享， 1为共享
    -- count 文件引用计数， 默认为1， 共享一次加1
    -- pv 文件下载量，默认值为1，下载一次加1
    */
    sprintf(sql_cmd, "insert into %s (file_id, url, filename, createtime, user, type, shared_status, count, pv) values ('%s', '%s', '%s', '%s', '%s', '%s', %d, %d, %d)",
        "file_info", fileid, fdfs_file_url, filename, create_time, user ,suffix, 0, 1, 1);

    if (mysql_query(mysql_conn, sql_cmd) != 0) //执行sql语句
    {
        print_error(mysql_conn, "插入失败");
        LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "插入失败\n");
    }

    LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "文件信息插入成功\n");

    if (mysql_conn != NULL)
    {
        mysql_close(mysql_conn); //断开数据库连接
    }

    return retn;
}



/* -------------------------------------------*/
/**
 * @brief  将文件信息存储到redis数据库多个hash表形式
 *
 * @param fileid
 * @param fdfs_file_url
 * @param filename
 * @param user
 *
 * @returns
 *          0 succ, -1 fail
 */
/* -------------------------------------------*/
int bk_fileinfo_to_redis(char *fileid, char *fdfs_file_url, char *filename, char *user)
{
    int retn = 0;
    char sql_cmd[SQL_MAX_LEN] = {0};
    char create_time[TIME_STRING_LEN];
    char suffix[SUFFIX_LEN];
    char user_id[10] = {0};
    char file_user_list[KEY_NAME_SIZ] = {0};
    redisContext * redis_conn = NULL;


    MYSQL *mysql_conn = NULL; //数据库连接句柄

     //连接 mysql 数据库
    mysql_conn = msql_conn(mysql_user, mysql_pwd, mysql_db);
    if (mysql_conn == NULL)
    {
        LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "msql_conn connect err\n");
        return -1;
    }

    //设置数据库编码
    mysql_query(mysql_conn, "set names utf8");


    //连接缓存数据库redis
    redis_conn = rop_connectdb_nopwd(redis_ip, redis_port);
    if (redis_conn == NULL)
    {
         LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC,"rop_connectdb_nopwderror!\n");

        return retn;
    }


    //FILEID_URL_HASH
    rop_hash_set(redis_conn, FILEID_URL_HASH, fileid, fdfs_file_url);
    //FILEID_NAME_HASH
    rop_hash_set(redis_conn, FILEID_NAME_HASH, fileid, filename);

    //FILEID_TIME_HASH
    //create time
    //通过mysql数据库获取文件的创建时间
    sprintf(sql_cmd, "select createtime from file_info where file_id=\"%s\"", fileid);
    if( -1 != process_result_one(mysql_conn, sql_cmd, create_time) )//deal_mysql.h
    {
        rop_hash_set(redis_conn, FILEID_TIME_HASH, fileid, create_time);
        LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "[+]get  create_time succ = %s\n", create_time);
    }


    //FILEID_USER_HASH
    rop_hash_set(redis_conn, FILEID_USER_HASH, fileid, user);
    LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "   rop_hash_set(redis_conn, FILEID_USER_HASH, fileid, user) = %s\n", user);

    //FILEID_TYPE_HASH
    get_file_suffix(filename, suffix);//得到文件后缀字符串 如果非法文件后缀,返回"null"
    rop_hash_set(redis_conn, FILEID_TYPE_HASH, fileid, suffix);


    //FILEID_SHARED_STATUS_HASH(文件的共享状态)
    rop_hash_set(redis_conn, FILEID_SHARED_STATUS_HASH, fileid, "0");

    //将文件插入到FILE_HOT_ZSET中, 默认权重为1
    rop_zset_increment(redis_conn, FILE_HOT_ZSET, fileid);

    //获取用户id
    //1、先从redis读取
    //2、如果redis读取失败，说明缓冲中没有数据
    //3、从mysql数据库获取id，如果mysql也没有，说明没有此用户
    //4、如果mysql有此用户id，要把此id放在缓冲中

     if(-1 == rop_hash_get(redis_conn, USER_USERID_HASH,  user, user_id) )
     {
        //通过数据库查询用户id
        sprintf(sql_cmd, "select u_id from user where u_name=\"%s\"", user);
        if(-1 == process_result_one(mysql_conn, sql_cmd, user_id) )//deal_mysql.h
        {
            return -1;
        }
        LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "[+]get u_id succ = %s\n", user_id);

        //如果mysql有此用户id，要把此id放在缓冲中
        //将用户ID 和 USERNAME关系表建立
        rop_hash_set(redis_conn, USER_USERID_HASH, user, user_id);
        LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC,"rop_hash_set[%s] %s %s!", USER_USERID_HASH, user, user_id);

     }


    //将FILEID 插入到 用户私有列表中FILE_USER_LIST
    sprintf(file_user_list, "%s%s", FILE_USER_LIST, user_id);
    rop_list_push(redis_conn, file_user_list, fileid);
    LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC,"rop_list_push[%s] %s!", file_user_list, fileid);

    //文件引用计数+1
    rop_hincrement_one_field(redis_conn,FILE_REFERENCE_COUNT_HASH , fileid, 1);

    rop_disconnect(redis_conn);

    if (mysql_conn != NULL)
    {
        mysql_close(mysql_conn); //断开数据库连接
    }

    return retn;
}


int main()
{
    char user[USER_NAME_LEN] = {0};         //当前登陆用户
    char filename[FILE_NAME_LEN] = {0};		//上传文件的名字
    char fileid[TEMP_BUF_MAX_LEN] = {0};	//文件上传到fastDFS后的文件id
    char fdfs_file_url[FILE_URL_LEN] = {0};	//文件所存放storage的host_name

    //读取配置信息
    read_cfg();

    while (FCGI_Accept() >= 0)
    {
        // cgi程序里，printf(), 实际上是给web服务器发送内容，不是打印到屏幕上
        // 但是，下面这句话，不会打印到网页上，这句话的作用，指明CGI给web服务器传输的文本格式为html
        // 如果不指明CGI给web服务器传输的文本格式，后期printf()是不能给web服务器传递信息
        printf("Content-type: text/html\r\n"
                "\r\n");

        //得到上传文件所属用户的用户名
        if (get_username(user) < 0)
        {
            break;
        }

        char *contentLength = getenv("CONTENT_LENGTH");
        int len;


        if (contentLength != NULL)
        {
            len = strtol(contentLength, NULL, 10);
        }
        else
        {
            len = 0;
        }

        if (len <= 0)
        {
            printf("No data from standard input\n");
        }
        else
        {
            //===============> 得到上传文件  <============
            if (recv_save_file(len, filename) < 0)
            {
                goto END;
            }

            //===============> 将该文件存入fastDFS中,并得到文件的file_id <============
            if (upload_to_dstorage(filename, fileid) < 0)
            {
                goto END;
            }

            //================> 删除本地临时存放的上传文件 <===============
            unlink(filename);

            //================> 得到文件所存放storage的host_name <=================
            if (make_file_url(fileid, fdfs_file_url) < 0)
            {
                goto END;
            }

            //===============> 将该文件的FastDFS路径和名称和上传者存入mysql中 <======
            if (store_fileinfo_to_mysql(fileid, fdfs_file_url, filename, user) < 0)
            {
                goto END;
            }

             //===============> 将该文件的FastDFS路径和名称和上传者 备份 一份到redis中 <======
            if (bk_fileinfo_to_redis(fileid, fdfs_file_url, filename, user) < 0)
            {
                goto END;
            }

END:
            memset(user, 0, USER_NAME_LEN);
            memset(filename, 0, FILE_NAME_LEN);
            memset(fileid, 0, TEMP_BUF_MAX_LEN);
            memset(fdfs_file_url, 0, FILE_URL_LEN);

        }

    } /* while */

    return 0;
}
