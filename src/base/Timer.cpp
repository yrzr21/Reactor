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
    timeout_ = {{interval.count(), 0}, {initial.count(), 0}};
    // printf("[DEBUG] timer set: initial = %ld, interval = %ld\n",
    //        timeout_.it_value.tv_sec, timeout_.it_interval.tv_sec);
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

void Timer::drain() {
    uint64_t expirations = 0;
    ssize_t n = read(timer_fd_, &expirations, sizeof(expirations));
    // if (n == sizeof(expirations)) {
    //     printf("[TIMER] %lu expirations\n", expirations);
    // } else {
    //     perror("read timerfd failed");
    // }
}
