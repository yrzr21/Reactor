#include "./Epoll.h"

Epoll::Epoll() {
    if ((epoll_fd_ = epoll_create(1)) == -1) {
        printf("epoll_create() failed(%d).\n", errno);
        exit(-1);
    }
    // 分配内存但不构造元素
    active_channels_.reserve(MAX_EVENTS);
}

Epoll::~Epoll() { close(epoll_fd_); }

void Epoll::controlChannel(int op, Channel *channel) {
    epoll_event ev{.events = channel->events(), .data = {.ptr = channel}};
    epoll_event *pev = (op == EPOLL_CTL_DEL ? nullptr : &ev);

    if (::epoll_ctl(epoll_fd_, op, channel->fd(), pev) == -1) {
        // todo: 更合理的错误处理
        printf("epoll_ctl() failed. op=%d,ev=%d,epoll_fd_=%d,channel fd=%d\n",
               op, pev, epoll_fd_, channel->fd());
        exit(-1);
    }
}

// 监听，返回发生的事件
// 再次进入时清空原 vector
const PChannelVector &Epoll::loop(int timeout) {
    // bzero(events_, sizeof(events_));
    active_channels_.clear();

    int n = epoll_wait(epoll_fd_, events_, MAX_EVENTS, timeout);
    // printf("Epoll: %d events\n", n);
    if (n < 0) [[unlikely]] {
        perror("epoll_wait() failed");
        exit(-1);
    }
    // 超时由 eventloop 负责处理

    // > 0
    for (int i = 0; i < n; i++) {
        Channel *ch = static_cast<Channel *>(events_[i].data.ptr);
        // printf("fd %d event ", ch->fd());
        // if (events_[i].events & EPOLLIN) printf("EPOLLIN ");
        // if (events_[i].events & EPOLLOUT) printf("fd %d EPOLLOUT", ch->fd());
        // if (events_[i].events & EPOLLRDHUP) printf("EPOLLRDHUP ");
        // printf("\n");

        ch->setRevents(events_[i].events);

        active_channels_.push_back(ch);
    }
    return active_channels_;
}
