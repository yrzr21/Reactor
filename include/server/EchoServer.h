#ifndef ECHOSERVER
#define ECHOSERVER
#include <sys/syscall.h>
#include <unistd.h>

#include <string>

#include "TcpServer.h"
#include "ThreadPool.h"
#include "types.h"

class EchoServer {
   private:
    TcpServer tcpServer_;
    ThreadPool pool_;

   public:
    EchoServer(const std::string &ip, uint16_t port, int nListen,
               int nSubthreads, int nWorkThreads, int maxGap, int heartCycle,
               uint16_t bufferType);
    ~EchoServer() = default;

    void start();
    void stop();

    //  -- handler --
    void handleNewConnection(ConnectionPtr connection);
    void handleMessage(ConnectionPtr connection, MessagePtr message);
    void handleSendComplete(ConnectionPtr connection);
    void handleConnectionClose(ConnectionPtr connection);
    void handleConnectionError(ConnectionPtr connection);
    void handleLoopTimeout(Eventloop *loop);

    // 以下函数用于工作线程进行业务计算
    void OnMessage(ConnectionPtr connection, MessagePtr message);
};

#endif  // !ECHOSERVER