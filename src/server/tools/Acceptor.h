#ifndef ACCEPTOR
#define ACCEPTOR
#include "Socket.h"
#include "Channel.h"

class Acceptor
{
private:
    Eventloop *loop_; // 被loop中epoll监视，不可delete loop

    Socket *acceptSocket;   // 管理, 析构函数中释放
    Channel *acceptChannel; // 管理, 析构函数中释放
    
    int maxN_;              // listen最大

public:
    Acceptor(const std::string &ip, uint16_t port, Eventloop *loop, int maxN);
    ~Acceptor();

    void listen(); // 我添加了listen，在服务器 start 后调用它。而不是阻塞在listen
};

#endif // !ACCEPTOR