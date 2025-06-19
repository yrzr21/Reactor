#ifndef TIMER_H
#define TIMER_H
#include <sys/timerfd.h>
#include <unistd.h>

#include "../types.h"

// RAII 秒级定时器，可改为纳秒级
class Timer {
   private:
    int timer_fd_;
    struct itimerspec timeout_;

   public:
    Timer();
    ~Timer();

    void start(Seconds initial, Seconds interval);
    void stop();
    int fd();
};

#endif