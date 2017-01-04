# 杀死 已经启动的后台CGI程序
kill -9 `ps aux | grep "cgi_bin/upload.cgi" | grep -v grep | awk '{print $2}'`


#启动 必要的业务后台cgi应用程序
spawn-fcgi -a 127.0.0.1 -p 8003 -f ./cgi_bin/upload.cgi

#启动MySQL服务器
sudo service mysql restart


#启动nginx服务器
#sudo /usr/local/nginx/sbin/nginx -s stop
#sudo /usr/local/nginx/sbin/nginx
if [ ! -f "/usr/local/nginx/logs/nginx.pid" ]; then
    #no nginx.pid  need start
    sudo /usr/local/nginx/sbin/nginx
    echo "nginx start!"
else
    #has nginx.pid need reload
    sudo /usr/local/nginx/sbin/nginx -s reload
    echo "nginx reload!"
fi

#启动本地tracker
sudo /usr/bin/fdfs_trackerd ./conf/FastDFS/tracker/tracker.conf restart

#启动本地storage
sudo /usr/bin/fdfs_storaged ./conf/FastDFS/storage/storage.conf restart