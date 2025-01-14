#include "Connection.h"

Connection::Connection(Eventloop *loop, Socket *clientSocket)
    : loop_(loop), clientSocket_(clientSocket)

{
    this->clientChannel_ = new Channel(this->loop_, clientSocket->fd());  // 属于同一个事件循环
    this->clientChannel_->enablereading(), this->clientChannel_->useet(); // 监视读，边缘触发

    this->clientChannel_->setreadcallback(std::bind(&Connection::onmessage, this));
    this->clientChannel_->setWritablecallback(std::bind(&Connection::onWritable, this));
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
            this->inputBuffer_.append(buffer, nread);

            // send(this->clientChannel_->fd(), buffer, strlen(buffer), 0);
        }
        else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取。
            continue;
        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕。
        {
            this->outputBuffer_ = this->inputBuffer_; // 现在似乎和它没关系
            while (true)
            {
                std::string message = this->inputBuffer_.nextMessage();
                if (message.size() == 0) // 不完整报文
                    break;
                this->onmessage_cb_(this, message); // 回调 tcpServer onmessage 进行处理
            }
            break;
        }
        else if (nread == 0) // 客户端连接已断开。
        {
            this->close_cb_(this);
            break;
        }
    }

} // 处理对端发送过来的消息。

// 可写立即尝试全部写入。发送缓冲区清空后禁用写事件
void Connection::onWritable()
{
    int nwrite = ::send(this->fd(), this->outputBuffer_.data(), this->outputBuffer_.size(), 0);
    if (nwrite > 0)
        this->outputBuffer_.erase(0, nwrite);

    if (this->outputBuffer_.size() == 0)
        this->clientChannel_->disableWriting();
}

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

void Connection::send(const std::string &message)
{
    uint32_t size = message.size();
    this->outputBuffer_.append(std::to_string(size) + message, 4 + size);

    // 注册写事件，在被channel回调的 onWritable 中发送数据
    this->clientChannel_->enableWriting();
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

void Connection::setOnmessage_cb(std::function<void(Connection *, std::string)> fn)
{
    this->onmessage_cb_ = fn;
}

void Connection::setClose_cb(std::function<void(Connection *connection)> fn)
{
    this->close_cb_ = fn;
}

void Connection::setError_cb(std::function<void(Connection *connection)> fn)
{
    this->error_cb_ = fn;
}
