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

}

int ThreadPoll::threadpollDestory(){

}

void *ThreadPoll::threadpollRun(void *args){

}

