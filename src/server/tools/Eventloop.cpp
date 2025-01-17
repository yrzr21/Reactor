#include "Eventloop.h"

Eventloop::Eventloop() : ep_(new Epoll)
{
}

Eventloop::~Eventloop()
{
    if (ep_ != nullptr)
        delete ep_;
}

void Eventloop::run()
{
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

void Eventloop::setEpollTimtoutCallback(std::function<void(Eventloop *)> fn)
{
    this->epollTimeout_cb_ = fn;
}
