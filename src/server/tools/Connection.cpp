#include "Connection.h"

Connection::Connection(Eventloop *loop, Socket *clientSocket)
    : loop_(loop), clientSocket_(clientSocket)

{
    this->clientChannel_ = new Channel(this->loop_, clientSocket->fd());  // 属于同一个事件循环
    this->clientChannel_->enablereading(), this->clientChannel_->useet(); // 监视读，边缘触发

    this->clientChannel_->setreadcallback(std::bind(&Connection::onmessage, this));
    this->clientChannel_->setClosecallback(std::bind(&Connection::onClose, this));
    this->clientChannel_->setErrorcallback(std::bind(&Connection::onError, this));
}

Connection::~Connection()
{
    delete clientSocket_;
    delete clientChannel_;
}

void Connection::onmessage()
{
    char buffer[1024];
    while (true) // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
    {
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(this->clientChannel_->fd(), buffer, sizeof(buffer));
        if (nread > 0) // 成功的读取到了数据。
        {
            // 把接收到的报文内容原封不动的发回去。
            printf("recv(eventfd=%d):%s\n", this->clientChannel_->fd(), buffer);
            send(this->clientChannel_->fd(), buffer, strlen(buffer), 0);
        }
        else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取。
            continue;
        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕。
            break;
        else if (nread == 0) // 客户端连接已断开。
        {
            printf("client(eventfd=%d) disconnected.\n", this->clientChannel_->fd());
            close(this->clientChannel_->fd()); // 关闭客户端的fd。
            break;
        }
    }

} // 处理对端发送过来的消息。

void Connection::onClose()
{
    this->close_cb_(this);

    printf("client(eventfd=%d) disconnected.\n", this->fd());
    // 以下不必要，因为tcpserver中关闭connection，connection会关闭socket，socket负责关闭fd
    // close(this->fd()); // 关闭客户端的fd。
}

void Connection::onError()
{
    this->error_cb_(this);

    printf("client(eventfd=%d) error.\n", this->fd());
    // 以下不必要，因为tcpserver中关闭connection，connection会关闭socket，socket负责关闭fd
    // close(this->fd()); // 关闭客户端的fd。
}

int Connection::fd() const // 返回fd_成员。
{
    return this->clientSocket_->fd();
}

std::string Connection::ip() const
{
    return this->clientSocket_->ip();
}

uint16_t Connection::port() const
{
    return this->clientSocket_->port();
}

void Connection::setClose_cb(std::function<void(Connection *connection)> fn)
{
    this->close_cb_ = fn;
}

void Connection::setError_cb(std::function<void(Connection *connection)> fn)
{
    this->error_cb_ = fn;
}
