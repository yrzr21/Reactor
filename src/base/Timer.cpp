#include "Timer.h"

Timer::Timer() {
    timer_fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timer_fd_ == -1) {
        // todo: 更合理的错误处理
        perror("timerfd_create failed");
        exit(-1);
    }
}
Timer::~Timer() { close(timer_fd_); }

void Timer::start(Seconds initial, Seconds interval) {
    timeout_ = {{initial_sec.count(), 0}, {interval_sec.count(), 0}};
    if (timerfd_settime(timer_fd_, 0, &timeout_, nullptr) == -1) {
        // todo: 更合理的错误处理
        perror("timerfd_settime failed");
        exit(-1);
    }
}

void Timer::stop() {
    timeout_ = {};
    timerfd_settime(timer_fd_, 0, &timeout_, nullptr);
}

int Timer::fd() { return timer_fd_; }