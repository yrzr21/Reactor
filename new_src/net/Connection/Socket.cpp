#include "Socket.h"

Socket::Socket(int fd) : fd_(fd) {}
Socket::~Socket() { ::close(this->fd_); }

int Socket::fd() const { return this->fd_; }
std::string Socket::ip() const { return this->ip_; }
uint16_t Socket::port() const { return this->port_; }
void Socket::setIpPort(const std::string &ip, uint16_t port) {
    this->ip_ = ip;
    this->port_ = port;
}

void Socket::setOption(int level, int optname, bool on) {
    int optval = static_cast<int>(on);
    if (::setsockopt(this->fd_, level, optname, &optval, sizeof(optval)) != 0) {
        // TODO: 日志或错误处理
        return false;
    }
    return true;
}
bool setReuseAddr(bool on) { return setOption(SOL_SOCKET, SO_REUSEADDR, on); }
bool setReusePort(bool on) { return setOption(SOL_SOCKET, SO_REUSEPORT, on); }
bool setTcpNoDelay(bool on) { return setOption(IPPROTO_TCP, TCP_NODELAY, on); }
bool setKeepAlive(bool on) { return setOption(SOL_SOCKET, SO_KEEPALIVE, on); }

void Socket::bind(const InetAddress &addr) {
    if (::bind(this->fd_, addr.addr(), sizeof(sockaddr)) < 0) {
        // TODO: 日志或错误处理
        perror("bind() failed");
        close(this->fd_);
        exit(-1);
    }

    this->setIpPort(addr.ip(), addr.port());
}

void Socket::listen(int backlog) {
    if (::listen(this->fd_, backlog) != 0) {
        perror("listen() failed");
        close(this->fd_);
        exit(-1);
    }
}

int Socket::accept(InetAddress &clientaddr) {
    sockaddr_in peeraddr;
    socklen_t len = sizeof(peeraddr);
    int clientfd = accept4(this->fd_, (sockaddr *)&peeraddr, &len, SOCK_NONBLOCK);

    clientaddr.setaddr(peeraddr);

    return clientfd;
}