#include "util.h"
#include "epoll.h"
#include "requestData.h"
#include <sys/epoll.h> //linux 平台独有的, windows系统没有
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include <unordered_map>
#include <queue>
#include <fcntl.h>
#include <iostream>


void MimeType::init()
{
    mime[".html"] = "text/html";
    mime[".avi"] = "video/x-msvideo";
    mime[".bmp"] = "image/bmp";
    mime[".c"] = "text/plain";
    mime[".doc"] = "application/msword";
    mime[".gif"] = "image/gif";
    mime[".gz"] = "application/x-gzip";
    mime[".htm"] = "text/html";
    mime[".ico"] = "application/x-ico";
    mime[".jpg"] = "image/jpeg";
    mime[".png"] = "image/png";
    mime[".txt"] = "text/plain";
    mime[".mp3"] = "audio/mp3";
}

std::string MimeType::getMime(const std::string &suffix)
{
    pthread_once(&once_control, MimeType::init);
    if (mime.find(suffix) == mime.end())
        return mime["default"];
    else
        return mime[suffix];
}

RequestData::RequestData() : now_read_pos(0),
                             state(STATE_PARSE_URI),
                             h_state(h_start),
                             keep_alive(true),
                             isAbleRead(true),
                             isAbleWrite(false),
                             events(0),
                             error(false) { std::cout << "---RequestData()---\n" << std::endl; }

RequestData::RequestData(int _epollfd, int _fd, std::string _path) : now_read_pos(0),
                                                                     state(STATE_PARSE_URI),
                                                                     h_state(h_start),
                                                                     keep_alive(true),
                                                                     path(_path),
                                                                     fd(_fd),
                                                                     epollfd(_epollfd),
                                                                     isAbleRead(true),
                                                                     isAbleWrite(false),
                                                                     events(0),
                                                                     error(false) { std::cout << "---RequestData(int _epollfd, int _fd, std::string _path)---\n" << std::endl; }

RequestData::~RequestData()
{
    std::cout << "---~RequestData()---\n" << std::endl;
    close(fd);
}

int RequestData::getFd()
{
    return fd;
}

void RequestData::setFd(int _fd)
{
    fd = _fd;
}

void RequestData::reset()
{
    inBuffer.clear();
    file_name.clear();
    path.clear();
    now_read_pos = 0;
    state = STATE_PARSE_URI;
    h_state = h_start;
    headers.clear();
    if (timer.lock())
    {
        std::shared_ptr<TimerNode> my_timer(timer.lock());
        my_timer->clearReq();
        timer.reset();
    }
}

void RequestData::seperateTimer()
{
    if (timer.lock())
    {
        std::shared_ptr<TimerNode> my_timer(timer.lock());
        my_timer->clearReq();
        timer.reset();
    }
}

