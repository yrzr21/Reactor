#include "Eventloop.h"

Eventloop::Eventloop(bool is_mainloop, int connection_timeout, int heartCycle)
    : is_mainloop_(is_mainloop), connection_timeout_(connection_timeout) {
    event_channel_->enableEvent(EPOLLIN);  // LT
    event_channel_->setEventHandler(HandlerType::Readable,
                                    [this] { this->handlePendingTasks(); });

    timer_channel_->enableEvent(EPOLLIN);  // LT
    timer_channel_->setEventHandler(HandlerType::Readable,
                                    [this] { this->handleTimer(); });

    timer_.start(Seconds(heartCycle), Seconds(heartCycle));
}

void Eventloop::run() {
    loop_tid = syscall(SYS_gettid);

    while (!stop_) {  // 事件循环
        std::vector<Channel *> rChannels = epoll_->loop(10 * 1000);

        if (rChannels.size() != 0) {
            for (auto ch : rChannels) ch->handleEvent();  // handle
        } else {
            loop_timeout_callback_(this);  // epoll time out
        }
    }
}

void Eventloop::stop() {
    stop_ = true;
    handlePendingTasks();
}

void Eventloop::controlChannel(EpollOp op, Channel *ch) {
    epoll_->controlChannel(op, ch);
}

// 判断当前线程是否是I/O线程，即原本的通信事件循环
bool Eventloop::inIOLoop() { return syscall(SYS_gettid) == loop_tid; }

void Eventloop::postTask(std::function<void()> task) {
    {
        MutexGuard guard(task_mtx_);
        tasks_.push(task);
    }
    handlePendingTasks();
}

void Eventloop::notifyEventLoop() {
    eventfd_write(event_fd_, 1);  // 写什么都行
}

// -- handler --
void Eventloop::handleTimer() {
    if (is_mainloop_) return;

    time_t now = time(0);
    MutexGuard guard(connection_mtx_);

    // printf("%ld Eventloop::handleTimeout()：fd  ", syscall(SYS_gettid));
    IntVector wait_timeout_fds;
    for (auto [first, second] : connections_) {
        if (!second->isTimeout(now, connection_timeout_)) continue;
        wait_timeout_fds.push_back(first);
    }

    timer_callback_(wait_timeout_fds);

    for (auto fd : wait_timeout_fds) connections_.erase(fd);
    // printf("\n");
}
void Eventloop::handlePendingTasks() {
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
void Eventloop::setTimerCallback(TimerCallback fn) { timer_callback_ = fn; }
void Eventloop::setLoopTimeoutCallback(LoopTimeoutCallback fn) {
    loop_timeout_callback_ = fn;
}
