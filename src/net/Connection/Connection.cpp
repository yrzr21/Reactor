#include "Connection.h"

Connection::Connection(Eventloop *loop, std::unique_ptr<Socket> clientSocket)
    : loop_(loop),
      socket_(std::move(clientSocket)),
      channel_(new Channel(this->loop_, clientSocket_->fd())) {
    channel_->enableEvent(EPOLLIN);
    channel_->enableEdgeTrigger();

    // SET callback for conncetion
    channel_->setEventHandler(HandlerType::Readable,
                              [this] { this->hanleMessage(); });
    channel_->setEventHandler(HandlerType::Writable,
                              [this] { this->handleWritable(); });
    channel_->setEventHandler(HandlerType::Closed,
                              [this] { this->handleClose(); });
    channel_->setEventHandler(HandlerType::Error,
                              [this] { this->handleError(); });

    // this->clientChannel_->setreadcallback(
    //     std::bind(&Connection::onmessage, this));
    // this->clientChannel_->setWritablecallback(
    //     std::bind(&Connection::onWritable, this));
    // this->clientChannel_->setClosecallback(
    //     std::bind(&Connection::onClose, this));
    // this->clientChannel_->setErrorcallback(
    //     std::bind(&Connection::onError, this));
}

// Connection::~Connection() {
//      printf("%d 已析构\n", this->fd());
// }

void Connection::onmessage() {}  // 处理对端发送过来的消息。

// 可写立即尝试全部写入。发送缓冲区清空后禁用写事件
void Connection::onWritable() {
    int nwrite = ::send(this->fd(), this->outputBuffer_.data(),
                        this->outputBuffer_.size(), 0);
    if (nwrite > 0) this->outputBuffer_.erase(0, nwrite);

    if (this->outputBuffer_.size() == 0) {
        this->clientChannel_->disableWriting();
        this->sendComplete_cb_(shared_from_this());
    }
}

void Connection::onClose() {
    this->isDisconnected_ = true;
    this->clientChannel_->remove();
    this->close_cb_(shared_from_this());

    // 以下不必要，因为tcpserver中关闭connection，connection会关闭socket，socket负责关闭fd
    // close(this->fd()); // 关闭客户端的fd。
}

void Connection::onError() {
    this->isDisconnected_ = true;
    this->clientChannel_->remove();
    this->error_cb_(shared_from_this());

    // 以下不必要，因为tcpserver中关闭connection，connection会关闭socket，socket负责关闭fd
    // close(this->fd()); // 关闭客户端的fd。
}

void Connection::send(std::string &&message) {
    if (this->isDisconnected_) {
        // printf("直接断开\n");
        return;
    }

    // 难以判定生命周期, 故使用智能指针
    std::shared_ptr<std::string> message_ptr(
        new std::string(std::move(message)));
    if (this->loop_->isInLoop())
        this->sendInIO(message_ptr);  // 0工作线程时会用到这个情况
    else
        this->loop_->enqueueTask(
            std::bind(&Connection::sendInIO, this, message_ptr));
}

void Connection::sendInIO(std::shared_ptr<std::string> message) {
    // printf("sendInIO: current thread: %ld\n", syscall(SYS_gettid));
    this->outputBuffer_.appendMessage(message->data(),
                                      message->size());  // 自动添加4B报文头

    // 注册写事件，在被channel回调的 onWritable 中发送数据
    this->clientChannel_->enableWriting();
}

int Connection::fd() const  // 返回fd_成员。
{
    return this->clientSocket_->fd();
}

std::string Connection::ip() const { return this->clientSocket_->ip(); }

uint16_t Connection::port() const { return this->clientSocket_->port(); }

void Connection::setOnmessage_cb(
    std::function<void(conn_sptr, std::string &)> fn) {
    this->onmessage_cb_ = fn;
}

void Connection::setSendComplete_cb(std::function<void(conn_sptr)> fn) {
    this->sendComplete_cb_ = fn;
}

void Connection::setClose_cb(std::function<void(conn_sptr connection)> fn) {
    this->close_cb_ = fn;
}

void Connection::setError_cb(std::function<void(conn_sptr connection)> fn) {
    this->error_cb_ = fn;
}

template <uint16_t BUFFER_TYPE>
void Connection<BUFFER_TYPE>::handleMessage() {
    this->lastEventTime_ = Timestamp::now();
    // printf("当前时间: %s\n", this->lastEventTime_.tostring().c_str());

    int n = inputBuffer_.fillFromFd(socket_.fd());
    if (n == 0) {  // 断开连接
        channel_->remove();
        onClose_(shared_from_this());
    } else if (n == -1) {  // 错误
        channel_->remove();
        onError(shared_from_this());
    }

    // 获取报文
    while (true) {
        // NRVO 会自动优化这里
        std::string message = inputBuffer_.popMessage();
        if (message.size() == 0)  // 不完整报文
            break;

        // 回调 tcpServer::onmessage 进行处理
        onMessage_(shared_from_this(), message);
    }
}

template <uint16_t BUFFER_TYPE>
void Connection<BUFFER_TYPE>::handleWritable() {}

template <uint16_t BUFFER_TYPE>
void Connection<BUFFER_TYPE>::handleClose() {}

template <uint16_t BUFFER_TYPE>
void Connection<BUFFER_TYPE>::handleError() {}

bool Connection::isTimeout(time_t now, int maxGap) {
    return now - this->lastEventTime_.toint() >= maxGap;
}