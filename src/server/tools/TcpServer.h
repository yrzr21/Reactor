#ifndef TCPSERVER
#define TCPSERVER
#include <string>
#include <map>
#include <functional>
#include "InetAddress.h"
#include "Socket.h"
#include "Eventloop.h"
#include "Acceptor.h"
#include "Connection.h"

class TcpServer
{
private:
    Eventloop loop_;     // 多个，暂时一个
    Acceptor *acceptor_; // 一个server一个acceptor

    std::map<int, Connection *> connections_;

    // 以下为 EchoServer 注册到本类中的回调函数
    std::function<void(Connection *)> newConnection_cb_;
    std::function<void(Connection *, std::string &)> onMessage_cb_;
    std::function<void(Connection *)> sendComplete_cb_;
    std::function<void(Connection *)> closeConnection_cb_;
    std::function<void(Connection *)> errorConnection_cb_;
    std::function<void(Eventloop *)> epollTimeout_cb_;

public:
    TcpServer(const std::string &ip, uint16_t port, int maxN); // 初始化监听的 socket，初始化 servChannel
    ~TcpServer();

    void start(); // 开始监听，有客户端连接后启动事件循环。原文中仅作启动事件循环

    // 注册到 acceptor 中进行回调
    void newConnection(Socket *clientSocket);

    // 以下注册到 connection 中进行回调
    void onMessage(Connection *connection, std::string &message); // 输入一条完整的报文进行处理, 此处选择添加报文头后发送回去
    void sendComplete(Connection *connection);
    void closeConnection(Connection *connection);
    void errorConnection(Connection *connection);

    // 以下注册到 Eventloop 进行回调
    void epollTimeout(Eventloop *loop);

    // set 回调
    void setNewConenctionCallback(std::function<void(Connection *)> fn);
    void setonMessageCallback(std::function<void(Connection *, std::string &)> fn);
    void setSendCompleteCallback(std::function<void(Connection *)> fn);
    void setCloseConnectionCallback(std::function<void(Connection *)> fn);
    void setErrorConnectionCallback(std::function<void(Connection *)> fn);
    void setEpollTimeoutCallback(std::function<void(Eventloop *)> fn);
};

#endif // !TCPSERVER