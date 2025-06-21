#include "TcpServer.h"

#include <iostream>
#include <cassert>

TcpServer::TcpServer(const std::string &ip, uint16_t port, int nListen,
                     int nSubthreads, int maxGap, int heartCycle)
    : mainloop_(std::make_unique<Eventloop>(true, maxGap, heartCycle)),
      io_thread_pool_(nSubthreads, "I/O"),
      acceptor_(ip, port, mainloop_.get(), nListen) {
    // set handler
    mainloop_->setLoopTimeoutHandler(
        [this](Eventloop *loop) { this->onLoopTimeout(loop); });

    for (int i = 0; i < nSubthreads; i++) {
        LoopPtr loop = std::make_unique<Eventloop>(false, maxGap, heartCycle);
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

    acceptor_.setNewConnectionHandler(
        [this](SocketPtr conn) { this->onNewConnection(std::move(conn)); });

    for (int i = 0; i < nSubthreads; ++i) {
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
}

void TcpServer::start() {
    acceptor_.listen();
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

    // set callback function
    clientConnection->setMessageHandler(
        [this](ConnectionPtr conn, MessagePtr message) {
            this->onMessage(std::move(conn), std::move(message));
        });
    clientConnection->setSendCompleteHandler(
        [this](ConnectionPtr conn) { this->onSendComplete(std::move(conn)); });
    clientConnection->setCloseHandler([this](ConnectionPtr conn) {
        this->onConnectionClose(std::move(conn));
    });
    clientConnection->setErrorHandler([this](ConnectionPtr conn) {
        this->onConnectionError(std::move(conn));
    });

    // 继续回调 EchoServer
    if (handle_new_connection_) handle_new_connection_(clientConnection);

    // register in loop & ConnectionMap
    subloops_[loopNo]->registerConnection(clientConnection);
    {
        UniqueLock lock(*mutexes_[loopNo]);
        // std::lock_guard<std::mutex> lg(mtx_);
        // connections_.emplace(clientConnection->fd(), clientConnection);
        connection_maps_[loopNo].emplace(clientConnection->fd(), clientConnection);
    }

    // 服务器知晓、并准备好一切后，再注册，后续可添加拒绝注册等逻辑
    // 业务部分仅留别拖太久
    clientConnection->enable();
}

// -- Connection handler --
void TcpServer::onMessage(ConnectionPtr connection, MessagePtr message) {
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