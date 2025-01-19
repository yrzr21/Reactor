#ifndef TCPSERVER
#define TCPSERVER
#include <string>
#include <map>
#include <functional>
#include <memory>
#include <vector>
#include "InetAddress.h"
#include "Socket.h"
#include "Eventloop.h"
#include "Acceptor.h"
#include "Connection.h"
#include "ThreadPool.h"

class TcpServer
{
private:
    std::unique_ptr<Eventloop> mainloop_;
    std::vector<std::unique_ptr<Eventloop>> subloops_;
    ThreadPool pool_;

    Acceptor acceptor_; // 一个server一个acceptor

    std::map<int, conn_sptr> connections_;
    std::mutex mtx_;

    // 以下为 EchoServer 注册到本类中的回调函数
    std::function<void(conn_sptr)> newConnection_cb_;
    std::function<void(conn_sptr, std::string &)> onMessage_cb_;
    std::function<void(conn_sptr)> sendComplete_cb_;
    std::function<void(conn_sptr)> closeConnection_cb_;
    std::function<void(conn_sptr)> errorConnection_cb_;
    std::function<void(Eventloop *)> epollTimeout_cb_;

public:
    TcpServer(const std::string &ip, uint16_t port, int nListen, int nSubthreads, int maxGap, int heartCycle); // 初始化监听的 socket，初始化 servChannel
    ~TcpServer();

    void start(); // 开始监听，有客户端连接后启动事件循环。原文中仅作启动事件循环

    // 注册到 acceptor 中进行回调
    void newConnection(std::unique_ptr<Socket> clientSocket);

    // 以下注册到 connection 中进行回调
    void onMessage(conn_sptr connection, std::string &message); // 输入一条完整的报文进行处理, 此处选择添加报文头后发送回去
    void sendComplete(conn_sptr connection);
    void closeConnection(conn_sptr connection);
    void errorConnection(conn_sptr connection);

    // 以下注册到 Eventloop 进行回调
    void epollTimeout(Eventloop *loop);

    // set 回调
    void setNewConenctionCallback(std::function<void(conn_sptr)> fn);
    void setonMessageCallback(std::function<void(conn_sptr, std::string &)> fn);
    void setSendCompleteCallback(std::function<void(conn_sptr)> fn);
    void setCloseConnectionCallback(std::function<void(conn_sptr)> fn);
    void setErrorConnectionCallback(std::function<void(conn_sptr)> fn);
    void setEpollTimeoutCallback(std::function<void(Eventloop *)> fn);

    void removeConnection(int fd);
};

#endif // !TCPSERVER