#ifndef _CFG_H_
#define _CFG_H_

#define CFG_PATH    "./conf/cfg.json" //配置文件路径

/* -------------------------------------------*/
/**
 * @brief  从配置文件中得到相对应的参数
 *
 * @param profile   配置文件路径
 * @param tile      配置文件title名称[title]
 * @param key       key
 * @param value    (out)  得到的value
 *
 * @returns
 *      0 succ, -1 fail
 */
/* -------------------------------------------*/
int get_pro_value(const char *profile, char *tile, char *key, char *value);


#endif
