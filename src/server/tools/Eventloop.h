#include <vector>
#ifndef EVENTLOOP
#define EVENTLOOP
#include "Epoll.h"
#include "Connection.h"
#include <functional>
#include <memory>
#include <unistd.h>
#include <sys/syscall.h> // syscall
#include <queue>
#include <mutex>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <map>
#include <atomic>

class Epoll;   // 需要前置声明
class Channel; // 需要前置声明
class Connection;

using conn_sptr = std::shared_ptr<Connection>;
class Eventloop
{
private:
    std::unique_ptr<Epoll> ep_;
    std::function<void(Eventloop *)> epollTimeout_cb_;
    pid_t loop_tid;
    std::queue<std::function<void()>> tasks_;
    std::mutex mtx_;
    eventfd_t efd_;
    std::unique_ptr<Channel> eChannel_;

    int tfd_;
    std::unique_ptr<Channel> tChannel_;
    bool isMainloop_;
    time_t maxTimeGap_;                 // connection 最长不发生事件的时间间隔
    int heartCycle_;                    // 定时器监测周期
    std::function<void(int)> timer_cb_; // 回调TcpServer::removeConnection

    std::map<int, conn_sptr> connections_; // 运行的所有连接
    std::mutex con_mtx_;

    std::atomic_bool stop_;

public:
    Eventloop(bool isMainloop, int maxGap, int heartCycle); // 创建 Epoll
    ~Eventloop();

    void run();
    void stop();
    void updateChannel(Channel *ch); // 把channel中的event加入红黑树中或修改channel
    void removeChannel(Channel *ch);

    void setEpollTimtoutCallback(std::function<void(Eventloop *)> fn);

    bool isInLoop(); // 判断当前线程是否是I/O线程，即原本的通信事件循环
    void enqueueTask(std::function<void()> task);
    void wakeUp();
    void handleWakeup();

    void handleTimeout();                      // timerfd 的读事件回调函数，若超时未
    void newConnection(conn_sptr connnection); // 由TcpServer::newconnection 调用，添加运行在其中的connection
    void setTimer_cb(std::function<void(int)> fn);
};

#endif // !EVENTLOOP