#include "Eventloop.h"

Eventloop::Eventloop()
    : ep_(new Epoll), efd_(eventfd(0, EFD_NONBLOCK)), eChannel_(new Channel(this, efd_))
{
    this->eChannel_->enablereading(); // LT
    this->eChannel_->setreadcallback(std::bind(&Eventloop::handleWakeup, this));
}

Eventloop::~Eventloop()
{
}

void Eventloop::run()
{
    this->loop_tid = syscall(SYS_gettid);
    while (true) // 事件循环。
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

void Eventloop::setEpollTimtoutCallback(std::function<void(Eventloop *)> fn)
{
    this->epollTimeout_cb_ = fn;
}
