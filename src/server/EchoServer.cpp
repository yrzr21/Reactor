#include "EchoServer.h"

EchoServer::EchoServer(const std::string &ip, uint16_t port, int nListen,
                       int nSubthreads, int nWorkThreads, int maxGap,
                       int heartCycle, uint16_t bufferType)
    : tcpServer_(ip, port, nListen, nSubthreads, maxGap, heartCycle,
                 bufferType),
      pool_(nWorkThreads, "work") {
    // set handler
    tcpServer_.setNewConenctionCallback([this](ConnectionPtr connection) {
        this->handleNewConnection(connection);
    });
    tcpServer_.setMessageCallback(
        [this](ConnectionPtr connection, MessagePtr message) {
            this->handleMessage(connection, std::move(message));
        });
    tcpServer_.setSendCompleteCallback([this](ConnectionPtr connection) {
        this->handleSendComplete(connection);
    });
    tcpServer_.setConnectionCloseCallback([this](ConnectionPtr connection) {
        this->handleConnectionClose(connection);
    });
    tcpServer_.setConnectionErrorCallback([this](ConnectionPtr connection) {
        this->handleConnectionError(connection);
    });
    tcpServer_.setLoopTimeoutCallback([this](ConnectionPtr connection) {
        this->handleLoopTimeout(connection);
    });
}

void EchoServer::start() { tcpServer_.start(); }

void EchoServer::stop() {
    pool_.stopAll();
    printf("工作线程已停止\n");
    // sleep(1000);
    tcpServer_.stop();
    printf("tcpServer 已停止\n");
    // sleep(1000);
}

//  -- handler --
void EchoServer::handleNewConnection(ConnectionPtr connection) {
    printf("NewConnection:fd=%d,ip=%s,port=%d ok.\n", connection->fd(),
           connection->ip().c_str(), connection->port());
}
void EchoServer::handleMessage(ConnectionPtr connection, MessagePtr message) {
    // printf("%ld HandleOnMessage: recv(eventfd=%d):%s\n", syscall(SYS_gettid),
    // connection->fd(), message.c_str());

    if (pool_.size() == 0) {
        OnMessage(connection, std::move(message));
    } else {
        // 添加到工作线程。原shared_ptr计数+1，此前的级联调用中计数不变
        pool_.addTask([this](ConnectionPtr connection, MessagePtr message) {
            this->OnMessage(connection, std::move(message));
        });
    }
}
void EchoServer::handleSendComplete(ConnectionPtr connection) {
    // printf("send complete\n");
}
void EchoServer::handleConnectionClose(ConnectionPtr connection) {
    printf("client(eventfd=%d) disconnected.\n", connection->fd());
}
void EchoServer::handleConnectionError(ConnectionPtr connection) {
    // printf("client(eventfd=%d) error.\n", connection->fd());
}
void EchoServer::handleLoopTimeout(Eventloop *loop) {
    // printf("loop time out\n");
}

void EchoServer::OnMessage(ConnectionPtr connection, MessagePtr message) {
    // printf("onMessage runing on thread(%ld).\n", syscall(SYS_gettid)); //
    // 显示线程ID。os分配的唯一id 经过计算构造好的回应报文
    std::string ret = "reply: " + message;
    // sleep(2);
    // printf("处理完业务后，将使用connecion对象。\n");
    connection->postSend(std::move(ret));
}
