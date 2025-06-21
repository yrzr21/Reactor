#include <vector>
#ifndef EVENTLOOP
#define EVENTLOOP
#include <sys/eventfd.h>
#include <sys/syscall.h>  // syscall
#include <sys/timerfd.h>
#include <unistd.h>

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>

#include "../../types.h"
#include "../Connection/Connection.h"
#include "Epoll.h"
#include "../../base/Timer.h"

// 基于 Epoll 继续封装
// 用于异步 IO 任务
class Eventloop {
   private:
    // -- basic --
    EpollPtr epoll_ = std::make_unique<Epoll>();
    AtomicBool stop_ = false;
    bool is_mainloop_;
    pid_t loop_tid;
    LoopTimeoutHandler handle_loop_timeout_;

    // -- connections --
    ConnectionMap connections_;
    Mutex connection_mtx_;
    time_t connection_timeout_;

    // -- async I/O tasks --
    eventfd_t event_fd_ = eventfd(0, EFD_NONBLOCK);  // 注册到事件循环用于唤醒它
    ChannelPtr event_channel_ = std::make_unique<Channel>(this, event_fd_);
    TaskQueue tasks_;
    Mutex task_mtx_;

    // -- timer --
    Timer timer_;
    ChannelPtr timer_channel_ = std::make_unique<Channel>(this, timer_.fd());
    TimerHandler handle_timer_;

   public:
    Eventloop(bool is_mainloop, int connection_timeout, int heartCycle);
    ~Eventloop() = default;

    void run();
    void stop();

    void controlChannel(int op, Channel* ch);
    void registerConnection(ConnectionPtr connnection);

    bool inIOThread();

    void postTask(Task&& task);
    void wakeupEventloop();  // 唤醒事件循环，它苏醒后会去执行异步任务
    void onWakeUp();         // 苏醒后处理异步任务

    void onTimer();  // 定期苏醒，处理空闲连接
    void stopTimer();

    // -- setter --
    void setTimerHandler(TimerHandler fn);
    void setLoopTimeoutHandler(LoopTimeoutHandler fn);
};

#endif  // !EVENTLOOP