CC=gcc
CPPFLAGS= -I./include -I/usr/local/include/hiredis -I/usr/include/mysql/
CFLAGS=-Wall 
LIBS=-lhiredis -lfcgi -lm -lmysqlclient

#src
TEST_PATH=./test
CGI_BIN_PATH=./cgi_bin
CGI_SRC_PATH=./cgi_src
COMMON_PATH=./common

#test测试代码
test_log=$(TEST_PATH)/test_log 		  # 日志的使用
test_upload=$(TEST_PATH)/test_upload  # fdfs上传文件
test_redis=$(TEST_PATH)/test_redis    # redis客户端

#cgi程序
upload.cgi=$(CGI_BIN_PATH)/upload.cgi 	#upload.cgi 上传
login.cgi=$(CGI_BIN_PATH)/login.cgi 	#login.cgi  登陆


target=$(test_log) $(test_upload) $(test_redis) $(upload.cgi) $(login.cgi)


ALL:$(target)


#test测试代码
#test_log程序
$(test_log):$(TEST_PATH)/test_log.o $(COMMON_PATH)/make_log.o 
	$(CC) $^ -o $@ $(LIBS)

#upload程序
$(test_upload):$(TEST_PATH)/upload_file.o $(COMMON_PATH)/make_log.o 
	$(CC) $^ -o $@ $(LIBS)
	
#test_redis程序
$(test_redis):$(TEST_PATH)/test_redis.o $(COMMON_PATH)/make_log.o $(COMMON_PATH)/redis_op.o
	$(CC) $^ -o $@ $(LIBS)

#cgi程序
#upload.cgi程序
$(upload.cgi):$(CGI_SRC_PATH)/upload_cgi.o $(COMMON_PATH)/make_log.o  $(COMMON_PATH)/util_cgi.o
	$(CC) $^ -o $@ $(LIBS)
	
#login.cgi程序
$(login.cgi):$(CGI_SRC_PATH)/login_cgi.o $(COMMON_PATH)/make_log.o  $(COMMON_PATH)/util_cgi.o $(COMMON_PATH)/cJSON.o $(COMMON_PATH)/deal_mysql.o $(COMMON_PATH)/cfg.o
	$(CC) $^ -o $@ $(LIBS)


#clean指令
clean:
	-rm -rf $(target) $(TEST_PATH)/*.o $(CGI_SRC_PATH)/*.o $(COMMON_PATH)/*.o

distclean:
	-rm -rf $(target) $(TEST_PATH)/*.o $(CGI_SRC_PATH)/*.o $(COMMON_PATH)/*.o

#将clean目标 改成一个虚拟符号
.PHONY: clean ALL distclean
