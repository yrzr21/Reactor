#include "TcpServer.h"

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
}

TcpServer::~TcpServer() { stop(); }

void TcpServer::removeConnection(int fd) {
    std::lock_guard<std::mutex> lg(mtx_);
    connections_.erase(fd);
}

void TcpServer::start() {
    acceptor_.listen();
    mainloop_->run();
}

void TcpServer::stop() {
    mainloop_->stop();
    // printf("主事件循环已停止\n");
    for (size_t i = 0; i < subloops_.size(); i++) subloops_[i]->stop();
    // printf("从事件循环已停止\n");

    io_thread_pool_.stopAll();
    // printf("I/O线程已停止\n");
}

// -- Acceptor hanler --
void TcpServer::onNewConnection(SocketPtr clientSocket) {
    // create connection
    int loopNo = clientSocket->fd() % io_thread_pool_.size();
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

    // register in loop & ConnectionMap
    subloops_[loopNo]->registerConnection(clientConnection);
    {
        std::lock_guard<std::mutex> lg(mtx_);
        connections_.emplace(clientConnection->fd(), clientConnection);
    }

    // 继续回调 EchoServer
    if (handle_new_connection_) handle_new_connection_(clientConnection);
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