#ifndef TCPSERVER
#define TCPSERVER
#include <string>
#include <map>
#include "InetAddress.h"
#include "Socket.h"
#include "Eventloop.h"
#include "Acceptor.h"
#include "Connection.h"

class TcpServer
{
private:
    Eventloop loop_;    // 多个，暂时一个
    Acceptor* acceptor_; // 一个server一个acceptor

    std::map<int, Connection*> connections_;

public:
    TcpServer(const std::string &ip, uint16_t port, int maxN); // 初始化监听的 socket，初始化 servChannel
    ~TcpServer();

    void start(); // 开始监听，有客户端连接后启动事件循环。原文中仅作启动事件循环
    void newConnection(Socket *clientSocket);
    void closeConnection(Connection *connection);
    void errorConnection(Connection *connection);
};

#endif // !TCPSERVER