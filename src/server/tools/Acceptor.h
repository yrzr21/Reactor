#ifndef ACCEPTOR
#define ACCEPTOR
#include <functional>
#include "Socket.h"
#include "Channel.h"

class Acceptor
{
private:
    Eventloop *loop_; // 被loop中epoll监视，不可delete loop

    Socket *acceptSocket;   // 管理, 析构函数中释放
    Channel *acceptChannel; // 管理, 析构函数中释放

    std::function<void(Socket *clientSocket)> newConnection_cb_; // 回调tcpServer中的读事件 handler，传递给对方继续处理

    int maxN_; // listen最大

public:
    Acceptor(const std::string &ip, uint16_t port, Eventloop *loop, int maxN);
    ~Acceptor();

    void listen(); // 我添加了listen，在服务器 start 后调用它。而不是阻塞在listen

    void newconnection(); // 读事件回调，需要注册到channel，处理新客户端连接请求。

    void setNewConnection_cb(std::function<void(Socket *clientSocket)> fn);
};

#endif // !ACCEPTOR