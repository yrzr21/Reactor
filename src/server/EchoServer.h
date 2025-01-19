#ifndef ECHOSERVER
#define ECHOSERVER
#include <string>
#include <unistd.h>
#include <sys/syscall.h>
#include "tools/TcpServer.h"
#include "tools/ThreadPool.h"

class EchoServer
{
private:
    TcpServer tcpServer_;
    ThreadPool pool_;

public:
    EchoServer(const std::string &ip, uint16_t port, int nListen, int nSubthreads, int nWorkThreads, int maxGap, int heartCycle);
    ~EchoServer();

    void start(); // 开始监听，有客户端连接后启动事件循环。原文中仅作启动事件循环

    // 以下注册到 tcpServer 中被回调
    void HandleNewConnection(conn_sptr connection);
    void HandleOnMessage(conn_sptr connection, std::string &message);
    void HandleSendComplete(conn_sptr connection);
    void HandleCloseConnection(conn_sptr connection);
    void HandleErrorConnection(conn_sptr connection);
    void HandleEpollTimeout(Eventloop *loop);

    // 以下函数用于工作线程进行业务计算
    void OnMessage(conn_sptr connection, std::string &message);
};

#endif // !ECHOSERVER