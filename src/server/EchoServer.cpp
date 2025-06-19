#include "EchoServer.h"

EchoServer::EchoServer(const std::string &ip, uint16_t port, int nListen,
                       int nSubthreads, int nWorkThreads, int maxGap,
                       int heartCycle)
    : tcpServer_(ip, port, nListen, nSubthreads, maxGap, heartCycle),
      work_thread_pool_(nWorkThreads, "work") {
    // set handler
    tcpServer_.setNewConenctionHandler([this](ConnectionPtr connection) {
        this->onNewConnection(connection);
    });
    tcpServer_.setMessageHandler(
        [this](ConnectionPtr connection, MessagePtr message) {
            this->onMessage(connection, std::move(message));
        });
    tcpServer_.setSendCompleteHandler(
        [this](ConnectionPtr connection) { this->onSendComplete(connection); });
    tcpServer_.setConnectionCloseHandler([this](ConnectionPtr connection) {
        this->onConnectionClose(connection);
    });
    tcpServer_.setConnectionErrorHandler([this](ConnectionPtr connection) {
        this->onConnectionError(connection);
    });
    tcpServer_.setLoopTimeoutHandler(
        [this](Eventloop *loop) { this->onLoopTimeout(loop); });
}

void EchoServer::start() { tcpServer_.start(); }

void EchoServer::stop() {
    work_thread_pool_.stopAll();
    printf("工作线程已停止\n");
    // sleep(1000);
    tcpServer_.stop();
    printf("tcpServer 已停止\n");
    // sleep(1000);
}

//  -- handler --
void EchoServer::onNewConnection(ConnectionPtr connection) {
    printf("NewConnection:fd=%d,ip=%s,port=%d ok.\n", connection->fd(),
           connection->ip().c_str(), connection->port());
}
void EchoServer::onMessage(ConnectionPtr connection, MessagePtr message) {
    // printf("%ld HandleOnMessage: recv(eventfd=%d):%s\n", syscall(SYS_gettid),
    // connection->fd(), message.c_str());

    if (work_thread_pool_.size() == 0) {
        OnMessage(connection, std::move(message));
    } else {
        // 添加到工作线程。原shared_ptr计数+1，此前的级联调用中计数不变

        // ptr 将来可能被继续移动，需要加 mutable
        work_thread_pool_.addTask([this, con_ptr = connection,
                                   msg_ptr = std::move(message)]() mutable {
            this->OnMessage(con_ptr, std::move(msg_ptr));
        });
    }
}
void EchoServer::onSendComplete(ConnectionPtr connection) {
    // printf("send complete\n");
}
void EchoServer::onConnectionClose(ConnectionPtr connection) {
    printf("client(eventfd=%d) disconnected.\n", connection->fd());
}
void EchoServer::onConnectionError(ConnectionPtr connection) {
    // printf("client(eventfd=%d) error.\n", connection->fd());
}
void EchoServer::onLoopTimeout(Eventloop *loop) {
    // printf("loop time out\n");
}

// using MessagePtr = std::unique_ptr<std::string>;
void EchoServer::OnMessage(ConnectionPtr connection, MessagePtr message) {
    // printf("onMessage runing on thread(%ld).\n", syscall(SYS_gettid));

    std::string ret = "reply: " + std::move(*message);
    // sleep(2);
    // printf("处理完业务后，将使用connecion对象。\n");
    connection->postSend(std::move(ret));
}
