#include "Channel.h"

Channel::Channel(Eventloop *loop, int fd) : loop_(loop), fd_(fd) {}

// ?
// Channel::~Channel() {
//     if (loop_ && inEpoll_) unregister();
// }

void Channel::onEvent() {
    if (revents_ & EPOLLRDHUP)
        // 对方已关闭，有些系统检测不到，可以使用EPOLLIN，recv()返回0。
        handle_close_();
    else if (revents_ & (EPOLLIN | EPOLLPRI))
        handle_readable_();
    else if (revents_ & EPOLLOUT)
        handle_writable_();
    else
        // 其它事件都视为错误
        handle_error_();
}

// void Channel::setinepoll() { inepoll_ = true; }

void Channel::enableEdgeTrigger() { events_ = events_ | EPOLLET; }

// 监视读写事件 EPOLLIN 或 EPOLLOUT
void Channel::enableEvent(uint32_t events) {
    events |= events;
    loop_->updateChannel(this);
}
void Channel::disableEvent(uint32_t events) {
    if (events & EPOLLIN) events &= ~EPOLLIN;
    if (events & EPOLLOUT) events &= ~EPOLLOUT;
    loop_->updateChannel(this);
}

void Channel::unregister() {
    disableEvent(EPOLLIN | EPOLLOUT);
    loop_->removeChannel(this);
    loop_ = nullptr;
}
bool Channel::isRegistered() { return loop_ != nullptr; }

// ?
// void Channel::diableAll() {
//     events_ = 0;
//     loop_->updateChannel(this);
// }

int Channel::fd() { return fd_; }
uint32_t Channel::events() { return events_; }
uint32_t Channel::revents() { return revents_; }

void Channel::setRevents(uint32_t revents) { revents_ = revents; }
void Channel::setEventHandler(HandlerType type, OnChannelEvent fn) {
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