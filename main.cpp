#include "Epoll.h"
#include "util.h"
#include "ThreadPoll.h"
#include "DataRequst.h"
#include <iostream>


const int THREAD_NUM = 10;
const int QUEUE_SIZE = 65532;
const int MAXEVENTS = 5000;
const int LISTEN_NUM = 1024;
const int PORT = 5555;

int main(int argc, char **argv){
    handSignal();

    Epoll epoll;    //创建mian_reactor
    if(epoll.myepoll_creat(MAXEVENTS, LISTEN_NUM) < 0){

    }

    ThreadPoll threadpoll(THREAD_NUM, QUEUE_SIZE);      //创建线城池

    int listen_fd = sockInitAndBind(PORT);      //创建监听SOCKET

    //设置非阻塞模式
    //TODO

    //创建事件类，并且加入到epoll堆中，并且监听
    std::shared_ptr<DataRequst> data_req(new DataRequst);
    data_req->setFd(listen_fd);

    epoll.epoll_add(listen_fd, data_req, EPOLLIN | EPOLLET);

    //监听事件，在线程任务中处理
    while(true){
        epoll.my_epoll_wait(listen_fd, MAXEVENTS, -1);
    }

    return 0;
}