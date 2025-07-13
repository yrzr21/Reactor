#include "Channel.h"

Channel::Channel(Eventloop *loop, int fd) : loop_(loop), fd_(fd) {
    loop->controlChannel(EPOLL_CTL_ADD, this);
}

// ?
// Channel::~Channel() {
//     if (loop_ && inEpoll_) unregister();
// }

// void Channel::setinepoll() { inepoll_ = true; }

void Channel::enableEdgeTrigger() { events_ = events_ | EPOLLET; }

// 监视读写事件 EPOLLIN 或 EPOLLOUT
void Channel::enableEvent(uint32_t events) {
    if (events_ & events) return;
    events_ |= events;
    loop_->controlChannel(EPOLL_CTL_MOD, this);
}
void Channel::disableEvent(uint32_t events) {
    if (!(events_ & events)) return;
    events_ &= ~events;
    loop_->controlChannel(EPOLL_CTL_MOD, this);
}

void Channel::diableAll() {
    events_ = 0;
    loop_->controlChannel(EPOLL_CTL_MOD, this);
}

void Channel::unregister() {
    disableEvent(events_);
    loop_->controlChannel(EPOLL_CTL_DEL, this);
    loop_ = nullptr;
}

void Channel::setEventHandler(HandlerType type, ChannelEventHandler fn) {
    switch (type) {
        case HandlerType::Readable:
            handle_readable_ = fn;
            break;
        case HandlerType::Writable:
            handle_writable_ = fn;
            break;
        case HandlerType::Closed:
            handle_close_ = fn;
            break;
        case HandlerType::Error:
            handle_error_ = fn;
            break;
    }
}