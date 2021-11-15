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


    Epoll epoll(MAXEVENTS, LISTEN_NUM);    //创建mian_reactor
    
    ThreadPoll::threadpollCreate(THREAD_NUM, QUEUE_SIZE);      //创建线城池


    int listen_fd = sockInitAndBind(PORT, LISTEN_NUM);      //创建监听SOCKET

    if(listen_fd < 0){
        std::cout << "set listen_fd failed!" << std::endl;
        return 0;
    }

    //设置非阻塞模式
    if(setSocketNonBlocking(listen_fd) < 0){
        std::cout << "set nonblock failed!" << std::endl;
        return 0;
    }

    //创建事件类，并且加入到epoll堆中，并且监听
    SP_DataRequest data_req;
    data_req->setFd(listen_fd);

    epoll.epoll_add(listen_fd, data_req, EPOLLIN | EPOLLET);

    //监听事件，在线程任务中处理
    while(true){
        epoll.my_epoll_wait(listen_fd, -1);
    }

    return 0;
}