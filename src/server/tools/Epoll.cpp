#include "Epoll.h"

Epoll::Epoll()
{
    if ((epollfd_ = epoll_create(1)) == -1) // 创建epoll句柄（红黑树）。
    {
        printf("epoll_create() failed(%d).\n", errno);
        exit(-1);
    }
}

Epoll::~Epoll()
{
    close(epollfd_); // 在析构函数中关闭epollfd_。
}

// // 把fd和它需要监视的事件添加到红黑树上。
// void Epoll::addfd(int fd, uint32_t op)
// {
//     epoll_event ev;  // 声明事件的数据结构。
//     ev.data.fd = fd; // 指定事件的自定义数据，会随着epoll_wait()返回的事件一并返回。
//     ev.events = op;  // 让epoll监视fd的。

//     if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &ev) == -1) // 把需要监视的fd和它的事件加入epollfd中。
//     {
//         printf("epoll_ctl() failed(%d).\n", errno);
//         exit(-1);
//     }
// }

// 把channel添加/更新到红黑树上，channel中有fd，也有需要监视的事件。
void Epoll::updatechannel(Channel *ch)
{
    epoll_event ev;           // 声明事件的数据结构。
    ev.data.ptr = ch;         // 指定channel。
    ev.events = ch->events(); // 指定事件。

    if (ch->inpoll()) // 如果channel已经在树上了。
    {
        if (epoll_ctl(epollfd_, EPOLL_CTL_MOD, ch->fd(), &ev) == -1)
        {
            perror("epoll_ctl() failed.\n");
            exit(-1);
        }
    }
    else // 如果channel不在树上。
    {
        if (epoll_ctl(epollfd_, EPOLL_CTL_ADD, ch->fd(), &ev) == -1)
        {
            perror("epoll_ctl() failed.\n");
            exit(-1);
        }
        ch->setinepoll(); // 把channel的inepoll_成员设置为true。
    }
}

void Epoll::removeChannel(Channel *ch)
{
    printf("remove Channel\n");
    sleep(1);
    if (!ch->inpoll())
        return;

    if (epoll_ctl(epollfd_, EPOLL_CTL_DEL, ch->fd(), 0) == -1)
    {
        perror("epoll_ctl() failed.\n");
        exit(-1);
    }
}

// 运行epoll_wait()，等待事件的发生，已发生的事件用vector容器返回。
std::vector<Channel *> Epoll::loop(int timeout)
{
    std::vector<Channel *> rChannels; // 存放epoll_wait()返回的事件。

    bzero(events_, sizeof(events_));
    int infds = epoll_wait(epollfd_, events_, MaxEvents, timeout); // 等待监视的fd有事件发生。

    if (infds < 0) // 失败
    {
        perror("epoll_wait() failed");
        exit(-1);
    }
    else if (infds == 0) // 超时
        return rChannels;

    // 如果infds>0，表示有事件发生的fd的数量。
    for (int ii = 0; ii < infds; ii++) // 事件
    {
        Channel *ch = (Channel *)events_[ii].data.ptr;
        ch->setrevents(events_[ii].events);
        rChannels.push_back(ch);
    }

    return rChannels;
}