#include "Acceptor.h"

Acceptor::Acceptor(const std::string &ip, uint16_t port, Eventloop *loop, int maxN)
    : maxN_(maxN), loop_(loop)
{
    InetAddress servaddr(ip, port); // 服务端的地址和协议。

    this->acceptSocket = new Socket(createnonblocking());
    this->acceptSocket->setReuseaddr(true), acceptSocket->setTcpnodelay(true), acceptSocket->setReuseport(true), acceptSocket->setKeepalive(true);
    this->acceptSocket->bind(servaddr);

    this->acceptChannel = new Channel(loop, this->acceptSocket->fd());

    this->acceptChannel->setreadcallback( // 提前绑定默认参数
        std::bind(&Channel::newconnection, acceptChannel, this->acceptSocket));
    this->acceptChannel->enablereading(); // 监视读事件
}

Acceptor::~Acceptor()
{
    delete acceptChannel;
    delete acceptSocket;
}

void Acceptor::listen()
{
    this->acceptSocket->listen(this->maxN_);
}
