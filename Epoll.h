#pragma once
#include "DataRequst.h"

#include <sys/socket.h>
#include <sys/epoll.h>
#include <iostream>
#include <vector>


class Epoll
{
public:
    typedef std::shared_ptr<DataRequst> SP_DataRequest;

    Epoll(int max_events, int listen_num);
    ~Epoll();

    void epoll_add(int fd, SP_DataRequest request, uint32_t events);
    void epoll_mod(int fd, SP_DataRequest request, uint32_t events);
    void epoll_del(int fd, uint32_t events);
    void my_epoll_wait(int listen_fd, int max_events, int timeout);

    std::vector<DataRequst> getRequest(int fd, int event_num);

private:
    static const int MAXFDS = 100000;
    std::vector<epoll_event> events_;
    int epoll_fd_;
    SP_DataRequest fd2req[MAXFDS];

};


