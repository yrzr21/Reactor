#include "Eventloop.h"
#include <iostream>

Eventloop::Eventloop(bool is_mainloop, int connection_timeout, int heartCycle)
    : is_mainloop_(is_mainloop), connection_timeout_(connection_timeout) {
    event_channel_->setEventHandler(HandlerType::Readable,
                                    [this] { this->onWakeUp(); });
    timer_channel_->setEventHandler(HandlerType::Readable,
                                    [this] { this->onTimer(); });
    timer_.start(Seconds(heartCycle), Seconds(heartCycle));

    event_channel_->enableEvent(EPOLLIN);  // LT
    timer_channel_->enableEvent(EPOLLIN);  // LT
}

void Eventloop::run() {
    loop_tid = syscall(SYS_gettid);

    while (!stop_) {  // 事件循环
        // 这里不用担心 rChannels 被意外清空，因为：
        // 1.单线程占用Epoll 2.不处理完事件就不会再次进入 3.Eventloop 占有 Epoll
        PChannelVector &rChannels = epoll_->loop(10 * 1000);

        if (rChannels.size() != 0) {
            for (auto &ch : rChannels) ch->onEvent();
        } else {
            handle_loop_timeout_(this);
        }
    }
}

void Eventloop::stop() {
    stop_ = true;
    onWakeUp();
}

void Eventloop::controlChannel(int op, Channel *ch) {
    epoll_->controlChannel(op, ch);
}

// 判断当前线程是否是I/O线程，即原本的通信事件循环
bool Eventloop::inIOThread() { return syscall(SYS_gettid) == loop_tid; }

void Eventloop::postTask(Task &&task) {
    {
        MutexGuard guard(task_mtx_);
        tasks_.push(std::move(task));
    }
    wakeupEventloop();
}

void Eventloop::wakeupEventloop() {
    eventfd_write(event_fd_, 1);  // 写什么都行
}

// -- handler --
void Eventloop::onTimer() {
    // printf("onTimer\n");
    if (is_mainloop_) return;

    std::cout << "onTimer" << std::endl;
    // printf("%ld Eventloop::handleTimeout()：fd  ", syscall(SYS_gettid));

    timer_.drain();
    time_t now = time(0);
    IntVector wait_timeout_fds;
    {
        UniqueLock lock(connection_mtx_);
        for (auto [first, second] : connections_) {
            if (!second->isTimeout(now, connection_timeout_)) continue;
            wait_timeout_fds.push_back(first);
        }
        for (auto fd : wait_timeout_fds) connections_.erase(fd);
    }

    handle_timer_(wait_timeout_fds);
}
void Eventloop::onWakeUp() {
    eventfd_read(event_fd_, 0);

    MutexGuard guard(task_mtx_);
    while (!tasks_.empty()) {
        tasks_.front()();
        tasks_.pop();
    }
}

// 由TcpServer::newconnection 调用，添加运行在其中的connection
void Eventloop::registerConnection(ConnectionPtr connnection) {
    MutexGuard guard(connection_mtx_);
    connections_[connnection->fd()] = connnection;
}

// -- setter --
void Eventloop::setTimerHandler(TimerHandler fn) { handle_timer_ = fn; }
void Eventloop::setLoopTimeoutHandler(LoopTimeoutHandler fn) {
    handle_loop_timeout_ = fn;
}
