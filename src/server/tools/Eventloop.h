#include <vector>
#ifndef EVENTLOOP
#define EVENTLOOP
#include "Epoll.h"
#include <functional>
#include <memory>
#include <unistd.h>
#include <sys/syscall.h> // syscall
#include <queue>
#include <mutex>
#include <sys/eventfd.h>

class Epoll;   // 需要前置声明
class Channel; // 需要前置声明

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

public:
    Eventloop(); // 创建 Epoll
    ~Eventloop();

    void run();
    void updateChannel(Channel *ch); // 把channel中的event加入红黑树中或修改channel
    void removeChannel(Channel *ch);

    void setEpollTimtoutCallback(std::function<void(Eventloop *)> fn);

    bool isInLoop(); // 判断当前线程是否是I/O线程，即原本的通信事件循环
    void enqueueTask(std::function<void()> task);
    void wakeUp();
    void handleWakeup();
};

#endif // !EVENTLOOP