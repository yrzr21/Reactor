#include "Channel.h"

/*
class Channel
{
private:
    int fd_=-1;                             // Channel拥有的fd，Channel和fd是一对一的关系。
    Epoll *ep_=nullptr;                // Channel对应的红黑树，Channel与Epoll是多对一的关系，一个Channel只对应一个Epoll。
    bool inepoll_=false;              // Channel是否已添加到epoll树上，如果未添加，调用epoll_ctl()的时候用EPOLL_CTL_ADD，否则用EPOLL_CTL_MOD。
    uint32_t events_=0;              // fd_需要监视的事件。listenfd和clientfd需要监视EPOLLIN，clientfd还可能需要监视EPOLLOUT。
    uint32_t revents_=0;             // fd_已发生的事件。

public:
    Channel(Epoll* ep,int fd);      // 构造函数。
    ~Channel();                           // 析构函数。

    int fd();                                            // 返回fd_成员。
    void useet();                                    // 采用边缘触发。
    void enablereading();                     // 让epoll_wait()监视fd_的读事件。
    void setinepoll();;                           // 把inepoll_成员的值设置为true。
    void setrevents(uint32_t ev);         // 设置revents_成员的值为参数ev。
    bool inpoll();                                  // 返回inepoll_成员。
    uint32_t events();                           // 返回events_成员。
    uint32_t revents();                          // 返回revents_成员。
};*/

Channel::Channel(Epoll *ep, int fd) : ep_(ep), fd_(fd) // 构造函数。
{
}

Channel::~Channel() // 析构函数。
{
    // 在析构函数中，不要销毁ep_，也不能关闭fd_，因为这两个东西不属于Channel类，Channel类只是需要它们，使用它们而已。
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
    ep_->updatechannel(this);
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