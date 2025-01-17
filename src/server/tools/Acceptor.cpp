#include "Acceptor.h"

Acceptor::Acceptor(const std::string &ip, uint16_t port, Eventloop *loop, int maxN)
    : maxN_(maxN), loop_(loop), acceptSocket(createnonblocking()), acceptChannel(loop, this->acceptSocket.fd())
{
    this->acceptSocket.setReuseaddr(true), acceptSocket.setTcpnodelay(true), acceptSocket.setReuseport(true), acceptSocket.setKeepalive(true);
    InetAddress servaddr(ip, port); // 服务端的地址和协议。
    this->acceptSocket.bind(servaddr);

    this->acceptChannel.setreadcallback( // 注册读事件handler, 提前绑定默认参数
        std::bind(&Acceptor::newconnection, this));
    this->acceptChannel.enablereading(); // 监视读事件，加入到epoll
}

Acceptor::~Acceptor() {}

void Acceptor::listen()
{
    this->acceptSocket.listen(this->maxN_);
}

// 处理新客户端连接请求。
void Acceptor::newconnection()
{
    InetAddress clientaddr; // 客户端的地址和协议。
    // 注意，clientsock只能new出来，不能在栈上，否则析构函数会关闭fd。
    // 还有，这里new出来的对象没有释放，这个问题以后再解决。
    std::unique_ptr<Socket> clientsock(new Socket(this->acceptSocket.accept(clientaddr))); // 被 clientConnector 管理
    clientsock->setIpPort(clientaddr.ip(), clientaddr.port());

    this->newConnection_cb_(std::move(clientsock));
}

void Acceptor::setNewConnection_cb(std::function<void(std::unique_ptr<Socket>)> fn)
{
    this->newConnection_cb_ = fn;
}
