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
      socket_(newListenfd()),
      channel_(loop, socket_.fd()),
      loop_(loop) {
    socket_.setReuseaddr();
    socket_.setReuseport();
    socket_.setTcpnodelay();
    socket_.setKeepalive();

    socket_.bind(InetAddress{ip, port});

    channel_.enableEvent(EPOLLIN);

    // auto handler = std::bind(&Acceptor::newconnection, this);
    // channel_.setEventHandler(HandlerType::Readable, move(handler));
    // 替代如上写法
    channel_.setEventHandler(HandlerType::Readable,
                             [this] { this->handleNewConnection(); });
}

void Acceptor::listen() { socket_.listen(backlog_); }

void Acceptor::handleNewConnection() {
    InetAddress client_addr;
    int client_fd = socket_.accept(clientaddr);

    SocketPtr client_socket = std::make_unique<Socket>(client_fd);
    client_socket->setIpPort(client_addr.ip(), client_addr.port());

    newConnectionCallback_(std::move(client_socket));
}

void Acceptor::setNewConnectionCallBack(NewConncetionCallBack fn) {
    this->newConnectionCallback_ = fn;
}