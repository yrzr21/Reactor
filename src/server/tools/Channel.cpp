#include "Channel.h"

/*
class Channel
{
private:
    int fd_=-1;                             // Channel拥有的fd，Channel和fd是一对一的关系。
    Epoll *ep_=nullptr;                // Channel对应的红黑树，Channel与Epoll是多对一的关系，一个Channel只对应一个Epoll。
    bool inepoll_=false;              // Channel是否已添加到epoll树上，如果未添加，调用epoll_ctl()的时候用EPOLL_CTL_ADD，否则用EPOLL_CTL_MOD。
    uint32_t events_=0;              // fd_需要监视的事件。listenfd和clientfd需要监视EPOLLIN，clientfd还可能需要监视EPOLLOUT。
    uint32_t revents_=0;             // fd_已发生的事件。

public:
    Channel(Epoll* ep,int fd);      // 构造函数。
    ~Channel();                           // 析构函数。

    int fd();                                            // 返回fd_成员。
    void useet();                                    // 采用边缘触发。
    void enablereading();                     // 让epoll_wait()监视fd_的读事件。
    void setinepoll();;                           // 把inepoll_成员的值设置为true。
    void setrevents(uint32_t ev);         // 设置revents_成员的值为参数ev。
    bool inpoll();                                  // 返回inepoll_成员。
    uint32_t events();                           // 返回events_成员。
    uint32_t revents();                          // 返回revents_成员。
};*/

Channel::Channel(Epoll *ep, int fd) : ep_(ep), fd_(fd) // 构造函数。
{
}

Channel::~Channel() // 析构函数。
{
    // 在析构函数中，不要销毁ep_，也不能关闭fd_，因为这两个东西不属于Channel类，Channel类只是需要它们，使用它们而已。
}

void Channel::handleEvent()
{
    if (this->revents_ & EPOLLRDHUP) // 对方已关闭，有些系统检测不到，可以使用 EPOLLIN，recv()返回0。
    {
        printf("client(eventfd=%d) disconnected.\n", this->fd_);
        close(this->fd_); // 关闭客户端的fd。
    } //  普通数据  带外数据
    else if (this->revents_ & (EPOLLIN | EPOLLPRI))
        this->readCallback_();          // 接收缓冲区中有数据可以读。
    else if (this->revents_ & EPOLLOUT) // 有数据需要写，暂时没有代码，以后再说。
    {
    }
    else // 其它事件，都视为错误。
    {
        printf("client(eventfd=%d) error.\n", this->fd_);
        close(this->fd_); // 关闭客户端的fd。
    }
}

// listen fd 的函数
// 处理新客户端连接请求。
void Channel::newconnection(Socket *servsock)
{
    InetAddress clientaddr; // 客户端的地址和协议。
    // 注意，clientsock只能new出来，不能在栈上，否则析构函数会关闭fd。
    // 还有，这里new出来的对象没有释放，这个问题以后再解决。
    Socket *clientsock = new Socket(servsock->accept(clientaddr));

    Channel *peerChannel = new Channel(this->ep_, clientsock->fd());
    peerChannel->enablereading(), peerChannel->useet(); // 监视读，边缘触发
    peerChannel->setreadcallback(std::bind(&Channel::onmessage, peerChannel));

    this->ep_->updatechannel(peerChannel);

    printf("accept client(fd=%d,ip=%s,port=%d) ok.\n", clientsock->fd(), clientaddr.ip(), clientaddr.port());
}

void Channel::onmessage()
{
    char buffer[1024];
    while (true) // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
    {
        bzero(&buffer, sizeof(buffer));
        ssize_t nread = read(this->fd_, buffer, sizeof(buffer));
        if (nread > 0) // 成功的读取到了数据。
        {
            // 把接收到的报文内容原封不动的发回去。
            printf("recv(eventfd=%d):%s\n", this->fd_, buffer);
            send(this->fd_, buffer, strlen(buffer), 0);
        }
        else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取。
            continue;
        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕。
            break;
        else if (nread == 0) // 客户端连接已断开。
        {
            printf("client(eventfd=%d) disconnected.\n", this->fd_);
            close(this->fd_); // 关闭客户端的fd。
            break;
        }
    }

} // 处理对端发送过来的消息。

// 注册 fd 读事件处理函数，回调
void Channel::setreadcallback(std::function<void()> fn)
{
    this->readCallback_ = fn;
}

int Channel::fd() // 返回fd_成员。
{
    return fd_;
}

void Channel::useet() // 采用边缘触发。
{
    events_ = events_ | EPOLLET;
}

void Channel::enablereading() // 让epoll_wait()监视fd_的读事件。
{
    events_ |= EPOLLIN;
    ep_->updatechannel(this);
}

void Channel::setinepoll() // 把inepoll_成员的值设置为true。
{
    inepoll_ = true;
}

void Channel::setrevents(uint32_t ev) // 设置revents_成员的值为参数ev。
{
    revents_ = ev;
}

bool Channel::inpoll() // 返回inepoll_成员。
{
    return inepoll_;
}

uint32_t Channel::events() // 返回events_成员。
{
    return events_;
}

uint32_t Channel::revents() // 返回revents_成员。
{
    return revents_;
}