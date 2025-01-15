#include "EchoServer.h"

EchoServer::EchoServer(const std::string &ip, uint16_t port, int maxN) : tcpServer_(ip, port, maxN)
{
    // 设置回调函数
    this->tcpServer_.setCloseConnectionCallback(std::bind(&EchoServer::HandleCloseConnection, this, std::placeholders::_1));
    this->tcpServer_.setEpollTimeoutCallback(std::bind(&EchoServer::HandleEpollTimeout, this, std::placeholders::_1));
    this->tcpServer_.setErrorConnectionCallback(std::bind(&EchoServer::HandleErrorConnection, this, std::placeholders::_1));
    this->tcpServer_.setNewConenctionCallback(std::bind(&EchoServer::HandleNewConnection, this, std::placeholders::_1));
    this->tcpServer_.setonMessageCallback(std::bind(&EchoServer::HandleOnMessage, this, std::placeholders::_1, std::placeholders::_2));
    this->tcpServer_.setSendCompleteCallback(std::bind(&EchoServer::HandleSendComplete, this, std::placeholders::_1));
}

EchoServer::~EchoServer()
{
}

void EchoServer::start()
{
    this->tcpServer_.start();
}

void EchoServer::HandleNewConnection(Connection *connection)
{
    printf("accept client(fd=%d,ip=%s,port=%d) ok.\n", connection->fd(), connection->ip().c_str(), connection->port());
}
void EchoServer::HandleOnMessage(Connection *connection, std::string &message)
{
    printf("recv(eventfd=%d):%s\n", connection->fd(), message.c_str());

    // 经过一系列计算得到一个回应报文，此处仅在前面加一个reply
    message = "reply: " + message;
    connection->send(message); 
}
void EchoServer::HandleSendComplete(Connection *connection)
{
    printf("send complete\n");
}
void EchoServer::HandleCloseConnection(Connection *connection)
{
    printf("client(eventfd=%d) disconnected.\n", connection->fd());
}
void EchoServer::HandleErrorConnection(Connection *connection)
{
    printf("client(eventfd=%d) error.\n", connection->fd());
}
void EchoServer::HandleEpollTimeout(Eventloop *loop)
{
    printf("loop time out\n");
}