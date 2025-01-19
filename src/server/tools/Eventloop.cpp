#include "Eventloop.h"

int createTimerfd(int heartCycle)
{
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    struct itimerspec timeout;                      // alarm周期：5秒0纳秒
    memset(&timeout, 0, sizeof(struct itimerspec)); // 必须清0，不然timerfd无效----鬼知道什么原因
    // struct itimerspec timeout = {5, 0}; // alarm周期：5秒0纳秒
    timeout.it_value.tv_sec = heartCycle; // 定时时间，固定为5，方便测试。
    timeout.it_value.tv_nsec = 0;
    timerfd_settime(tfd, 0, &timeout, 0);
    return tfd;
}

Eventloop::Eventloop(bool isMainloop, int maxGap, int heartCycle)
    : ep_(new Epoll), efd_(eventfd(0, EFD_NONBLOCK)), eChannel_(new Channel(this, efd_)),
      tfd_(createTimerfd(heartCycle)), tChannel_(new Channel(this, tfd_)),
      isMainloop_(isMainloop), maxTimeGap_(maxGap), heartCycle_(heartCycle),
      stop_(false)

{
    this->eChannel_->enablereading(); // LT
    this->eChannel_->setreadcallback(std::bind(&Eventloop::handleWakeup, this));

    this->tChannel_->enablereading(); // 先enable和先set没区别
    this->tChannel_->setreadcallback(std::bind(&Eventloop::handleTimeout, this));
}

Eventloop::~Eventloop()
{
}

void Eventloop::run()
{
    this->loop_tid = syscall(SYS_gettid);
    while (!this->stop_) // 事件循环。
    {
        std::vector<Channel *> rChannels = this->ep_->loop(10 * 1000); // 等待监视的fd有事件发生。
        if (rChannels.size() == 0)
            this->epollTimeout_cb_(this);
        else
        {
            for (auto ch : rChannels)
                ch->handleEvent();
        }
    }
}

void Eventloop::stop()
{
    this->stop_ = true;
    this->wakeUp(); // 将事件循环从 epoll_wait 中唤醒
}

void Eventloop::updateChannel(Channel *ch)
{
    this->ep_->updatechannel(ch);
}

void Eventloop::removeChannel(Channel *ch)
{
    this->ep_->removeChannel(ch);
}

// 判断当前线程是否是I/O线程，即原本的通信事件循环
bool Eventloop::isInLoop()
{
    return syscall(SYS_gettid) == this->loop_tid;
}

void Eventloop::enqueueTask(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lg(this->mtx_);
        this->tasks_.push(task);
    }
    wakeUp();
}

void Eventloop::wakeUp()
{
    eventfd_write(this->efd_, 1); // 写什么都行
}
// efd 读事件handler，执行所有的任务
void Eventloop::handleWakeup()
{
    eventfd_read(this->efd_, 0);

    std::lock_guard<std::mutex> lg(this->mtx_);
    while (!this->tasks_.empty())
    {
        this->tasks_.front()();
        this->tasks_.pop();
    }
}

void Eventloop::handleTimeout()
{
    if (!this->isMainloop_)
    {
        // printf("%ld Eventloop::handleTimeout()：fd  ", syscall(SYS_gettid));
        time_t now = time(0);
        { // lock
            std::lock_guard<std::mutex> lg(this->con_mtx_);
            for (auto iter = this->connections_.begin(); iter != this->connections_.end();)
            {
                if (!iter->second->isTimeout(now, this->maxTimeGap_))
                {
                    iter++;
                    continue;
                }
                this->timer_cb_(iter->first);
                // printf("%d ", iter->second->fd());
                iter = this->connections_.erase(iter);
            }
        }
        // printf("\n");
    }

    struct itimerspec timeout;                      // alarm周期：5秒0纳秒
    memset(&timeout, 0, sizeof(struct itimerspec)); // 必须清0，不然timerfd无效----鬼知道什么原因
    // struct itimerspec timeout = {5, 0}; // alarm周期：5秒0纳秒
    timeout.it_value.tv_sec = this->heartCycle_; // 定时时间，固定为5，方便测试。
    timeout.it_value.tv_nsec = 0;
    timerfd_settime(this->tfd_, 0, &timeout, 0);
}

// 由TcpServer::newconnection 调用，添加运行在其中的connection
void Eventloop::newConnection(conn_sptr connnection)
{
    std::lock_guard<std::mutex> lg(this->con_mtx_);
    this->connections_[connnection->fd()] = connnection;
}
void Eventloop::setTimer_cb(std::function<void(int)> fn)
{
    this->timer_cb_ = fn;
}
void Eventloop::setEpollTimtoutCallback(std::function<void(Eventloop *)> fn)
{
    this->epollTimeout_cb_ = fn;
}
