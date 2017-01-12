cp ./conf/FastDFS/storage/http.conf /etc/fdfs/
cp ./conf/FastDFS/storage/mime.types /etc/fdfs/
cp ./conf/FastDFS/storage/mod_fastdfs.conf	/etc/fdfs/

#拷贝nginx配置文件到指定目录
cp ./conf/web_server/nginx.conf /usr/local/nginx/conf
cp ./conf/web_server/html/*  /usr/local/nginx/html -R
