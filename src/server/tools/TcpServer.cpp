#include "TcpServer.h"

TcpServer::TcpServer(const std::string &ip, uint16_t port) : servsock_(createnonblocking())
{
    InetAddress servaddr(ip, port); // 服务端的地址和协议。
    this->servsock_.setReuseaddr(true), servsock_.setTcpnodelay(true), servsock_.setReuseport(true), servsock_.setKeepalive(true);
    this->servsock_.bind(servaddr);

    this->servChannel = new Channel(&this->loop_, this->servsock_.fd());
    // 提前绑定默认参数
    this->servChannel->setreadcallback(std::bind(&Channel::newconnection, servChannel, &this->servsock_));
    this->servChannel->enablereading(); // 监视读事件
}

TcpServer::~TcpServer()
{
}

void TcpServer::start()
{
    this->servsock_.listen();
    this->loop_.run();
}
