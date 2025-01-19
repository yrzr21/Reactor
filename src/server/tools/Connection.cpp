#include "Connection.h"

Connection::Connection(Eventloop *loop, std::unique_ptr<Socket> clientSocket, uint16_t bufferType)
    : loop_(loop), clientSocket_(std::move(clientSocket)), isDisconnected_(false),
      clientChannel_(new Channel(this->loop_, clientSocket_->fd())), lastEventTime_(Timestamp::now()),
      inputBuffer_(bufferType), outputBuffer_(bufferType)

{
    this->clientChannel_->enablereading(), this->clientChannel_->useet(); // 监视读，边缘触发

    this->clientChannel_->setreadcallback(std::bind(&Connection::onmessage, this));
    this->clientChannel_->setWritablecallback(std::bind(&Connection::onWritable, this));
    this->clientChannel_->setClosecallback(std::bind(&Connection::onClose, this));
    this->clientChannel_->setErrorcallback(std::bind(&Connection::onError, this));
}

Connection::~Connection()
{
    // printf("%d 已析构\n", this->fd());
}

void Connection::onmessage()
{
    this->lastEventTime_ = Timestamp::now();
    // printf("当前时间: %s\n", this->lastEventTime_.tostring().c_str());
    char buffer[1024];
    while (true) // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
    {
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(this->clientChannel_->fd(), buffer, sizeof(buffer));
        if (nread > 0) // 成功的读取到了数据。
            this->inputBuffer_.append(buffer, nread);
        else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取。
            continue;
        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕。
        {
            while (true)
            {
                std::string message = this->inputBuffer_.nextMessage();
                if (message.size() == 0) // 不完整报文
                    break;
                this->onmessage_cb_(shared_from_this(), message); // 回调 tcpServer onmessage 进行处理
            }
            break;
        }
        else if (nread == 0) // 客户端连接已断开。
        {
            this->clientChannel_->remove();
            this->close_cb_(shared_from_this());
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
    {
        this->clientChannel_->disableWriting();
        this->sendComplete_cb_(shared_from_this());
    }
}

void Connection::onClose()
{
    this->isDisconnected_ = true;
    this->clientChannel_->remove();
    this->close_cb_(shared_from_this());

    // 以下不必要，因为tcpserver中关闭connection，connection会关闭socket，socket负责关闭fd
    // close(this->fd()); // 关闭客户端的fd。
}

void Connection::onError()
{
    this->isDisconnected_ = true;
    this->clientChannel_->remove();
    this->error_cb_(shared_from_this());

    // 以下不必要，因为tcpserver中关闭connection，connection会关闭socket，socket负责关闭fd
    // close(this->fd()); // 关闭客户端的fd。
}

void Connection::send(std::string &&message)
{
    if (this->isDisconnected_)
    {
        // printf("直接断开\n");
        return;
    }

    // 难以判定生命周期, 故使用智能指针
    std::shared_ptr<std::string> message_ptr(new std::string(std::move(message)));
    if (this->loop_->isInLoop())
        this->sendInIO(message_ptr); // 0工作线程时会用到这个情况
    else
        this->loop_->enqueueTask(std::bind(&Connection::sendInIO, this, message_ptr));
}

void Connection::sendInIO(std::shared_ptr<std::string> message)
{
    // printf("sendInIO: current thread: %ld\n", syscall(SYS_gettid));
    this->outputBuffer_.appendMessage(message->data(), message->size()); // 自动添加4B报文头

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

void Connection::setOnmessage_cb(std::function<void(conn_sptr, std::string &)> fn)
{
    this->onmessage_cb_ = fn;
}

void Connection::setSendComplete_cb(std::function<void(conn_sptr)> fn)
{
    this->sendComplete_cb_ = fn;
}

void Connection::setClose_cb(std::function<void(conn_sptr connection)> fn)
{
    this->close_cb_ = fn;
}

void Connection::setError_cb(std::function<void(conn_sptr connection)> fn)
{
    this->error_cb_ = fn;
}

bool Connection::isTimeout(time_t now, int maxGap)
{
    return now - this->lastEventTime_.toint() >= maxGap;
}