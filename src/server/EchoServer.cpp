#include "./EchoServer.h"

#include <string>

EchoServer::EchoServer(EchoServerConfig &config)
    : tcpServer_(config.tcp_server_config),
      work_thread_pool_(config.n_work_threads, "work"),
      upstream_(config.echo_server_pool_options) {
    config.service_provider_config.upstream = &upstream_;
    service_provider_.emplace(config.service_provider_config);

    // set handler
    tcpServer_.setNewConenctionHandler([this](ConnectionPtr connection) {
        this->onNewConnection(connection);
    });
    tcpServer_.setMessageHandler(
        [this](ConnectionPtr connection, MsgVec &&message) {
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
    // std::cout << "New Connection: fd=" << connection->fd() << std::endl;
    // printf("NewConnection:fd=%d,ip=%s,port=%d ok.\n", connection->fd(),
    //        connection->ip().c_str(), connection->port());
}
void EchoServer::onMessage(ConnectionPtr connection, MsgVec &&message) {
    // std::cout << "onMessage: fd=" << connection->fd()
    //           << "msg=" << message->c_str() << std::endl;
    // printf("%ld HandleOnMessage: recv(eventfd=%d):%s\n", syscall(SYS_gettid),
    // connection->fd(), message.c_str());

    if (work_thread_pool_.size() == 0) {
        sendMessage(connection, std::move(message));
    } else {
        // 添加到工作线程。原shared_ptr计数+1，此前的级联调用中计数不变

        // ptr 将来可能被继续移动，需要加 mutable
        work_thread_pool_.addTask([this, con_ptr = connection,
                                   msg_ptr = std::move(message)]() mutable {
            this->sendMessage(con_ptr, std::move(msg_ptr));
        });
    }
}
void EchoServer::onSendComplete(ConnectionPtr connection) {
    // printf("send complete\n");
}
void EchoServer::onConnectionClose(ConnectionPtr connection) {
    // printf("client(eventfd=%d) disconnected.\n", connection->fd());
}
void EchoServer::onConnectionError(ConnectionPtr connection) {
    // printf("client(eventfd=%d) error.\n", connection->fd());
}
void EchoServer::onLoopTimeout(Eventloop *loop) {
    // printf("loop time out\n");
}

// using MessagePtr = std::unique_ptr<std::string>;
void EchoServer::sendMessage(ConnectionPtr connection, MsgVec &&message) {
    // printf("onMessage runing on thread(%ld).\n", syscall(SYS_gettid));
    usleep(100);
    size_t nrecved = 0;
    for (size_t i = 0; i < message.size(); i++) {
        nrecved += message[i].size_;
    }

    // 构造多个报文发回去
    auto &unsync_pool = ServiceProvider::getLocalUnsyncPool();
    std::pmr::string msg{&unsync_pool};
    msg += "received: ";
    msg += std::to_string(nrecved);
    msg += " bytes";

    std::pmr::string msg2{msg, &unsync_pool};

    MsgVec msgs;
    msgs.emplace_back(std::move(ret));
    msgs.emplace_back(std::move(ret2));

    // std::cout << "sendMessage: fd=" << connection->fd() << ", msg=" << ret
    //           << ", size=" << ret.size() << std::endl;
    // sleep(2);
    // printf("处理完业务后，将使用connecion对象。\n");
    // printf("ret=%s\n",ret.c_str());
    connection->postSend(std::move(ret));
}
