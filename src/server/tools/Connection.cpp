#include "Connection.h"

Connection::Connection(Eventloop *loop, Socket *clientSocket)
    : loop_(loop), clientSocket_(clientSocket)

{
    this->clientChannel_ = new Channel(this->loop_, clientSocket->fd());  // 属于同一个事件循环
    this->clientChannel_->enablereading(), this->clientChannel_->useet(); // 监视读，边缘触发
    this->clientChannel_->setreadcallback(std::bind(&Channel::onmessage, this->clientChannel_));
}

Connection::~Connection()
{
    delete clientSocket_;
    delete clientChannel_;
}
