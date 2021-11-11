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



