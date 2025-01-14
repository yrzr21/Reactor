#include "Channel.h"

Channel::Channel(Eventloop *loop, int fd) : loop_(loop), fd_(fd) // 构造函数。
{
}

Channel::~Channel() // 析构函数。
{
    // 在析构函数中，不要销毁loop_，也不能关闭fd_，因为这两个东西不属于Channel类，Channel类只是需要它们，使用它们而已。
}

void Channel::handleEvent()
{
    if (this->revents_ & EPOLLRDHUP) // 对方已关闭，有些系统检测不到，可以使用 EPOLLIN，recv()返回0。
        this->closeCallback_();
    else if (this->revents_ & (EPOLLIN | EPOLLPRI)) //  普通数据  带外数据
        this->readCallback_();                      // 接收缓冲区中有数据可以读。
    else if (this->revents_ & EPOLLOUT)             // 发送缓冲区变为可写状态
        this->onWritableCallback_();
    else // 其它事件，都视为错误。
        this->errorCallback_();
}

// 注册 fd 读事件处理函数，回调
void Channel::setreadcallback(std::function<void()> fn)
{
    this->readCallback_ = fn;
}

void Channel::setWritablecallback(std::function<void()> fn)
{
    this->onWritableCallback_ = fn;
}

void Channel::setClosecallback(std::function<void()> fn)
{
    this->closeCallback_ = fn;
}

void Channel::setErrorcallback(std::function<void()> fn)
{
    this->errorCallback_ = fn;
}

int Channel::fd() // 返回fd_成员。
{
    return fd_;
}

void Channel::useet() // 采用边缘触发。
{
    events_ = events_ | EPOLLET;
}

void Channel::enablereading() // 让epoll_wait()监视fd_的读事件。
{
    events_ |= EPOLLIN;
    loop_->updateChannel(this);
}

void Channel::disableReading()
{
    events_ &= ~EPOLLIN;
    loop_->updateChannel(this);
}

void Channel::enableWriting()
{
    events_ |= EPOLLOUT;
    loop_->updateChannel(this);
}

void Channel::disableWriting()
{
    events_ &= ~EPOLLOUT;
    loop_->updateChannel(this);
}

void Channel::setinepoll() // 把inepoll_成员的值设置为true。
{
    inepoll_ = true;
}

void Channel::setrevents(uint32_t ev) // 设置revents_成员的值为参数ev。
{
    revents_ = ev;
}

bool Channel::inpoll() // 返回inepoll_成员。
{
    return inepoll_;
}

uint32_t Channel::events() // 返回events_成员。
{
    return events_;
}

uint32_t Channel::revents() // 返回revents_成员。
{
    return revents_;
}