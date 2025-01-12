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
    // 提前绑定默认参数
    servChannel->setreadcallback(std::bind(&Channel::newconnection, servChannel, &servsock));
    servChannel->enablereading();  // 监视读事件
    ep.updatechannel(servChannel); // 添加到epoll

    while (true) // 事件循环。
    {
        std::vector<Channel *> rChannels = ep.loop(); // 等待监视的fd有事件发生。
        for (auto ch : rChannels)
            ch->handleEvent();
    }

    return 0;
}