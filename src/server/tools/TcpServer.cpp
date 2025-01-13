#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, uint16_t port, int maxN)
{
    this->acceptor_ = new Acceptor(ip, port, &loop_, maxN);
    this->acceptor_->setReadCallback(
        std::bind(&TcpServer::newConnection, this, std::placeholders::_1));
}

TcpServer::~TcpServer()
{
    delete this->acceptor_;
}

void TcpServer::start()
{
    this->acceptor_->listen();
    this->loop_.run();
}

void TcpServer::newConnection(Socket *clientSocket)
{
    Connection *clientConnection = new Connection(&this->loop_, clientSocket);
}
