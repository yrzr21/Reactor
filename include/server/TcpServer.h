#ifndef TCPSERVER
#define TCPSERVER
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Acceptor.h"
#include "Connection.h"
#include "Eventloop.h"
#include "InetAddress.h"
#include "Socket.h"
#include "ThreadPool.h"
#include "types.h"

class TcpServer {
   private:
    LoopPtr mainloop_;
    std::vector<LoopPtr> subloops_;
    ThreadPool pool_; // IO线程

    // listen
    Acceptor acceptor_;

    ConnectionMap connections_;
    Mutex mtx_;

    const uint16_t bufferType_;

    // 回调 EchoServer
    EventCallback newConnection_cb_;
    MessageCallback onMessage_cb_;
    EventCallback sendComplete_cb_;
    EventCallback closeConnection_cb_;
    EventCallback errorConnection_cb_;
    TimeoutCallback epollTimeout_cb_;

   public:
    // 初始化监听的 socket，初始化 servChannel
    TcpServer(const std::string &ip, uint16_t port, int nListen,
              int nSubthreads, int maxGap, int heartCycle, uint16_t bufferType);
    ~TcpServer();

    void
    start();  // 开始监听，有客户端连接后启动事件循环。原文中仅作启动事件循环
    void stop();  // 停止所有事件循环

    // 被acceptor回调
    void handleNewConnection(std::unique_ptr<Socket> clientSocket);

    // 以下注册到 connection 中进行回调
    void onMessage(ConnectionPtr connection, std::string &message);

    void sendComplete(ConnectionPtr connection);
    void closeConnection(ConnectionPtr connection);
    void errorConnection(ConnectionPtr connection);

    // 被 Eventloop 回调
    void epollTimeout(Eventloop *loop);

    // set 回调
    void setNewConenctionCallback(EventCallback fn);
    void setonMessageCallback(MessageCallbackfn);
    void setSendCompleteCallback(EventCallback fn);
    void setCloseConnectionCallback(EventCallback fn);
    void setErrorConnectionCallback(EventCallback fn);
    void setEpollTimeoutCallback(TimeoutCallback fn);

    void removeConnection(int fd);
};

#endif  // !TCPSERVER