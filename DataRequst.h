#pragma onece

#include <string>
#include <memory>

class DataRequst{
public:
    typedef std::shared_ptr<DataRequst> SP_DataRequest;

    DataRequst();
    ~DataRequst();

    int getFd() { return fd_; };
    int getEpollfd() { return epoll_fd_; };
    void setFd(int fd) { fd_ = fd; };
    void setEpollFd(int epollfd) { epoll_fd_ = epollfd; };

    void handRead(){

    }

    void handWrite(){

    }

    void handConn(){

    }

    bool canRead() { return enable_read_ ; }
    bool canWrite() { return enable_write_; }
    void enAbleRead() { enable_read_ = true; }
    void enAbleWrite() { enable_write_ = true; }
    void disAbleWriteAndRead() { enable_write_ = false;
                                 enable_read_ = false;}

private:
    int fd_;
    int epoll_fd_;
    
    uint32_t events_;
    std::string in_buffer_;    //接受的信息
    std::string out_buffer_;   //发送的信息

    bool enable_read_;
    bool enable_write_;
};
