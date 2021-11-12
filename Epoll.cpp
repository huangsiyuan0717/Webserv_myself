#include "Epoll.h"
#include "util.h"

Epoll::Epoll(int max_events, int listen_num){
    if((epoll_fd_ = epoll_create(listen_num)) < 0){
        std::cout << "Creat epoll_fd failed! " << std::endl;
        exit(0);
    }
    max_events_ = max_events;
    events_ = new epoll_event[max_events];
}

Epoll::~Epoll(){

}

int  Epoll::epoll_add(int fd, SP_DataRequest request, uint32_t events){
    struct epoll_event event;
    event.data.fd = fd;
    event.events = events;
    fd2req[fd] = request;
    if(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &event) < 0){
        std::cout << "epoll_add failed !" << std::endl;
        return -1;
    }
    return 0;
}

int Epoll::epoll_mod(SP_DataRequest request, uint32_t events){
    struct epoll_event event;
    int fd = request->getFd();
    event.data.fd = fd;
    event.events = events;
    if(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event) < 0){
        std::cout << "epoll_mod failed !" << std::endl;
        return -1;
    }
    return 0;
}

int Epoll::epoll_del(SP_DataRequest request){
    int fd = request->getFd();
    struct epoll_event event;
    event.data.fd = fd;
    event.events = 0;
    if(epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event) < 0){
        std::cout << "epoll_del failed !" << std::endl;
        return -1;
    }
    fd2req[fd].reset();
    return 0;
}

//返回活跃时间给线程池
void Epoll::my_epoll_wait(int listen_fd, int timeout){
    int event_count = epoll_wait(epoll_fd_, events_, max_events_, timeout);
    if(event_count < 0){
        std::cout << "epoll_wait failed!" << std::endl;
    }
    auto data_reqs = getRequest(listen_fd, event_count);
    if(data_reqs.size() > 0){
        for(auto &req : data_reqs){
            if(ThreadPoll::threadpollAdd(req) < 0){
                break;    //continue试试
            }
        }
    }
}

void Epoll::acceptConn(int listen_fd){
    struct sockaddr_in client;
    memset(&client, 0, sizeof(client));
    int accept_fd = 0;
    
    while(accept_fd = accept(listen_fd, (sockaddr *)&client, (socklen_t *)sizeof(client)) > 0){
        std::cout << "Client: " << inet_ntoa(client.sin_addr) 
                  << "Port:" << ntohs(client.sin_port) << std::endl;

        if(accept_fd >= MAXFDS){
            close(accept_fd);
            continue;
        }

        int ret = setSocketNonBlocking(accept_fd);
        if(ret < 0){
            std::cout << std::endl;
            std::cout << "set nonblock failed!" << std::endl;
            return;
        }

        SP_DataRequest req(new DataRequst(epoll_fd_, accept_fd));
        uint32_t events = EPOLLIN | EPOLLET | EPOLLONESHOT;
        epoll_add(accept_fd, req, events);
    }
}

std::vector<SP_DataRequest> Epoll::getRequest(int listen_fd, int event_count){
    std::vector<SP_DataRequest> data_reqs;
    for(int i = 0; i < event_count; i++){
        int fd = events_[i].data.fd;
        if(fd < 3){
            std::cout << "fd < 3" << std::endl;
            break;
        }else if(fd == listen_fd){
            acceptConn(listen_fd);
        }else{

            //排除错误事件
            if((events_[i].events & EPOLLERR) || events_[i].events & EPOLLHUP){
                std::cout << "events error!" << std::endl;
                fd2req[fd].reset();
                continue;
            }

            //把事件加入线程池
            SP_DataRequest curr_req = fd2req[fd];
            if(curr_req){
                if((events_[i].events & EPOLLIN) || (events_[i].events & EPOLLPRI)){
                    curr_req->enAbleRead();
                }else{
                    curr_req->enAbleWrite();
                }
                data_reqs.push_back(curr_req);
                std::cout << "getEventsRequest fd==" << fd << std::endl;
                fd2req[fd].reset();
            }else{
                std::cout << "invlid curr_req !" << std::endl;
            }
        }
    }
    return data_reqs;
}









