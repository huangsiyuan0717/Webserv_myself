#pragma onece
#include "Epoll.h"
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

class ThreadTask{
public:
    std::function<void(std::shared_ptr<void>)> func;
    std::shared_ptr<void> argc;

};

class ThreadPoll
{
public:
    ThreadPoll(int thread_num, int queue_size);
    ~ThreadPoll();

    int threadpollAdd(std::shared_ptr<void> args, std::function<void(std::shared_ptr<void>)> fun);
    int threadpollDestory();
    void threadpollRun();


private:
    std::mutex mtx;
    std::condition_variable cond;
    std::vector<std::thread> threads;
    std::vector<ThreadTask> queue_tasks;

    int push_pos;
    int pop_pos;
    int thread_count_;
    int task_count;
    int queue_maxsize_;
    int shut_down;
};


