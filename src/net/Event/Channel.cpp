#include "Channel.h"

Channel::Channel(Eventloop *loop, int fd) : loop_(loop), fd_(fd) {}

// ?
// Channel::~Channel() {
//     if (loop_ && inEpoll_) unregister();
// }

void Channel::handleEvent() {
    if (this->revents_ & EPOLLRDHUP)
        // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
        this->onClose_();
    else if (this->revents_ & (EPOLLIN | EPOLLPRI))
        this->onReadable_();
    else if (this->revents_ & EPOLLOUT)
        this->onWritable_();
    else
        // 其它事件都视为错误
        this->onError_();
}

// void Channel::setinepoll() { inepoll_ = true; }

void Channel::enableEdgeTrigger() { events_ = events_ | EPOLLET; }

// 监视读写事件 EPOLLIN 或 EPOLLOUT
void Channel::enableEvent(uint32_t events) {
    this->events |= events;
    this->loop_->updateChannel(this);
}
void Channel::disableEvent(uint32_t events) {
    if (events & EPOLLIN) this->events &= ~EPOLLIN;
    if (events & EPOLLOUT) this->events &= ~EPOLLOUT;
    this->loop_->updateChannel(this);
}

void Channel::unregister() {
    this->disableEvent(EPOLLIN | EPOLLOUT);
    this->loop_->removeChannel(this);
    this->loop_ = nullptr;
}
bool Channel::isRegistered() { return loop_ != nullptr; }

// ?
// void Channel::diableAll() {
//     this->events_ = 0;
//     this->loop_->updateChannel(this);
// }

int Channel::fd() { return fd_; }
uint32_t Channel::events() { return events_; }
uint32_t Channel::revents() { return revents_; }

void Channel::setRevents(uint32_t revents) { this->revents_ = revents; }
void Channel::setEventHandler(HandlerType type, ChannelCallback fn) {
    switch (type) {
        case HandlerType::Readable:
            this->onReadable_ = fn;
            break;
        case HandlerType::Writable:
            this->onWritable_ = fn;
            break;
        case HandlerType::Closed:
            this->onClose_ = fn;
            break;
        case HandlerType::Error:
            this->onError_ = fn;
            break;
    }
}