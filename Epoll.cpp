#include "Epoll.h"

Epoll::Epoll(int maxevents, int listen_num) : epoll_fd_(epoll_create(listen_num)), events_(maxevents) {}

Epoll::~Epoll(){

}

