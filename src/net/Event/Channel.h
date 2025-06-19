#pragma once
#ifndef CHANNEL
#define CHANNEL

#include <sys/epoll.h>

#include <functional>

#include "../types.h"
#include "Connection.h"
#include "Eventloop.h"
#include "InetAddress.h"
#include "Socket.h"

// 封装事件。使用fd和loop
class Channel {
   private:
    int fd_ = -1;
    Eventloop *loop_ = nullptr;
    uint32_t events_ = 0;
    uint32_t revents_ = 0;  // returned events

    // call back
    ChannelEventHandler handle_readable_;
    ChannelEventHandler handle_writable_;
    ChannelEventHandler handle_close_;
    ChannelEventHandler handle_error_;

   public:
    Channel(Eventloop *loop, int fd);
    ~Channel() = default;
    Channel(const Channel &) = delete;
    Channel &operator=(const Channel &) = delete;

    void onEvent();

    void enableEdgeTrigger();
    void enableEvent(uint32_t events);
    void disableEvent(uint32_t events);
    void diableAll();

    void unregister();
    bool isRegistered();

    // void setinepoll();
    // diableAll

    int fd();
    uint32_t revents();
    uint32_t events();

    void setRevents(uint32_t revents);
    void setEventHandler(HandlerType type, ChannelEventHandler fn);
};

#endif