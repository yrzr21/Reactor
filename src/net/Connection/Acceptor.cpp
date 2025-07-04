#include "Acceptor.h"

int newListenFd() {
    int listenfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
    if (listenfd < 0) {
        // perror("socket() failed"); exit(-1);
        printf("%s:%s:%d listen socket create error:%d\n", __FILE__,
               __FUNCTION__, __LINE__, errno);
        exit(-1);
    }
    return listenfd;
}

Acceptor::Acceptor(const std::string &ip, uint16_t port, Eventloop *loop,
                   int backlog)
    : backlog_(backlog),
      socket_(newListenFd()),
      channel_(loop, socket_.fd()),
      loop_(loop) {
    // socket properties
    socket_.setReuseaddr(true);
    socket_.setReuseport(true);
    socket_.setTcpnodelay(true);
    socket_.setKeepalive(true);

    socket_.bind(InetAddress{ip, port});

    channel_.setEventHandler(HandlerType::Readable,
                             [this] { this->onNewConnection(); });
}

void Acceptor::listen() {
    channel_.enableEvent(EPOLLIN);
    socket_.listen(backlog_);
}

void Acceptor::onNewConnection() {
    InetAddress client_addr;
    int client_fd = socket_.accept(client_addr);

    SocketPtr client_socket = std::make_unique<Socket>(client_fd);
    client_socket->setIpPort(client_addr.ip(), client_addr.port());

    handle_new_connection_(std::move(client_socket));
}

void Acceptor::setNewConnectionHandler(NewConnectionHandler fn) {
    handle_new_connection_ = fn;
}