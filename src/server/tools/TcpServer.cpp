#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, uint16_t port, int maxN)
{
    this->acceptor_ = new Acceptor(ip, port, &loop_, maxN);
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
