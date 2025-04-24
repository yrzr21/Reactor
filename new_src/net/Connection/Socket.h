#pragma once
#ifndef SOCKET
#define SOCKET
#include <errno.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <string>

#include "InetAddress.h"

// 封装socket fd，持有一个socket fd
class Socket {
   private:
    const int fd_;
    std::string ip_;
    uint16_t port_;

   private:
    void setOption(int level, int optname, bool on);

   public:
    Socket(int fd);
    ~Socket();

    int fd() const;
    std::string ip() const;
    uint16_t port() const;
    void setIpPort(const std::string &ip, uint16_t port);

    void setReuseaddr(bool on);
    void setReuseport(bool on);
    void setTcpnodelay(bool on);
    void setKeepalive(bool on);

    void bind(const InetAddress &addr);
    void listen(int backlog = 1024);
    int accept(InetAddress &clientaddr);
};

#endif  // !SOCKET
