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
    ThreadPool pool_;  // IO线程

    Acceptor acceptor_;

    ConnectionMap connections_;
    Mutex mtx_;

    // -- callback --
    ConnectionEventCallback new_connection_callback_;
    MessageCallback message_callback_;
    ConnectionEventCallback send_complete_callback_;
    ConnectionEventCallback connection_close_callback_;
    ConnectionEventCallback connection_error_callback_;
    LoopTimeoutCallback loop_timeout_callback_;

   private:
    void removeConnection(int fd);

   public:
    // 初始化监听的 socket，初始化 servChannel
    TcpServer(const std::string &ip, uint16_t port, int nListen,
              int nSubthreads, int maxGap, int heartCycle);
    ~TcpServer();

    // 开始监听，有客户端连接后启动事件循环。原文中仅作启动事件循环
    void start();
    void stop();  // 停止所有事件循环

    // -- Acceptor hanler --
    void handleNewConnection(SocketPtr clientSocket);

    // -- Connection handler --
    void handleMessage(ConnectionPtr connection, MessagePtr message);
    void handleSendComplete(ConnectionPtr connection);
    void handleConnectionClose(ConnectionPtr connection);
    void handleConnectionError(ConnectionPtr connection);

    // -- Eventloop handler --
    void handleLoopTimeout(Eventloop *loop);
    void handleTimer(IntVector &wait_timeout_fds);

    // -- setter --
    void setNewConenctionCallback(ConnectionEventCallback fn);
    void setMessageCallback(MessageCallbackfn);
    void setSendCompleteCallback(ConnectionEventCallback fn);
    void setConnectionCloseCallback(ConnectionEventCallback fn);
    void setConnectionErrorCallback(ConnectionEventCallback fn);
    void setLoopTimeoutCallback(LoopTimeoutCallback fn);
};

#endif  // !TCPSERVER