#ifndef TCPSERVER
#define TCPSERVER
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../base/ThreadPool.h"
#include "../net/Connection/Acceptor.h"
#include "../net/Connection/Connection.h"
#include "../net/Connection/InetAddress.h"
#include "../net/Connection/Socket.h"
#include "../net/Event/Eventloop.h"
#include "../types.h"

struct TcpServerConfig {
    std::string ip = "127.0.0.1";
    uint16_t port = 8080;
    int backlog = 128;
    int nIOThreads = 4;
    int connection_timeout_second = 60;
    int loop_timer_interval = 60;
};

class TcpServer {
   private:
    LoopPtr mainloop_;               // listen
    std::vector<LoopPtr> subloops_;  // io eventloop
    ThreadPool io_thread_pool_;      // IO thread

    Acceptor acceptor_;

    ConnectionMap connections_;

    // 大小等于 io 事件循环数
    ConnectionMapVector connection_maps_;
    MutexPtrVector mutexes_;
    // Mutex mtx_;

    // -- callback --
    ConnectionEventHandler handle_new_connection_;
    MessageHandler handle_message_;
    ConnectionEventHandler handle_send_complete_;
    ConnectionEventHandler handle_connection_close_;
    ConnectionEventHandler handle_connection_error_;
    LoopTimeoutHandler handle_loop_timeout_;

   private:
    void removeConnection(int fd);

   public:
    // 初始化监听的 socket，初始化 servChannel
    TcpServer(const TcpServerConfig& config);
    // TcpServer(const std::string &ip, uint16_t port, int nListen,
    //           int nSubthreads, int maxGap, int heartCycle);
    ~TcpServer();

    // 开始监听，有客户端连接后启动事件循环
    void start();
    void stop();  // 停止所有事件循环

    // -- Acceptor --
    void onNewConnection(SocketPtr clientSocket);

    // -- Connection --
    void onMessage(ConnectionPtr connection, MsgVec &&message);
    void onSendComplete(ConnectionPtr connection);
    void onConnectionClose(ConnectionPtr connection);
    void onConnectionError(ConnectionPtr connection);

    // -- Eventloop --
    void onLoopTimeout(Eventloop *loop);
    void onTimer(IntVector &wait_timeout_fds);

    // -- setter --
    void setNewConenctionHandler(ConnectionEventHandler fn);
    void setMessageHandler(MessageHandler fn);
    void setSendCompleteHandler(ConnectionEventHandler fn);
    void setConnectionCloseHandler(ConnectionEventHandler fn);
    void setConnectionErrorHandler(ConnectionEventHandler fn);
    void setLoopTimeoutHandler(LoopTimeoutHandler fn);
};

#endif  // !TCPSERVER