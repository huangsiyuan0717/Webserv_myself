#include "util.h"

#include <string.h>
#include <signal.h>
#include <iostream>

void handSignal(){
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, nullptr)){
        return;
    }
}

int setSocketNonBlocking(int fd){
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag < 0){
        std::cout << "GETFL failed!" << std::endl;
        return -1;
    }
    flag = flag | O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flag) < 0){
        return -1;
    }
    return 0;
}

int sockInitAndBind(int port, int listen_num){
    if(port < 1024 || port > 65535){
        return -1;
    }

    int listen_fd = 0;
    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0) == -1)){
        std::cout << "creat listen_fd failed !" << std::endl;
        return -1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(listen_fd, (sockaddr *)&servaddr, sizeof(servaddr)) < 0){
        std::cout << "bind servaddr failed !" << std::endl;
    }

    if(listen(listen_fd, listen_num) < 0){
        std::cout << "listen failed!" << std::endl;
    }

    if(listen_fd < 0){
        std::cout << "bad listen_fd." << std::endl;
    }

    return listen_fd;
}



