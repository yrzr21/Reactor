#include "./Epoll.h"

template <int MAX_EVENTS>
Epoll<MAX_EVENTS>::Epoll() {
    if ((this->epoll_fd_ = epoll_create(1)) == -1) {
        printf("epoll_create() failed(%d).\n", errno);
        exit(-1);
    }
}

template <int MAX_EVENTS>
Epoll<MAX_EVENTS>::~Epoll() {
    close(this->epoll_fd_);
}

template <int MAX_EVENTS>
inline void Epoll<MAX_EVENTS>::control(EpollOp op, Channel *channel) {
    epoll_event ev{.data = {.ptr = ch}, .events = ch->events()};

    int cmd = static_cast<int>(op);
    epoll_event *pev = (cmd == EPOLL_CTL_DEL ? nullptr : &ev);

    if (::epoll_ctl(epollfd_, cmd, ch->fd(), pev) == -1) {
        // todo: 更合理的错误处理
        perror("epoll_ctl() failed.\n");
        exit(-1);
    }
}

// 监听，返回发生的事件
template <int MAX_EVENTS>
std::vector<Channel *> Epoll<MAX_EVENTS>::loop(int timeout) {
    // bzero(this->events_, sizeof(this->events_));

    int n = epoll_wait(epollfd_, this->events_, MAX_EVENTS, timeout);
    if (n < 0) {
        perror("epoll_wait() failed");
        exit(-1);
    } else if (n == 0) {
        // todo: 超时
    }

    // > 0
    std::vector<Channel *> active_channels;
    for (int ii = 0; ii < n; ii++) {
        Channel *ch = static_cast<Channel *>(this->events_[ii].data.ptr);
        ch->setRevents(this->events_[ii].events);

        active_channels.push_back(ch);
    }
    return active_channels;  // NRVO
}
