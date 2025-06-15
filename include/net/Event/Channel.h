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

// todo: 对外接口，隐藏 epoll 细节
// enum class MonitorEvent : uint32_t {
//     None = 0,
//     Read = EPOLLIN,
//     Write = EPOLLOUT,
//     Closed = EPOLLRDHUP,
//     Error = EPOLLERR,
//     All = Read | Write | Closed | Error
// };

// cast MonitorEvent
// inline MonitorEvent operator|(MonitorEvent a, MonitorEvent b) {
//     return static_cast<MonitorEvent>(static_cast<uint32_t>(a) |
//                                      static_cast<uint32_t>(b));
// }
// inline MonitorEvent operator&(MonitorEvent a, MonitorEvent b) {
//     return static_cast<MonitorEvent>(static_cast<uint32_t>(a) &
//                                      static_cast<uint32_t>(b));
// }
// inline MonitorEvent operator~(MonitorEvent a) {
//     return static_cast<MonitorEvent>(~static_cast<uint32_t>(a));
// }

// 封装事件。使用fd和loop
class Channel {
   private:
    int fd_ = -1;
    Eventloop *loop_ = nullptr;
    uint32_t events_ = 0;
    uint32_t revents_ = 0;  // returned events

    // call back
    OnChannelEvent handle_readable_;
    OnChannelEvent handle_writable_;
    OnChannelEvent handle_close_;
    OnChannelEvent handle_error_;

   public:
    Channel(Eventloop *loop, int fd);
    ~Channel() = default;
    Channel(const Channel &) = delete;
    Channel &operator=(const Channel &) = delete;

    void onEvent();

    void enableEdgeTrigger();
    void enableEvent(uint32_t events);
    void disableEvent(uint32_t events);
    void unregister();
    bool isRegistered();

    // void setinepoll();
    // diableAll

    int fd() const;
    uint32_t revents() const;
    uint32_t events() const;

    void setRevents(uint32_t revents);
    void setEventHandler(HandlerType type, OnChannelEvent fn);
};

#endif