#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, uint16_t port, int nListen,
                     int nSubthreads, int maxGap, int heartCycle)
    : mainloop_(new Eventloop(true, maxGap, heartCycle)),
      pool_(nSubthreads, "I/O"),
      acceptor_(ip, port, mainloop_.get(), nListen) {
    // set handler
    mainloop_->setLoopTimeoutCallback(
        [this](Eventloop *loop) { this->handleLoopTimeout(loop); });

    for (int i = 0; i < nSubthreads; i++) {
        LoopPtr loop = std::make_unique<Eventloop>(false, maxGap, heartCycle);
        // set handler
        loop->setLoopTimeoutCallback(
            [this](Eventloop *loop) { this->handleLoopTimeout(loop); });

        loop->setTimerCallback([this](IntVector &wait_timeout_fds) {
            this->handleTimer(wait_timeout_fds);
        });

        // run loop in thread
        pool_.addTask(std::bind(&Eventloop::run, loop.get()), "Eventloop::run");
        subloops_.push_back(std::move(loop));
    }

    // acceptor_.setNewConnection_cb(std::bind(
    //     &TcpServer::handleNewConnection, this, std::placeholders::_1));

    acceptor_.setNewConnectionCallBack([this](ConnectionPtr conn) {
        this->handleNewConnection(std::move(conn));
    };);
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

    pool_.stopAll();
    // printf("I/O线程已停止\n");
}

// -- Acceptor hanler --
void TcpServer::handleNewConnection(SocketPtr clientSocket) {
    // create connection
    int loopNo = clientSocket->fd() % pool_.size();
    Eventloop *loopPtr = subloops_[loopNo].get();
    ConnectionPtr clientConnection =
        std::make_shared<bufferType_>(loopPtr, std::move(clientSocket));

    // set callback function
    clientConnection->setMessageCallback(
        [this](ConnectionPtr conn, MessagePtr message) {
            this->handleMessage(std::move(conn), std::move(message));
        };);
    clientConnection->setSendCompleteCallback([this](ConnectionPtr conn) {
        this->handleSendComplete(std::move(conn))
    };);
    clientConnection->setCloseCallback([this](ConnectionPtr conn) {
        this->handleConnectionClose(std::move(conn))
    };);
    clientConnection->setErrorCallback([this](ConnectionPtr conn) {
        this->handleConnectionError(std::move(conn))
    };);

    // register in loop & ConnectionMap
    subloops_[loopNo]->registerConnection(clientConnection);
    {
        std::lock_guard<std::mutex> lg(mtx_);
        connections_.emplace(clientConnection->fd(), clientConnection);
    }

    // 继续回调 EchoServer
    if (newConnection_cb_) new_connection_callback_(clientConnection);
}

// -- Connection handler --
void TcpServer::handleMessage(ConnectionPtr connection, MessagePtr message) {
    if (message_callback_) message_callback_(connection, std::move(message));
}
void TcpServer::handleSendComplete(ConnectionPtr connection) {
    if (send_complete_callback_) send_complete_callback_(connection);
}
void TcpServer::handleConnectionClose(ConnectionPtr connection) {
    if (connection_close_callback_) connection_close_callback_(connection);

    removeConnection(connection->fd());
}
void TcpServer::handleConnectionError(ConnectionPtr connection) {
    if (connection_error_callback_) connection_error_callback_(connection);

    removeConnection(connection->fd());
}

// -- Eventloop handler --
void TcpServer::handleLoopTimeout(Eventloop *loop) {
    if (loop_timeout_callback_) loop_timeout_callback_(loop);
    // 以及其他业务需求代码
}
void TcpServer::handleTimer(IntVector &wait_timeout_fds) {
    for (auto fd : wait_timeout_fds) removeConnection(fd);
}

// -- setter --
void TcpServer::setNewConenctionCallback(ConnectionEventCallback fn) {
    new_connection_callback_ = fn;
}
void TcpServer::setMessageCallback(MessageCallbackfn) {
    message_callback_ = fn;
}
void TcpServer::setSendCompleteCallback(ConnectionEventCallback fn) {
    send_complete_callback_ = fn;
}
void TcpServer::setConnectionCloseCallback(ConnectionEventCallback fn) {
    connection_close_callback_ = fn;
}
void TcpServer::setConnectionErrorCallback(ConnectionEventCallback fn) {
    connection_error_callback_ = fn;
}
void TcpServer::setLoopTimeoutCallback(LoopTimeoutCallback fn) {
    loop_timeout_callback_ = fn;
}