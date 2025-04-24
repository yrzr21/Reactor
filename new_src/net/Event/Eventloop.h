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
#include "types.h"

// 基于 Epoll 继续封装
class Eventloop {
   private:
    EpollPtr epoll_;
    // 运行的所有连接，仅使用
    ConnectionMap connections_;
    Mutex con_mtx_;

    TimeoutCallback onEpollTimeout_;

    AtomicBool stop_;

    TaskQueue tasks_;
    Mutex mtx_;

    // 异步唤醒事件循环
    eventfd_t efd_;
    ChannelPtr eChannel_;

    // 定时唤醒事件循环，清理空闲连接
    int timer_fd_;
    ChannelPtr timer_channel_;
    time_t maxTimeGap_;      // connection 最长不发生事件的时间间隔
    int heartCycle_;         // 定时器监测周期
    TimerCallback onTimer_;  // 回调TcpServer::removeConnection

    bool isMainloop_;
    pid_t loop_tid;

   public:
    Eventloop(bool isMainloop, int maxGap, int heartCycle);  // 创建 Epoll
    ~Eventloop();

    void run();
    void stop();

    // 把channel中的event加入红黑树中或修改channel
    void updateChannel(Channel *ch);
    void removeChannel(Channel *ch);

    void setEpollTimtoutCallback(std::function<void(Eventloop *)> fn);

    bool isInLoop();  // 判断当前线程是否是I/O线程，即原本的通信事件循环
    void enqueueTask(std::function<void()> task);
    void wakeUp();
    void handleWakeup();

    void handleTimeout();  // timerfd 的读事件回调函数，若超时未
    void newConnection(
        ConnectionPtr connnection);  // 由TcpServer::newconnection
                                     // 调用，添加运行在其中的connection
    void setTimer_cb(std::function<void(int)> fn);
};

#endif  // !EVENTLOOP