#include "./Epoll.h"

template <int MAX_EVENTS>
Epoll<MAX_EVENTS>::Epoll() {
    if ((epoll_fd_ = epoll_create(1)) == -1) {
        printf("epoll_create() failed(%d).\n", errno);
        exit(-1);
    }
}

template <int MAX_EVENTS>
Epoll<MAX_EVENTS>::~Epoll() {
    close(epoll_fd_);
}

template <int MAX_EVENTS>
inline void Epoll<MAX_EVENTS>::controlChannel(EpollOp op, Channel *channel) {
    epoll_event ev{.data = {.ptr = channel}, .events = channel->events()};
    epoll_event *pev = (op == EpollOp::Del ? nullptr : &ev);

    int cmd = static_cast<int>(op);  // 转回原宏
    if (::epoll_ctl(epollfd_, cmd, channel->fd(), pev) == -1) {
        // todo: 更合理的错误处理
        perror("epoll_ctl() failed.\n");
        exit(-1);
    }
}

// 监听，返回发生的事件
template <int MAX_EVENTS>
std::vector<Channel *> Epoll<MAX_EVENTS>::loop(int timeout) {
    // bzero(events_, sizeof(events_));

    int n = epoll_wait(epollfd_, events_, MAX_EVENTS, timeout);
    if (n < 0) {
        perror("epoll_wait() failed");
        exit(-1);
    }
    // 超时由 eventloop 负责处理

    // > 0
    std::vector<Channel *> active_channels;
    for (int i = 0; i < n; i++) {
        Channel *ch = static_cast<Channel *>(events_[i].data.ptr);
        ch->setRevents(events_[i].events);

        active_channels.push_back(ch);
    }
    return active_channels;  // NRVO
}
