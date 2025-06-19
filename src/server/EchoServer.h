#ifndef ECHOSERVER
#define ECHOSERVER
#include <sys/syscall.h>
#include <unistd.h>

#include <string>

#include "../types.h"
#include "TcpServer.h"
#include "ThreadPool.h"

class EchoServer {
   private:
    TcpServer tcpServer_;
    ThreadPool work_thread_pool_;

   public:
    EchoServer(const std::string &ip, uint16_t port, int nListen,
               int nSubthreads, int nWorkThreads, int maxGap, int heartCycle);
    ~EchoServer() = default;

    void start();
    void stop();

    //  -- handler --
    void onNewConnection(ConnectionPtr connection);
    void onMessage(ConnectionPtr connection, MessagePtr message);
    void onSendComplete(ConnectionPtr connection);
    void onConnectionClose(ConnectionPtr connection);
    void onConnectionError(ConnectionPtr connection);
    void onLoopTimeout(Eventloop *loop);

    // 以下函数用于工作线程进行业务计算
    void OnMessage(ConnectionPtr connection, MessagePtr message);
};

#endif  // !ECHOSERVER