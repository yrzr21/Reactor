#ifndef TCPSERVER
#define TCPSERVER
#include <string>
#include "InetAddress.h"
#include "Socket.h"
#include "Eventloop.h"

class TcpServer
{
private:
    Eventloop loop_;
    Socket servsock_; // 原文中把sock放在构造函数中new出来，我选择把它当作成员对待
    Channel *servChannel; // 原文中不作为成员

public:
    TcpServer(const std::string &ip, uint16_t port); // 初始化监听的 socket，初始化 servChannel
    ~TcpServer();

    void start(); // 开始监听，有客户端连接后启动事件循环。原文中仅作启动事件循环
};

#endif // !TCPSERVER