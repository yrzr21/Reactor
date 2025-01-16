#ifndef ECHOSERVER
#define ECHOSERVER
#include <string>
#include <unistd.h>
#include <sys/syscall.h>
#include "tools/TcpServer.h"

class EchoServer
{
private:
    TcpServer tcpServer_;

public:
    EchoServer(const std::string &ip, uint16_t port, int nListen, int nThreads);
    ~EchoServer();

    void start(); // 开始监听，有客户端连接后启动事件循环。原文中仅作启动事件循环

    // 以下注册到 tcpServer 中被回调
    void HandleNewConnection(Connection *connection);
    void HandleOnMessage(Connection *connection, std::string &message);
    void HandleSendComplete(Connection *connection);
    void HandleCloseConnection(Connection *connection);
    void HandleErrorConnection(Connection *connection);
    void HandleEpollTimeout(Eventloop *loop);
};

#endif // !ECHOSERVER