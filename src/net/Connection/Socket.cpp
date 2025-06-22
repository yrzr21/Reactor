#include "Socket.h"

void Socket::setOption(int level, int optname, bool on) {
    int optval = static_cast<int>(on);
    if (::setsockopt(fd_, level, optname, &optval, sizeof(optval)) != 0) {
        // TODO: 日志或错误处理
        perror("setOption failed");
        exit(-1);
    }
}

void Socket::setReuseaddr(bool on) { setOption(SOL_SOCKET, SO_REUSEADDR, on); }
void Socket::setReuseport(bool on) { setOption(SOL_SOCKET, SO_REUSEPORT, on); }
void Socket::setTcpnodelay(bool on) { setOption(IPPROTO_TCP, TCP_NODELAY, on); }
void Socket::setKeepalive(bool on) { setOption(SOL_SOCKET, SO_KEEPALIVE, on); }

void Socket::bind(const InetAddress &addr) {
    if (::bind(fd_, (const sockaddr *)addr.addr(), sizeof(sockaddr)) < 0) {
        // TODO: 日志或错误处理
        perror("bind() failed");
        close(fd_);
        exit(-1);
    }

    setIpPort(addr.ip(), addr.port());
}

void Socket::listen(int backlog) {
    if (::listen(fd_, backlog) != 0) {
        perror("listen() failed");
        close(fd_);
        exit(-1);
    }
}

int Socket::accept(InetAddress &clientaddr) {
    sockaddr_in peeraddr;
    socklen_t len = sizeof(peeraddr);
    int clientfd = accept4(fd_, (sockaddr *)&peeraddr, &len, SOCK_NONBLOCK);

    clientaddr.setaddr(peeraddr);

    return clientfd;
}