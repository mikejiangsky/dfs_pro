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
#include "fcgi_stdio.h"
#include "make_log.h" //日志头文件
#include "util_cgi.h" //cgi后台通用接口，trim_space(), memstr()

//log 模块
#define UPLOAD_LOG_MODULE "cgi"
#define UPLOAD_LOG_PROC   "upload"

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
		
		//通过execlp执行fdfs_upload_file
		execlp("fdfs_upload_file", "fdfs_upload_file", FDFS_CLIENT_CONF, filename, NULL);
		
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
		
		execlp("fdfs_file_info", "fdfs_file_info", FDFS_CLIENT_CONF, fileid, NULL);
		
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

		strcat(fdfs_file_url, "http://");
		strcat(fdfs_file_url, fdfs_file_host_name);
		strcat(fdfs_file_url, ":");
		strcat(fdfs_file_url, "80");
		strcat(fdfs_file_url, "/");
		strcat(fdfs_file_url, fileid);

		printf("[%s]\n", fdfs_file_url);
		LOG(UPLOAD_LOG_MODULE, UPLOAD_LOG_PROC, "file url is: %s\n", fdfs_file_url);
	}

END:
	return retn;
}


int main()
{
    char filename[FILE_NAME_LEN] = {0};		//上传文件的名字
	char fileid[TEMP_BUF_MAX_LEN] = {0};	//文件上传到fastDFS后的文件id
	char fdfs_file_url[FILE_URL_LEN] = {0};	//文件所存放storage的host_name

    while (FCGI_Accept() >= 0) 
	{
        char *contentLength = getenv("CONTENT_LENGTH");
        int len;

        printf("Content-type: text/html\r\n"
                "\r\n");


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

END:
            memset(filename, 0, FILE_NAME_LEN);
			memset(fileid, 0, TEMP_BUF_MAX_LEN);
			memset(fdfs_file_url, 0, FILE_URL_LEN);

        }
		
    } /* while */

    return 0;
}
