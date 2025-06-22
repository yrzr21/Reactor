#pragma once
#ifndef EPOLL
#define EPOLL

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/epoll.h>
#include <unistd.h>

#include <vector>

#include "../../types.h"
#include "Channel.h"

// 封装 epoll 监视事件
// MAX_EVENTS：maximum number of events to be returned
class Epoll {
   private:
    int epoll_fd_ = -1;
    static constexpr int MAX_EVENTS = 100;
    epoll_event events_[MAX_EVENTS];
    PChannelVector active_channels_;

   public:
    Epoll();
    ~Epoll();

    // 禁用拷贝，允许移动
    Epoll(const Epoll&) = delete;
    Epoll& operator=(const Epoll&) = delete;
    Epoll(Epoll&&) = default;
    Epoll& operator=(Epoll&&) = default;

    // 对 channel 进行增删改
    void controlChannel(int op, Channel* channel);

    // 监听，返回发生的事件
    const PChannelVector& loop(int timeout = -1);
};

#endif  // !EPOLL
