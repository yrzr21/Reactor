#include "Connection.h"

#include <iostream>

Connection::Connection(Eventloop *loop, SocketPtr clientSocket)
    : loop_(loop), socket_(std::move(clientSocket)) {
    channel_ = std::make_unique<Channel>(loop_, socket_->fd());

    // SET callback for conncetion
    channel_->setEventHandler(HandlerType::Readable,
                              [this] { this->onMessage(); });
    channel_->setEventHandler(HandlerType::Writable,
                              [this] { this->onWritable(); });
    channel_->setEventHandler(HandlerType::Closed, [this] { this->onClose(); });
    channel_->setEventHandler(HandlerType::Error, [this] { this->onError(); });

    // 必须先设置handler，再监听...不然有事件了handler还没好
    // channel_->enableEvent(EPOLLIN);
    // channel_->enableEdgeTrigger();
}

Connection::~Connection() {
    // std::cout<<"fd "<<fd()<<" closed"<<std::endl;
}

// tcpserver 在万事具备的时候才会允许 Connection 在 epoll 注册事件
void Connection::enable() {
    channel_->enableEvent(EPOLLIN);
    channel_->enableEdgeTrigger();
}

// 把写操作交给事件循环
void Connection::postSend(std::string &&message) {
    if (disconnected_) return;

    MessagePtr msg_ptr = std::make_unique<std::string>(std::move(message));

    if (loop_->inIOThread()) {
        prepareSend(std::move(msg_ptr));  // 0 工作线程
    } else {
        auto self = shared_from_this();
        // auto task = [this, ptr = std::move(msg_ptr)]() mutable {
        //     this->prepareSend(std::move(ptr));
        // };
        // loop_->postTask(std::move(task));
        loop_->postTask([this, ptr = std::move(msg_ptr)]() mutable {
            this->prepareSend(std::move(ptr));
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
void Connection::setMessageHandler(MessageHandler cb) {
    handle_message_ = std::move(cb);
}
void Connection::setSendCompleteHandler(ConnectionEventHandler cb) {
    handle_send_complete_ = std::move(cb);
}
void Connection::setCloseHandler(ConnectionEventHandler cb) {
    handle_close_ = std::move(cb);
}
void Connection::setErrorHandler(ConnectionEventHandler cb) {
    handle_error_ = std::move(cb);
}

// -- handler --
void Connection::onMessage() {
    lastEventTime_ = Timestamp::now();
    // printf("当前时间: %s\n", lastEventTime_.tostring().c_str());

    int n = input_buffer_.fillFromFd(socket_->fd());
    if (n == 0) [[unlikely]] {
        onClose();
    } else if (n == -1) [[unlikely]] {
        onError();
    }

    // 获取报文
    while (true) {
        // NRVO 会自动优化这里
        auto message = input_buffer_.popMessage();
        if (message.size() == 0)  // 不完整报文
            break;

        auto ptr = std::make_unique<std::string>(std::move(message));
        handle_message_(shared_from_this(), std::move(ptr));
    }
}
// 可写立即尝试全部写入
// 全部发送完毕后，禁用写并回调TcpServer::sendComplete
void Connection::onWritable() {
    ssize_t nsend = output_buffer_.sendAllToFd(fd());

    if (output_buffer_.empty()) {
        channel_->disableEvent(EPOLLOUT);
        handle_send_complete_(shared_from_this());
    }
}
void Connection::onClose() {
    disconnected_ = true;
    channel_->unregister();
    handle_close_(shared_from_this());

    // close(fd()); // Socket会自己关闭，不用管它
}
void Connection::onError() {
    disconnected_ = true;
    channel_->unregister();
    handle_error_(shared_from_this());

    // close(fd()); // Socket会自己关闭，不用管它
}