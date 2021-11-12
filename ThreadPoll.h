#pragma onece
#include "Epoll.h"
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

const int MAX_THREADS = 1024;
const int MAX_QUEUE = 65535;

class ThreadTask{
public:
    std::function<void(std::shared_ptr<void>)> func;
    std::shared_ptr<void> argc;

};

void myHandle(std::shared_ptr<void> req);

class ThreadPoll
{
public:
    static int threadpollCreate(int thread_count, int queue_maxsize);
    static int threadpollAdd(std::shared_ptr<void> args, std::function<void(std::shared_ptr<void>)> fun = myHandle);
    static int threadpollDestory();
    static void *threadpollRun(void *args);


private:
    static std::mutex mtx_;
    static std::condition_variable cond_;
    static std::vector<std::thread> threads;
    static std::vector<ThreadTask> queue_tasks;

    static int push_pos;
    static int pop_pos;
    static int thread_count_;
    static int task_count;
    static int queue_maxsize_;
    static int shut_down;
};


