#pragma once
#ifndef CHANNEL
#define CHANNEL

#include <sys/epoll.h>
#include <functional>
#include "InetAddress.h"
#include "Socket.h"
#include "Eventloop.h"

class Eventloop;

class Channel
{
private:
    int fd_ = -1;           // Channel拥有的fd，Channel和fd是一对一的关系。

    Eventloop* loop_ = nullptr;
    // Epoll *ep_ = nullptr;   // Channel对应的红黑树，Channel与Epoll是多对一的关系，一个Channel只对应一个Epoll。
    bool inepoll_ = false;  // Channel是否已添加到epoll树上，如果未添加，调用epoll_ctl()的时候用EPOLL_CTL_ADD，否则用EPOLL_CTL_MOD。
    uint32_t events_ = 0;   // fd_需要监视的事件。listenfd和clientfd需要监视EPOLLIN，clientfd还可能需要监视EPOLLOUT。
    uint32_t revents_ = 0;  // fd_已发生的事件。

    std::function<void()> readCallback_; // fd_读事件的回调函数。

public:
    Channel(Eventloop* loop, int fd); // 构造函数。
    ~Channel();                                // 析构函数。

    void handleEvent(); // 处理事件，revents_ 由 Epoll::loop 设置

    void setreadcallback(std::function<void()> fn); // 注册 fd 读事件处理函数，回调
    // 这两个函数用于注册读事件 handler
    void newconnection(Socket *servsock);           // 读事件，lisen fd 新连接
    void onmessage();                               // 读事件，客户端发消息

    // 返回与设置成员
    int fd();                     // 返回fd_成员。
    void useet();                 // 采用边缘触发。
    void enablereading();         // 让epoll_wait()监视fd_的读事件。
    void setinepoll();            // 把inepoll_成员的值设置为true。
    void setrevents(uint32_t ev); // 设置revents_成员的值为参数ev。
    bool inpoll();                // 返回inepoll_成员。
    uint32_t events();            // 返回events_成员。
    uint32_t revents();           // 返回revents_成员。
};

#endif