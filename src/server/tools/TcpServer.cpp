#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, uint16_t port, int maxN)
{
    this->acceptor_ = new Acceptor(ip, port, &loop_, maxN);
    this->acceptor_->setNewConnection_cb(
        std::bind(&TcpServer::newConnection, this, std::placeholders::_1));

    this->loop_.setEpollTimtoutCallback(std::bind(&TcpServer::epollTimeout, this, std::placeholders::_1));
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
    this->loop_.run();
}

void TcpServer::newConnection(Socket *clientSocket)
{
    Connection *clientConnection = new Connection(&this->loop_, clientSocket);
    clientConnection->setSendComplete_cb(std::bind(&TcpServer::sendComplete, this, std::placeholders::_1));
    clientConnection->setOnmessage_cb(std::bind(&TcpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    clientConnection->setClose_cb(std::bind(&TcpServer::closeConnection, this, std::placeholders::_1));
    clientConnection->setError_cb(std::bind(&TcpServer::errorConnection, this, std::placeholders::_1));

    this->connections_[clientSocket->fd()] = clientConnection;

    printf("accept client(fd=%d,ip=%s,port=%d) ok.\n", clientSocket->fd(), clientSocket->ip().c_str(), clientSocket->port());
}

void TcpServer::onMessage(Connection *connection, std::string message)
{
    printf("recv(eventfd=%d):%s\n", connection->fd(), message.c_str());

    // 经过一系列计算得到一个回应报文，此处仅在前面加一个reply, 并添加报文头
    message = "reply: " + message;
    message = std::to_string(message.size()) + message;
    connection->send(message);

    // send(connection->fd(), message.data(), message.size(), 0);
}

void TcpServer::sendComplete(Connection *connection)
{
    printf("send complete\n");

    // 以及其他业务需求代码
}

void TcpServer::closeConnection(Connection *connection)
{
    this->connections_.erase(connection->fd());
    delete this->connections_[connection->fd()];
}

void TcpServer::errorConnection(Connection *connection)
{
    this->connections_.erase(connection->fd());
    delete this->connections_[connection->fd()];
}

void TcpServer::epollTimeout(Eventloop *loop)
{
    printf("loop time out\n");
    
    // 以及其他业务需求代码
}
