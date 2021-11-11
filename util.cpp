#include "util.h"

#include <string.h>
#include <signal.h>

void handSignal(){
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, nullptr)){
        return;
    }
}



