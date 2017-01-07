#ifndef URL_CODE_H
#define URL_CODE_H

#define BURSIZE 2048

 /*
  * 1、字符'a'-'z','A'-'Z','0'-'9','.','-','*'和'_' 都不被编码，维持原值；
  * 2、空格' '被转换为加号'+'。
  * 3、其他每个字节都被表示成"%XY"的格式，X和Y分别代表一个十六进制位。编码为UTF-8。
 */

//编码一个url
void urlencode(char url[]);

//解码url
void urldecode(char url[]);

#endif // URL_CODE_H

