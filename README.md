# Webserv_myself


### 碰到的问题
1. **解析数据方面**
   1. 音视频不能缓冲完
   2. 中文的文本文件问题（中文名字的utf-8的码会转成 %0x的形式，转换会一个字节的形式）
   3. 大文件传输直接服务器崩了（有的浏览器碰到传输大文件，会先关闭管道，重构读写缓冲区，这个时候服务端会接到SIGPIPE，屏蔽即可）（写缓冲区已经满了，无法继续写)/socket为设置非阻塞导致接受了整个文件，发送的时候发送窗口不够导致卡住
2. **改进**
   1. 添加日志
   2. 重构效率高的buff
   3. 目前多线程只是在处理侧，分发侧也可以考虑
   4. 利用的是主线程的while循环来监听事件发生，考虑多线程事件循环来监听事件，muduo中的 timefd，eventfd,socketfd三者都唤醒任务队列。
