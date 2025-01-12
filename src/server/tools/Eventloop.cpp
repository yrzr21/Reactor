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
        std::vector<Channel *> rChannels = this->ep_->loop(); // 等待监视的fd有事件发生。
        for (auto ch : rChannels)
            ch->handleEvent();
    }
}

Epoll *Eventloop::ep()
{
    return this->ep_;
}
