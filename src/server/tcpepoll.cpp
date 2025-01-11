/*
 * 程序名：tcpepoll.cpp，此程序用于演示采用epoll模型实现网络通讯的服务端。
 * 作者：吴从周
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>
#include <netinet/tcp.h> // TCP_NODELAY需要包含这个头文件。
#include "tools/InetAddress.h"
#include "tools/Socket.h"
#include "tools/Epoll.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage: ./tcpepoll ip port\n");
        printf("example: ./tcpepoll 192.168.150.128 5085\n\n");
        return -1;
    }

    InetAddress servaddr(argv[1], atoi(argv[2])); // 服务端的地址和协议。

    Socket servsock(createnonblocking());
    servsock.setReuseaddr(true), servsock.setTcpnodelay(true), servsock.setReuseport(true), servsock.setKeepalive(true);
    servsock.bind(servaddr);
    servsock.listen();

    Epoll ep;

    Channel *servChannel = new Channel(&ep, servsock.fd());
    servChannel->enablereading(); // 读事件，水平触发
    ep.updatechannel(servChannel);

    while (true) // 事件循环。
    {
        std::vector<Channel *> rChannels = ep.loop(); // 等待监视的fd有事件发生。

        for (auto ch : rChannels)
        {
            uint32_t event = ch->revents();
            if (event & EPOLLRDHUP) // 对方已关闭，有些系统检测不到，可以使用 EPOLLIN，recv()返回0。
            {
                printf("client(eventfd=%d) disconnected.\n", ch->fd());
                close(ch->fd()); // 关闭客户端的fd。
            } //  普通数据  带外数据
            else if (event & (EPOLLIN | EPOLLPRI)) // 接收缓冲区中有数据可以读。
            {
                if (ch == servChannel) // 如果是listenfd有事件，表示有新的客户端连上来。
                {
                    ////////////////////////////////////////////////////////////////////////
                    InetAddress clientaddr; // 客户端的地址和协议。
                    // 注意，clientsock只能new出来，不能在栈上，否则析构函数会关闭fd。
                    // 还有，这里new出来的对象没有释放，这个问题以后再解决。
                    Socket *clientsock = new Socket(servsock.accept(clientaddr));
                    Channel *peerChannel = new Channel(&ep, clientsock->fd());
                    peerChannel->enablereading(), peerChannel->useet(); // 监视读，边缘触发

                    ep.updatechannel(peerChannel);

                    printf("accept client(fd=%d,ip=%s,port=%d) ok.\n", clientsock->fd(), clientaddr.ip(), clientaddr.port());
                    ////////////////////////////////////////////////////////////////////////
                }
                else // 如果是客户端连接的fd有事件。
                {
                    ////////////////////////////////////////////////////////////////////////
                    char buffer[1024];
                    while (true) // 由于使用非阻塞IO，一次读取buffer大小数据，直到全部的数据读取完毕。
                    {
                        bzero(&buffer, sizeof(buffer));
                        ssize_t nread = read(ch->fd(), buffer, sizeof(buffer));
                        if (nread > 0) // 成功的读取到了数据。
                        {
                            // 把接收到的报文内容原封不动的发回去。
                            printf("recv(eventfd=%d):%s\n", ch->fd(), buffer);
                            send(ch->fd(), buffer, strlen(buffer), 0);
                        }
                        else if (nread == -1 && errno == EINTR) // 读取数据的时候被信号中断，继续读取。
                        {
                            continue;
                        }
                        else if (nread == -1 && ((errno == EAGAIN) || (errno == EWOULDBLOCK))) // 全部的数据已读取完毕。
                        {
                            break;
                        }
                        else if (nread == 0) // 客户端连接已断开。
                        {
                            printf("client(eventfd=%d) disconnected.\n", ch->fd());
                            close(ch->fd()); // 关闭客户端的fd。
                            break;
                        }
                    }
                }
            }
            else if (event & EPOLLOUT) // 有数据需要写，暂时没有代码，以后再说。
            {
            }
            else // 其它事件，都视为错误。
            {
                printf("client(eventfd=%d) error.\n", ch->fd());
                close(ch->fd()); // 关闭客户端的fd。
            }
        }
    }

    return 0;
}