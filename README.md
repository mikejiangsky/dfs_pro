# dfs_project

>###作者：mike
>###email: mikejiangsky@foxmail.com


##1. 配置文件

### 应用服务器配置

配置文件在 ./conf/web_server/

* nginx.conf ----nginx服务器配置文件


### 前端界面

./conf/web_server/html/ 路径下为前端界面html、js、css代码

将html直接部署在安装好的web服务器下即可
例如：/usr/local/nginx/html/

### FastDFS分布式存储

* storage 

存储服务器配置文件 ./conf/FastDFS/storage.conf  

nginx中FastDFS/模块配置文件 

    ./conf/FastDFS/mod_fastdfs.conf ---> /etc/fdfs/下

    ./conf/FastDFS/mime.types       ---> /etc/fdfs/下

    ./conf/FastDFS/http.conf        ---> /etc/fdfs/下

* tracker ./conf/FastDFS/tracker.conf

* client  ./conf/FastDFS/client.conf

### 缓冲数据库

配置文件在 ./conf/redis/

* redis.conf ----redis服务器配置文件

### 项目配置文件
配置文件在 ./conf
* cfg.json ----项目所需服务器、配置路径信息配置文件



##2. 生成CGI程序

make 后会生成 

cgi_bin/upload.cgi    ----处理文件上传后台cgi程序

cgi_bin/data.cgi      ----处理展示主界面后台cgi程序

cgi_bin/reg.cgi       ----处理用户注册后台cgi程序

cgi_bin/login.cgi     ----处理文件登陆后台cgi程序

##3. 启动脚本
需要超级用户启动脚本
start_up.sh


##4. 部署脚本

路径在 ./conf/Package

>web-server服务器端
un-package-web-server.sh

