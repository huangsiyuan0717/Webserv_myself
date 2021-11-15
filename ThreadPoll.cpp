#include "ThreadPoll.h"

std::mutex ThreadPoll::mtx_;
std::condition_variable ThreadPoll::cond_;
std::vector<std::thread> ThreadPoll::threads;
std::vector<ThreadTask> ThreadPoll::queue_tasks;

int  ThreadPoll::push_pos = 0;
int  ThreadPoll::pop_pos = 0;
int  ThreadPoll::thread_count_ = 0;
int  ThreadPoll::task_count = 0;
int  ThreadPoll::queue_maxsize_ = 0;
int  ThreadPoll::shut_down = 0;

void myHandle(std::shared_ptr<void> req){
    SP_DataRequest request = std::static_pointer_cast<DataRequst>(req);
    if(request->canRead()){
        request->handRead();
    }else if(request->canWrite()){
        request->handWrite();
    }else{
        request->handConn();
    }
}

int ThreadPoll::threadpollCreate(int thread_num, int queue_size){
    if(thread_num <= 0 || thread_num > MAX_THREADS ||
        queue_size <= 0  ||  queue_size > MAX_QUEUE){
        thread_count_ = 4;
        queue_maxsize_ = 1024;
    }

    thread_count_ = 0;
    queue_maxsize_ = queue_size;
    threads.resize(thread_num);
    queue_tasks.resize(queue_size);

    for(int i = 0; i < thread_num; i++){
        std::thread th(threadpollRun);
        threads[i] = std::move(th);  //threads.emplace_back(th);
        thread_count_++;
    }
    return 0;
}

int ThreadPoll::threadpollAdd(std::shared_ptr<void> args, std::function<void(std::shared_ptr<void>)> fun){
    std::unique_lock<std::mutex> lock(mtx_);
    if(task_count == queue_maxsize_){
        return THREADPOOL_QUEUE_FULL;
    }

    if(shut_down){
        return THREADPOOL_SHUTDOWN;
    }

    queue_tasks[push_pos].args = args;
    queue_tasks[push_pos].func = fun;
    task_count++;
    push_pos = (push_pos + 1) % queue_maxsize_;
    cond_.notify_one();
}

int ThreadPoll::threadpollDestory(){
    mtx_.unlock();
    shut_down = true;
    return 0;
}

void *ThreadPoll::threadpollRun(void *args){
    while(true){
        ThreadTask task;
        std::unique_lock<std::mutex> lock(mtx_);
        while(task_count == 0 && !shut_down){
            cond_.wait(lock, [](){return !queue_tasks.empty(); });
        }

        if(shut_down){
            std::cout << "thread end!" << std::endl;
            return;
        }
        task.args = queue_tasks[pop_pos].args;
        task.func = queue_tasks[pop_pos].func;
        queue_tasks[pop_pos].args.reset();
        queue_tasks[pop_pos].func = nullptr;
        pop_pos = (pop_pos + 1) % queue_maxsize_;
        task_count--;
        lock.unlock();
        (task.func)(task.args);
    }

}

