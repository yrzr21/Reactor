#include "EchoServer.h"

EchoServer::EchoServer(const std::string &ip, uint16_t port, int nListen, int nSubthreads, int nWorkThreads, int maxGap, int heartCycle)
    : tcpServer_(ip, port, nListen, nSubthreads, maxGap, heartCycle), pool_(nWorkThreads, "work")
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

void EchoServer::stop()
{
    this->pool_.stopAll();
    printf("工作线程已停止\n");
    // sleep(1000);
    this->tcpServer_.stop();
    printf("tcpServer 已停止\n");
    // sleep(1000);
}

void EchoServer::HandleNewConnection(conn_sptr connection)
{
    printf("NewConnection:fd=%d,ip=%s,port=%d ok.\n", connection->fd(), connection->ip().c_str(), connection->port());
}
void EchoServer::HandleOnMessage(conn_sptr connection, std::string &message)
{
    // printf("%ld HandleOnMessage: recv(eventfd=%d):%s\n", syscall(SYS_gettid), connection->fd(), message.c_str());

    if (this->pool_.size() == 0)
        this->OnMessage(connection, message);
    else // 添加到工作线程。原shared_ptr计数+1，此前的级联调用中计数不变
        this->pool_.addTask(std::bind(&EchoServer::OnMessage, this, connection, message),
                            "EchoServer::OnMessage");
}
void EchoServer::HandleSendComplete(conn_sptr connection)
{
    // printf("send complete\n");
}
void EchoServer::HandleCloseConnection(conn_sptr connection)
{
    printf("client(eventfd=%d) disconnected.\n", connection->fd());
}
void EchoServer::HandleErrorConnection(conn_sptr connection)
{
    // printf("client(eventfd=%d) error.\n", connection->fd());
}
void EchoServer::HandleEpollTimeout(Eventloop *loop)
{
    // printf("loop time out\n");
}

void EchoServer::OnMessage(conn_sptr connection, std::string &message)
{
    // printf("onMessage runing on thread(%ld).\n", syscall(SYS_gettid)); // 显示线程ID。os分配的唯一id
    // 经过计算构造好的回应报文
    std::string ret = "reply: " + message;
    // sleep(2);
    // printf("处理完业务后，将使用connecion对象。\n");
    connection->send(std::move(ret));
}
