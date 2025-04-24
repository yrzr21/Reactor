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

#include "Connection.h"
#include "Epoll.h"
#include "Timer.h"
#include "types.h"

// 基于 Epoll 继续封装
class Eventloop {
   private:
    // -- basic --
    EpollPtr epoll_ = std::make_unique<Epoll>();
    AtomicBool stop_ = false;
    bool is_mainloop_;
    pid_t loop_tid;
    LoopTimeoutCallback loop_timeout_callback_;

    // -- connections --
    ConnectionMap connections_;
    Mutex connection_mtx_;
    time_t connection_timeout_;

    // -- async I/O tasks --
    eventfd_t event_fd_ = eventfd(0, EFD_NONBLOCK);
    ChannelPtr event_channel_ = std::make_unique<Channel>(this, event_fd_);
    TaskQueue tasks_;
    Mutex task_mtx_;

    // -- timer --
    Timer timer_;
    ChannelPtr timer_channel_ = std::make_unique<Channel>(this, timer_.fd());
    TimerCallback timer_callback_;

   public:
    Eventloop(bool is_mainloop, int connection_timeout, int heartCycle);
    ~Eventloop() = default;

    void run();
    void stop();

    void controlChannel(EpollOp op, Channel *ch);
    void registerConnection(ConnectionPtr connnection);

    bool inIOLoop();

    void postTask(Task task);
    void notifyEventLoop();  // notify Eventloop to handle pending tasks

    // -- handler --
    void handleTimer();
    void handlePendingTasks();

    // -- setter --
    void setTimerCallback(TimerCallback fn);
    void setLoopTimeoutCallback(LoopTimeoutCallback fn);
};

#endif  // !EVENTLOOP