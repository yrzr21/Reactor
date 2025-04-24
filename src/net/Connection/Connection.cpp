#include "Connection.h"

Connection::Connection(Eventloop *loop, std::unique_ptr<Socket> clientSocket)
    : loop_(loop),
      socket_(std::move(clientSocket)),
      channel_(new Channel(loop_, clientSocket_->fd())) {
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

// Connection::~Connection() {
//      printf("%d 已析构\n", fd());
// }

void Connection::send(std::string &&message) {
    if (disconnected_) return;

    // 难以判定生命周期, 故使用智能指针
    MessagePtr msg_ptr = std::make_shared<std::string>(message);

    if (loop_->inIOLoop()) {
        // 0工作线程时会用到这个情况
        sendInIO(message_ptr);
    } else {
        loop_->postTask([this] { this->sendInIO(std::move(message_ptr)) });
    }
}

// do send
void Connection::sendInIO(MessagePtr &&message) {
    // printf("sendInIO: current thread: %ld\n", syscall(SYS_gettid));
    output_buffer_.pushMessage(std::forward<MessagePtr>(message));

    // 注册写事件，在被channel回调的 onWritable 中发送数据
    channel_->enableWriting();
}

int Connection::fd() const { return clientSocket_->fd(); }
std::string Connection::ip() const { return clientSocket_->ip(); }
uint16_t Connection::port() const { return clientSocket_->port(); }

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

bool Connection::isTimeout(time_t now, int maxGap) {
    return now - lastEventTime_.toint() >= maxGap;
}

void Connection::onClose() {
    disconnected_ = true;
    channel_->remove();
    on_close_cb_(shared_from_this());

    // 以下不必要，因为tcpserver中关闭connection，connection会关闭socket，socket负责关闭fd
    // close(fd()); // 关闭客户端的fd。
}

void Connection::onError() {
    disconnected_ = true;
    channel_->remove();
    on_error_cb_(shared_from_this());

    // 以下不必要，因为tcpserver中关闭connection，connection会关闭socket，socket负责关闭fd
    // close(fd()); // 关闭客户端的fd。
}

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
        std::string message = input_buffer_.popMessage();
        if (message.size() == 0)  // 不完整报文
            break;

        // 回调 tcpServer::onmessage 进行处理
        on_message_cb_(shared_from_this(), message);
    }
}

// 可写立即尝试全部写入
// 全部发送完毕后，禁用写并回调TcpServer::sendComplete
void Connection::handleWritable() {
    ssize_t nsend = output_buffer_.sendAllToFd(fd());

    if (output_buffer_.empty()) {
        channel_->disableWriting();
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