void RequestData::handleRead()
{
    do
    {
        int read_num = readn(fd, inBuffer);
        printf("read_num= %d \n", read_num);
        if (read_num < 0)
        {
            perror("1");
            error = true;
            handleError(fd, 400, "Bad request");
            break;
        }
        else if (read_num == 0)
        {
            error = true;
            break;
        }
        if (state == STATE_PARSE_URI)
        {
            int flag = this->parse_URI();
            if (flag == PARSE_URI_AGAIN)
            {
                break;
            }
            else if (flag == PARSE_URI_ERROR)
            {
                perror("2");
                error = true;
                handleError(fd, 400, "Bad request");
                break;
            }
            else
                state = STATE_PARSE_HEADERS;
        }
        if (state = STATE_PARSE_HEADERS)
        {
            int flag = this->parse_Headers();
            if (flag == PARSE_HEADER_AGAIN)
                break;
            else if (flag == PARSE_HEADER_ERROR)
            {
                perror("3");
                error = true;
                handleError(fd, 400, "Bad request");
                break;
            }
            if (method == METHOD_POST) //先解析URI，成功再解析header，成功开始准备post发送数据
            {
                state = STATE_RECV_BODY;
            }
            else
                state = STATE_ANALYSIS;
        }
        if (state == STATE_RECV_BODY)
        {
            int content_length = -1;
            if (headers.find("Content-length") != headers.end())
            {
                content_length = std::stoi(headers["content-length"]);
            }
            else
            {
                error = true;
                handleError(fd, 400, "Bad request: 缺少参数：content-length");
                break;
            }
            if (inBuffer.size() < content_length)
                break;
            state = STATE_ANALYSIS;
        }
        if (state == STATE_ANALYSIS)
        {
            int flag = this->analysisRequest();
            if (flag == ANALYSIS_SUCCESS)
            {
                state = STATE_FINISH;
                break;
            }
            else if (flag == ANALYSIS_ERROR)
                printf("inBuffer.size() < content_length，分析失败");
            else
            {
                error = true;
                break;
            }
        }
    } while (false); // do...while...循环
    if  (!error)
    {
        if (outBuffer.size() > 0)
            events |= EPOLLOUT;
        if (state == STATE_FINISH)
        {
            std::cout << "keep_alive=" << keep_alive << std::endl;
            if (keep_alive)
            {
                this->reset();
                events |= EPOLLIN;
            }
            else
                return;
        }
        else
            events |= EPOLLIN;
    }
}

void RequestData::handleWrite()
{
    if (!error)
    {
        if (writen(fd, outBuffer) < 0)
        {
            perror("writen /n");
            events = 0;
            error = true;
        }
        else if (outBuffer.size() > 0)
            events |= EPOLLOUT;
    }
}

void RequestData::handleConn()
{
    if (!error)
    {
        if (events != 0)
        {
            // 一定要先加时间信息，否则可能会出现刚加进去，下个in触发来了，
            //然后分离失败后，又加入队列，最后超时被删，然后正在线程中进行的任务出错，double free错误。
            // 新增时间信息
            int timeout = 200;
            if (keep_alive)
            {
                timeout = 5 * 60 * 10000;
            }
            isAbleRead = false;
            isAbleWrite = false;
            Epoll::add_timer(shared_from_this(), timeout);
            if ((events & EPOLLIN) && (events & EPOLLOUT))
            {
                events = __uint32_t(0);
                events |= EPOLLOUT;
            }
            events |= EPOLLET| EPOLLONESHOT;
            __uint32_t _events = events;
            events = 0;
            if (Epoll::epoll_mod(fd, shared_from_this(), _events) < 0)
            {
                printf("Epoll::epoll_mod error\n");
            }
        }
        else if (keep_alive)
        {
            events |= (EPOLLIN | EPOLLET | EPOLLONESHOT);
            int timeout = 5 * 6 * 10000;
            isAbleRead = false;
            isAbleWrite = false;
            Epoll::add_timer(shared_from_this(), timeout);
            __uint32_t _events = events;
            events = 0;
            if (Epoll::epoll_mod(fd, shared_from_this(), _events) < 0)
            {
                printf("Epoll::epoll_mod error\n");
            }
        }
    }
}

