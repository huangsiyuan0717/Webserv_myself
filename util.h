#pragma once

#include<fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void handSignal();  //处理客户端传完数据后断开连接，发送PIPE信号给服务端产生的异常

int sockInitAndBind(int port, int listen_num);
int setSocketNonBlocking(int fd);



