#pragma once
#ifndef INETADDRESS
#define INETADDRESS
#include <arpa/inet.h>
#include <netinet/in.h>

#include <string>

// 封装用于网络通信的 ip 和端口，持有一个sockaddr_in
class InetAddress {
   private:
    sockaddr_in addr_;

   public:
    InetAddress();
    InetAddress(const std::string &ip, uint16_t port);
    InetAddress(const sockaddr_in addr);
    ~InetAddress();

    const char *ip() const;
    uint16_t port() const;

    // const sockaddr *addr() const;
    const sockaddr_in *addr() const;
    void setaddr(sockaddr_in clientaddr);
};

#endif  // !INETADDRESS
