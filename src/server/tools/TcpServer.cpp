#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, uint16_t port, int nListen, int nSubthreads, int maxGap, int heartCycle, uint16_t bufferType)
    : mainloop_(new Eventloop(true, maxGap, heartCycle)), pool_(nSubthreads, "I/O"),
      acceptor_(ip, port, mainloop_.get(), nListen), bufferType_(bufferType)
{
    this->mainloop_->setEpollTimtoutCallback(std::bind(&TcpServer::epollTimeout, this, std::placeholders::_1));
    for (int i = 0; i < nSubthreads; i++)
    {
        std::unique_ptr<Eventloop> loop(new Eventloop(false, maxGap, heartCycle));
        loop->setEpollTimtoutCallback(std::bind(&TcpServer::epollTimeout, this, std::placeholders::_1));
        loop->setTimer_cb(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
        this->pool_.addTask(std::bind(&Eventloop::run, loop.get()),
                            "Eventloop::run"); // 关联到线程池
        this->subloops_.push_back(std::move(loop));
    }

    this->acceptor_.setNewConnection_cb(
        std::bind(&TcpServer::newConnection, this, std::placeholders::_1));
}

TcpServer::~TcpServer() {}

void TcpServer::start()
{
    this->acceptor_.listen();
    this->mainloop_->run();
}

void TcpServer::stop()
{
    this->mainloop_->stop();
    printf("主事件循环已停止\n");
    for (size_t i = 0; i < this->subloops_.size(); i++)
        subloops_[i]->stop();
    printf("从事件循环已停止\n");
    this->pool_.stopAll();
    printf("I/O线程已停止\n");
}

void TcpServer::newConnection(std::unique_ptr<Socket> clientSocket)
{
    int loopNo = clientSocket->fd() % this->pool_.size();
    conn_sptr clientConnection(new Connection(this->subloops_[loopNo].get(), std::move(clientSocket), this->bufferType_));
    clientConnection->setSendComplete_cb(std::bind(&TcpServer::sendComplete, this, std::placeholders::_1));
    clientConnection->setOnmessage_cb(std::bind(&TcpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    clientConnection->setClose_cb(std::bind(&TcpServer::closeConnection, this, std::placeholders::_1));
    clientConnection->setError_cb(std::bind(&TcpServer::errorConnection, this, std::placeholders::_1));

    this->subloops_[loopNo]->newConnection(clientConnection);
    {
        std::lock_guard<std::mutex> lg(this->mtx_);
        this->connections_[clientConnection->fd()] = clientConnection;
    }

    if (this->newConnection_cb_)
        this->newConnection_cb_(clientConnection);
}

void TcpServer::onMessage(conn_sptr connection, std::string &message)
{
    if (this->onMessage_cb_)
        this->onMessage_cb_(connection, message);
}

void TcpServer::sendComplete(conn_sptr connection)
{
    if (this->sendComplete_cb_)
        this->sendComplete_cb_(connection);

    // 以及其他业务需求代码
}

void TcpServer::closeConnection(conn_sptr connection)
{
    if (this->closeConnection_cb_)
        this->closeConnection_cb_(connection);

    this->removeConnection(connection->fd());
    // delete this->connections_[connection->fd()];
}

void TcpServer::errorConnection(conn_sptr connection)
{
    if (this->errorConnection_cb_)
        this->errorConnection_cb_(connection);

    this->removeConnection(connection->fd());
    // delete this->connections_[connection->fd()];
}

void TcpServer::epollTimeout(Eventloop *loop)
{
    if (this->epollTimeout_cb_)
        this->epollTimeout_cb_(loop);
    // 以及其他业务需求代码
}

void TcpServer::setNewConenctionCallback(std::function<void(conn_sptr)> fn) { this->newConnection_cb_ = fn; }
void TcpServer::setonMessageCallback(std::function<void(conn_sptr, std::string &)> fn) { this->onMessage_cb_ = fn; }
void TcpServer::setSendCompleteCallback(std::function<void(conn_sptr)> fn) { this->sendComplete_cb_ = fn; }
void TcpServer::setCloseConnectionCallback(std::function<void(conn_sptr)> fn) { this->closeConnection_cb_ = fn; }
void TcpServer::setErrorConnectionCallback(std::function<void(conn_sptr)> fn) { this->errorConnection_cb_ = fn; }
void TcpServer::setEpollTimeoutCallback(std::function<void(Eventloop *)> fn) { this->epollTimeout_cb_ = fn; }

void TcpServer::removeConnection(int fd)
{
    std::lock_guard<std::mutex> lg(this->mtx_);
    this->connections_.erase(fd);
}
