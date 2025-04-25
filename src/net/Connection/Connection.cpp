#include "Connection.h"

Connection::Connection(Eventloop *loop, SocketPtr clientSocket)
    : loop_(loop), socket_(std::move(clientSocket)) {
    channel_ = std::make_unique<Channel>(loop_, socket_->fd());
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

    // channel_->setreadcallback(
    //     std::bind(&Connection::onmessage, this));
    // channel_->setWritablecallback(
    //     std::bind(&Connection::onWritable, this));
    // channel_->setClosecallback(
    //     std::bind(&Connection::onClose, this));
    // channel_->setErrorcallback(
    //     std::bind(&Connection::onError, this));
}

// 把写操作交给事件循环
void Connection::postSend(std::string &&message) {
    if (disconnected_) return;

    // 难以判定生命周期, 故使用智能指针
    MessagePtr msg_ptr = std::make_unique<std::string>(std::move(message));

    if (loop_->inIOLoop()) {
        // 0工作线程时会用到这个情况
        prepareSend(std::move(message_ptr));
    } else {
        auto self = shared_from_this();
        loop_->postTask([self]((MessagePtr &&message)) {
            self->prepareSend(std::move(message_ptr))
        });
    }
}

// register write event
void Connection::prepareSend(MessagePtr &&message) {
    // printf("prepareSend: current thread: %ld\n", syscall(SYS_gettid));
    output_buffer_.pushMessage(std::move(message));

    channel_->enableEvent(EPOLLOUT);  // do send in Connection::handleWritable()
}

bool Connection::isTimeout(time_t now, int maxGap) {
    return now - lastEventTime_.toint() >= maxGap;
}

// -- getter --
int Connection::fd() const { return socket_->fd(); }
std::string Connection::ip() const { return socket_->ip(); }
uint16_t Connection::port() const { return socket_->port(); }

// -- setter --
void Connection::setMessageCallback(MessageCallback cb) {
    message_callback_ = std::move(cb);
}
void Connection::setSendCompleteCallback(ConnectionEventCallback cb) {
    send_complete_callback_ = std::move(cb);
}
void Connection::setCloseCallback(ConnectionEventCallback cb) {
    close_callback_ = std::move(cb);
}
void Connection::setErrorCallback(ConnectionEventCallback cb) {
    error_callback_ = std::move(cb);
}

// -- handler --
void Connection::handleMessage() {
    lastEventTime_ = Timestamp::now();
    // printf("当前时间: %s\n", lastEventTime_.tostring().c_str());

    int n = input_buffer_.fillFromFd(socket_.fd());
    if (n == 0) {
        handleClose();
    } else if (n == -1) {
        handleError();
    }

    // 获取报文
    while (true) {
        // NRVO 会自动优化这里
        auto message = input_buffer_.popMessage();
        if (message.size() == 0)  // 不完整报文
            break;

        auto ptr = std::make_unique<std::string>(std::move(message));
        on_message_cb_(shared_from_this(), std::move(ptr));
    }
}
// 可写立即尝试全部写入
// 全部发送完毕后，禁用写并回调TcpServer::sendComplete
void Connection::handleWritable() {
    ssize_t nsend = output_buffer_.sendAllToFd(fd());

    if (output_buffer_.empty()) {
        channel_->disableEvent(EPOLLOUT);
        on_send_complete_cb_(shared_from_this());
    }
}
void Connection::handleClose() {
    disconnected_ = true;
    channel_->remove();
    close_callback_(shared_from_this());

    // close(fd()); // Socket会自己关闭，不用管它
}
void Connection::handleError() {
    disconnected_ = true;
    channel_->remove();
    error_callback_(shared_from_this());

    // close(fd()); // Socket会自己关闭，不用管它
}