int RequestData::parse_URI()
{
    std::string &str = inBuffer;
    //读到完整的请求再开始解析请求
    int pos = str.find('\r', now_read_pos);
    if (pos < 0)
    {
        return PARSE_URI_AGAIN;
    }
    // 去掉请求行所占的空间， 节省空间
    std::string request_line = str.substr(0, pos);
    if (str.size() > pos + 1)
    {
        str = str.substr(pos + 1);
    }
    else
    {
        str.clear();
    }
    pos = request_line.find("GET");
    if (pos < 0)
    {
        pos = request_line.find("POST");
        if (pos < 0)
            return PARSE_URI_ERROR;
        else
            method = METHOD_POST;
    }
    else
        method = METHOD_GET;
    pos = request_line.find("/", pos);
    if (pos < 0)
    {
        return PARSE_URI_ERROR;
    }
    else
    {
        int _pos = request_line.find(' ', pos);
        if (_pos < 0)
            return PARSE_URI_ERROR;
        else
        {
            if (_pos - pos > 1)
            {
                file_name = request_line.substr(pos + 1, _pos - pos - 1);
                int _pos = file_name.find('?');
                if (_pos >= 0)
                {
                    file_name = file_name.substr(0, _pos);
                }
            }
            else
                file_name = "index.html";
        }
        pos = _pos;
    }
    // HTTP 版本號
    pos = request_line.find('/', pos);
    if (pos < 0)
    {
        return PARSE_URI_ERROR;
    }
    else
    {
        if (request_line.size() - pos <= 3)
            return PARSE_URI_ERROR;
        else
        {
            std::string ver = request_line.substr(pos + 1, 3);
            if (ver == "1.0")
            {
                HTTPversion = HTTP_10;
            }
            else if (ver == "1.1")
            {
                HTTPversion = HTTP_11;
            }
            else
                return PARSE_URI_ERROR;
        }
    }
    return PARSE_URI_SUCCESS;
}

int RequestData::parse_Headers()
{
    std::string &str = inBuffer;
    int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
    int now_read_line_begin = 0;
    bool notFinish = true;
    for (int i = 0; i < str.size() && not notFinish; ++i)
    {
        switch (h_state)
        {
        case h_start:
        {
            if (str[i] == '\n' || str[i] == '\r')
                break;
            h_state = h_key;
            key_start = i;
            now_read_line_begin = i;
            break;
        }

        case h_key:
        {
            if (str[i] == ':')
            {
                h_state = h_spaces_after_colon;
            }
            else
                return PARSE_HEADER_ERROR;
            break;
        }
        case h_spaces_after_colon:
        {
            h_state = h_value;
            value_start = i;
            break;
        }
        case h_value:
        {
            if (str[i] == '\r') //回车
            {
                h_state = h_CR;
                value_end = i;
                if (value_end - value_start <= i)
                {
                    return PARSE_HEADER_ERROR;
                }
            }
            else if (i - value_start > 255)
                return PARSE_HEADER_ERROR;
            break;
        }
        case h_CR:
        {
            if (str[i] == '\n')
            {
                h_state = h_LF;
                std::string key(str.begin() + key_start, str.begin() + key_end);
                std::string value(str.begin() + value_start, str.begin() + key_end);
                headers[key] = value;
                now_read_line_begin = i;
            }
            return PARSE_HEADER_ERROR;
        }
        case h_LF:
        {
            if (str[i] == '\r')
            {
                h_state = h_end_CR;
            }
            else
            {
                key_start = i;
                h_state = h_key;
            }
            break;
        }
        case h_end_CR:
        {
            if (str[i] == '\n')
            {
                h_state = h_end_LF;
            }
            else
                return PARSE_HEADER_ERROR;
            break;
        }
        case h_end_LF:
        {
            notFinish = false;
            key_start = i;
            now_read_line_begin = i;
            break;
        }
        default:
            break;
        }
    }
    if (h_state == h_end_LF)
    {
        str == str.substr(now_read_line_begin);
        return PARSE_HEADER_SUCCESS;
    }
    str = str.substr(now_read_line_begin);
    return PARSE_HEADER_AGAIN;
}

