#ifndef _UTIL_CGI_H_
#define _UTIL_CGI_H_

#define FILE_NAME_LEN       (256)	//文件名字长度
#define TEMP_BUF_MAX_LEN    (512)	//临时缓冲区大小
#define FILE_URL_LEN        (512)   //文件所存放storage的host_name长度
#define HOST_NAME_LEN       (30)

#define FDFS_CLIENT_CONF    "./conf/FastDFS/client/client.conf" //fastDFS client配置文件

//log 模块
#define UPLOAD_LOG_MODULE "cgi"
#define UPLOAD_LOG_PROC   "upload"


/**
 * @brief  去掉一个字符串两边的空白字符
 *
 * @param inbuf确保inbuf可修改
 *
 * @returns   
 *      0 成功
 *      -1 失败
 */
int trim_space(char *inbuf);

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
char* memstr(char* full_data, int full_data_len, char* substr);


#endif
