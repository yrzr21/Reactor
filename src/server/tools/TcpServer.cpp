#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, uint16_t port, int maxN)
{
    this->acceptor_ = new Acceptor(ip, port, &loop_, maxN);
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
    this->loop_.run();
}

void TcpServer::newConnection(Socket *clientSocket)
{
    Connection *clientConnection = new Connection(&this->loop_, clientSocket);
    clientConnection->setClose_cb(std::bind(&TcpServer::closeConnection, this, std::placeholders::_1));
    clientConnection->setError_cb(std::bind(&TcpServer::errorConnection, this, std::placeholders::_1));

    this->connections_[clientSocket->fd()] = clientConnection;

    printf("accept client(fd=%d,ip=%s,port=%d) ok.\n", clientSocket->fd(), clientSocket->ip().c_str(), clientSocket->port());
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
