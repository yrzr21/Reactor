#ifndef ECHOSERVER
#define ECHOSERVER
#include <sys/syscall.h>
#include <unistd.h>

#include <string>

#include "../base/Logger.h"
#include "../base/ServiceProvider.h"
#include "../base/ThreadPool.h"
#include "../types.h"
#include "TcpServer.h"

constexpr size_t MAX_BLOCK_PER_CHUNK = 40;
constexpr size_t LARGEST_BLOCK = 1024;

struct EchoServerConfig {
    TcpServerConfig tcp_server_config;
    ServiceProviderConfig service_provider_config;
    size_t n_work_threads;
    PoolOptions echo_server_pool_options;
};

class EchoServer {
   private:
    SynchronizedPool upstream_;  // 从下往上析构
    TcpServer tcpServer_;
    ThreadPool work_thread_pool_;
    std::optional<ServiceProvider> service_provider_;

   public:
    EchoServer(EchoServerConfig &config);
    // EchoServer(const std::string &ip, uint16_t port, int nListen,
    //            int nSubthreads, int nWorkThreads, int maxGap, int
    //            heartCycle);
    ~EchoServer();

    void start();
    void stop();

    //  -- handler --
    void onNewConnection(ConnectionPtr connection);
    void onMessage(ConnectionPtr connection, MsgVec &&message);
    void onSendComplete(ConnectionPtr connection);
    void onConnectionClose(ConnectionPtr connection);
    void onConnectionError(ConnectionPtr connection);
    void onLoopTimeout(Eventloop *loop);

    // 以下函数用于工作线程进行业务计算
    void sendMessage(ConnectionPtr connection, MsgVec &&message);
};

#endif  // !ECHOSERVER