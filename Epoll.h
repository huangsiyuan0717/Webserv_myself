#pragma once
#include "DataRequst.h"
#include "ThreadPoll.h"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <iostream>
#include <vector>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>


typedef std::shared_ptr<DataRequst> SP_DataRequest;

class Epoll
{
public:

    Epoll(int max_events, int listen_num);
    ~Epoll();

    int epoll_add(int fd, SP_DataRequest request, uint32_t events);
    int epoll_mod(SP_DataRequest request, uint32_t events);
    int epoll_del(SP_DataRequest request);
    void my_epoll_wait(int listen_fd, int timeout);
    void acceptConn(int listen_fd);

    std::vector<SP_DataRequest> getRequest(int fd, int event_count);    //获得有事件发生的数组

private:
    static const int MAXFDS = 100000;
    int max_events_;
    epoll_event *events_;
    int epoll_fd_;
    SP_DataRequest fd2req[MAXFDS];
};


