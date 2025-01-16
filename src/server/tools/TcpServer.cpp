#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, uint16_t port, int nListen, int nThreads)
    : mainloop_(new Eventloop), pool_(new ThreadPool(nThreads))
{
    this->mainloop_->setEpollTimtoutCallback(std::bind(&TcpServer::epollTimeout, this, std::placeholders::_1));
    for (int i = 0; i < nThreads; i++)
    {
        Eventloop *loop = new Eventloop;
        this->subloops_.push_back(loop);
        loop->setEpollTimtoutCallback(std::bind(&TcpServer::epollTimeout, this, std::placeholders::_1));
        this->pool_->addTask(std::bind(&Eventloop::run, loop)); // 关联到线程池
    }

    this->acceptor_ = new Acceptor(ip, port, mainloop_, nListen);
    this->acceptor_->setNewConnection_cb(
        std::bind(&TcpServer::newConnection, this, std::placeholders::_1));
}

TcpServer::~TcpServer()
{
    delete this->acceptor_;
    for (auto &connection : this->connections_)
        delete connection.second;
}

void TcpServer::start()
{
    this->acceptor_->listen();
    this->mainloop_->run();
}

void TcpServer::newConnection(Socket *clientSocket)
{
    int loopNo = clientSocket->fd() % this->pool_->size();
    Connection *clientConnection = new Connection(this->subloops_[loopNo], clientSocket);
    clientConnection->setSendComplete_cb(std::bind(&TcpServer::sendComplete, this, std::placeholders::_1));
    clientConnection->setOnmessage_cb(std::bind(&TcpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    clientConnection->setClose_cb(std::bind(&TcpServer::closeConnection, this, std::placeholders::_1));
    clientConnection->setError_cb(std::bind(&TcpServer::errorConnection, this, std::placeholders::_1));

    this->connections_[clientSocket->fd()] = clientConnection;

    if (this->newConnection_cb_)
        this->newConnection_cb_(clientConnection);
}

void TcpServer::onMessage(Connection *connection, std::string &message)
{
    if (this->onMessage_cb_)
        this->onMessage_cb_(connection, message);
}

void TcpServer::sendComplete(Connection *connection)
{
    if (this->sendComplete_cb_)
        this->sendComplete_cb_(connection);

    // 以及其他业务需求代码
}

void TcpServer::closeConnection(Connection *connection)
{
    if (this->closeConnection_cb_)
        this->closeConnection_cb_(connection);

    this->connections_.erase(connection->fd());
    delete this->connections_[connection->fd()];
}

void TcpServer::errorConnection(Connection *connection)
{
    if (this->errorConnection_cb_)
        this->errorConnection_cb_(connection);

    this->connections_.erase(connection->fd());
    delete this->connections_[connection->fd()];
}

void TcpServer::epollTimeout(Eventloop *loop)
{
    if (this->epollTimeout_cb_)
        this->epollTimeout_cb_(loop);
    // 以及其他业务需求代码
}

void TcpServer::setNewConenctionCallback(std::function<void(Connection *)> fn) { this->newConnection_cb_ = fn; }
void TcpServer::setonMessageCallback(std::function<void(Connection *, std::string &)> fn) { this->onMessage_cb_ = fn; }
void TcpServer::setSendCompleteCallback(std::function<void(Connection *)> fn) { this->sendComplete_cb_ = fn; }
void TcpServer::setCloseConnectionCallback(std::function<void(Connection *)> fn) { this->closeConnection_cb_ = fn; }
void TcpServer::setErrorConnectionCallback(std::function<void(Connection *)> fn) { this->errorConnection_cb_ = fn; }
void TcpServer::setEpollTimeoutCallback(std::function<void(Eventloop *)> fn) { this->epollTimeout_cb_ = fn; }