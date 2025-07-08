#include "TcpServer.h"

#include <cassert>
#include <iostream>

TcpServer::TcpServer(const TcpServerConfig &config)
    : mainloop_(std::make_unique<Eventloop>(
          true, config.connection_timeout_second, config.loop_timer_interval)),
      io_thread_pool_(config.nIOThreads, "I/O"),
      acceptor_(config.ip, config.port, mainloop_.get(), config.backlog),
      connection_maps_(config.nIOThreads) {
    mainloop_->stopTimer();

    // set handler
    mainloop_->setLoopTimeoutHandler(
        [this](Eventloop *loop) { this->onLoopTimeout(loop); });

    // init io eventloop
    for (int i = 0; i < config.nIOThreads; i++) {
        LoopPtr loop =
            std::make_unique<Eventloop>(false, config.connection_timeout_second,
                                        config.loop_timer_interval);
        // set handler
        loop->setLoopTimeoutHandler(
            [this](Eventloop *loop) { this->onLoopTimeout(loop); });

        loop->setTimerHandler([this](IntVector &wait_timeout_fds) {
            this->onTimer(wait_timeout_fds);
        });

        // run loop in thread
        io_thread_pool_.addTask([ptr = loop.get()] { ptr->run(); });
        subloops_.push_back(std::move(loop));
    }

    // acceptor
    acceptor_.setNewConnectionHandler(
        [this](SocketPtr conn) { this->onNewConnection(std::move(conn)); });

    // mutex
    for (int i = 0; i < config.nIOThreads; ++i) {
        mutexes_.emplace_back(std::make_unique<Mutex>());
    }
}

TcpServer::~TcpServer() { stop(); }

void TcpServer::removeConnection(int fd) {
    // // 检查是否会非所属事件循环线程的线程碰这个 fd
    // pid_t tid = syscall(SYS_gettid);
    // assert(connections_[fd]->loop_->loop_tid == tid);

    // std::lock_guard<std::mutex> lg(mtx_);
    int loopNo = fd % io_thread_pool_.size();
    UniqueLock lock(*mutexes_[loopNo]);

    connection_maps_[loopNo].erase(fd);
    subloops_[loopNo]->unregisterConnection(fd); // 不会有死锁问题
}

void TcpServer::start() {
    acceptor_.listen();
    std::cout << "listening..." << std::endl;
    mainloop_->run();
}

void TcpServer::stop() {
    std::cout << "stopping..." << std::endl;
    mainloop_->stop();
    std::cout << "主事件循环已停止" << std::endl;
    for (size_t i = 0; i < subloops_.size(); i++) subloops_[i]->stop();
    std::cout << "从事件循环已停止" << std::endl;

    io_thread_pool_.stopAll();
    std::cout << "I/O线程已停止" << std::endl;
}

// -- Acceptor hanler --
void TcpServer::onNewConnection(SocketPtr clientSocket) {
    // create connection
    int loopNo = clientSocket->fd() % io_thread_pool_.size();
    // std::cout << "fd=" << clientSocket->fd() << ", subloop no=" << loopNo <<
    // std::endl;
    Eventloop *loopPtr = subloops_[loopNo].get();
    ConnectionPtr clientConnection =
        std::make_shared<Connection>(loopPtr, std::move(clientSocket));

    // 继续回调 EchoServer
    if (handle_new_connection_) handle_new_connection_(clientConnection);

    // register in loop & ConnectionMap
    subloops_[loopNo]->registerConnection(clientConnection);
    {
        UniqueLock lock(*mutexes_[loopNo]);
        // std::lock_guard<std::mutex> lg(mtx_);
        // connections_.emplace(clientConnection->fd(), clientConnection);
        connection_maps_[loopNo].emplace(clientConnection->fd(),
                                         clientConnection);
    }

    /*
        事件循环线程内初始化 connection——在业务层知晓、并准备好一切后
        业务层可设置接收缓冲区的 chunk_size 和 max_msg_size，此处暂且设为固定值
     */
    RecvBufferConfig config{4096, 1024};
    subloops_[loopNo]->postTask([clientConnection, config,
                                 server = this]() mutable {
        // handler
        clientConnection->setMessageHandler(
            // 内层 lambda 捕获外层 lambda 的变量
            [server](ConnectionPtr conn, MsgVec &&message) {
                server->onMessage(std::move(conn), std::move(message));
            });
        clientConnection->setSendCompleteHandler([server](ConnectionPtr conn) {
            server->onSendComplete(std::move(conn));
        });
        clientConnection->setCloseHandler([server](ConnectionPtr conn) {
            server->onConnectionClose(std::move(conn));
        });
        clientConnection->setErrorHandler([server](ConnectionPtr conn) {
            server->onConnectionError(std::move(conn));
        });

        // buffer，延迟构造
        clientConnection->initBuffer(config);
        clientConnection->enable();
    });
}

// -- Connection handler --
void TcpServer::onMessage(ConnectionPtr connection, MsgVec &&message) {
    if (handle_message_) handle_message_(connection, std::move(message));
}
void TcpServer::onSendComplete(ConnectionPtr connection) {
    if (handle_send_complete_) handle_send_complete_(connection);
}
void TcpServer::onConnectionClose(ConnectionPtr connection) {
    if (handle_connection_close_) handle_connection_close_(connection);

    removeConnection(connection->fd());
}
void TcpServer::onConnectionError(ConnectionPtr connection) {
    if (handle_connection_error_) handle_connection_error_(connection);

    removeConnection(connection->fd());
}

// -- Eventloop handler --
void TcpServer::onLoopTimeout(Eventloop *loop) {
    if (handle_loop_timeout_) handle_loop_timeout_(loop);
    // 以及其他业务需求代码
}
void TcpServer::onTimer(IntVector &wait_timeout_fds) {
    for (auto fd : wait_timeout_fds) removeConnection(fd);
}

// -- setter --
void TcpServer::setNewConenctionHandler(ConnectionEventHandler fn) {
    handle_new_connection_ = fn;
}
void TcpServer::setMessageHandler(MessageHandler fn) { handle_message_ = fn; }
void TcpServer::setSendCompleteHandler(ConnectionEventHandler fn) {
    handle_send_complete_ = fn;
}
void TcpServer::setConnectionCloseHandler(ConnectionEventHandler fn) {
    handle_connection_close_ = fn;
}
void TcpServer::setConnectionErrorHandler(ConnectionEventHandler fn) {
    handle_connection_error_ = fn;
}
void TcpServer::setLoopTimeoutHandler(LoopTimeoutHandler fn) {
    handle_loop_timeout_ = fn;
}