int RequestData::analysisRequest()
{
    if (method == METHOD_POST)
    {
        //get inBuffer
        std::string header;
        header += std::string("HTTP/1.1 200 OK\r\n"); //响应头和标头字段，浏览器查看
        if (headers.find("Connection") != headers.end() && headers["Connection"] == "keep-alive")
        {
            std::cout << headers["Connection"] << std::endl;
            keep_alive = true;
            header += std::string("Connection: keep-alive\r\n") + "keep-alive: timeout = " + std::to_string(5 * 60 * 1000) + "\r\n";
        }
        int length = std::stoi(headers["Conten-length"]);
            std::vector<char> data(inBuffer.begin(), inBuffer.begin() + length);
            std::cout << "data.size()" << data.size() << std::endl;
            cv::Mat src = cv::imdecode(data, cv::IMREAD_ANYDEPTH);
            cv::imwrite("receive.bmp", src);
            std::cout << "-->bmp" << std::endl;
            cv::Mat res = stitch(src);

            std::cout << "---2--" << std::endl;
            std::vector<uchar> data_encode;
            cv::imencode(".png", src, data_encode);
            std::cout << "---3--" << std::endl;
            header += std::string("Content-length:") + std::to_string(data_encode.size()) + "\r\n";
            std::cout << "---4--" << std::endl;
            outBuffer += header + std::string(data_encode.begin(), data_encode.end());
            std::cout << "---5--" << std::endl;
            inBuffer = inBuffer.substr(length);
            return ANALYSIS_SUCCESS;
        
    }
    else if (method == METHOD_GET)
    {
        std::string header;
        header += std::string("HTTP/1.1 200 OK\r\n"); //响应头和标头字段，浏览器查看
        if (headers.find("Connection") != headers.end() && headers["Connection"] == "keep-alive")
        {
            std::cout << headers["Connection"] << std::endl;
            keep_alive = true;
            header += std::string("Connection: keep-alive\r\n") + "keep-alive: timeout = " + std::to_string(5 * 60 * 1000) + "\r\n";
        }
        int dot_pos = file_name.find('.');
        std::string filetype;
        if (dot_pos < 0)
        {
            filetype = MimeType::getMime("default");
        }
        else
            filetype = MimeType::getMime(file_name.substr(dot_pos));

        struct stat sbuf;
        if (stat(file_name.c_str(), &sbuf) < 0)
        {
            header.clear();
            handleError(fd, 404, "Not Found");
            return ANALYSIS_ERROR;
        }
        header += "Content-type: " + filetype + "\r\n";
        header += "COntent-length" + std::to_string(sbuf.st_size) + "\r\n";
        // 头部结束

        header += "\r\n";
        outBuffer += header;

        int src_fd = open(file_name.c_str(), O_RDONLY, 0);
        char *src_addr = static_cast<char *>(mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0));
        close(src_fd);

        outBuffer += src_addr;
        munmap(src_addr, sbuf.st_size);

        return ANALYSIS_SUCCESS;
    }
    else
        return ANALYSIS_ERROR;
}

void RequestData::linkTimer(std::shared_ptr<TimerNode> mtimer)
{
    // shared_ptr重载了bool, 但weak_ptr没有
    //if (!timer.lock())
        timer = mtimer;
}

void RequestData::handleError(int fd, int err_num, std::string short_msg)
{
    short_msg == "" + short_msg;
    char send_buff[MAX_BUFF];
    std::string body_buff, header_buff;
    body_buff += "<html><title>哎~出错了</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += std::to_string(err_num) + short_msg;
    body_buff += "<hr><em> LinYa's Web Server</em>\n</body></html>";

    header_buff += "HTTP/1.1 " + std::to_string(err_num) + short_msg + "\r\n";
    header_buff += "Content-type: text/html\r\n";
    header_buff += "Connection: close\r\n";
    header_buff += "Content-length: " + std::to_string(body_buff.size()) + "\r\n";
    header_buff += "\r\n";

    // 错误处理不考虑writen不完的情况
    sprintf(send_buff, "%s", header_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
    sprintf(send_buff, "%s", body_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
}

void RequestData::disableReadAndWrite()
{
    isAbleRead = false;
    isAbleWrite = false;
}

void RequestData::enableRead()
{
    isAbleRead = true;
}

void RequestData::enableWrite()
{
    isAbleWrite = true;
}

bool RequestData::canRead()
{
    return isAbleRead;
}

bool RequestData::canWrite()
{
    return isAbleWrite;
}