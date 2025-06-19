// 网络通讯的客户端程序。
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage:./client ip port\n");
        printf("example:./client 192.168.150.128 5085\n\n");
        return -1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char buf[1024];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket() failed.\n");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        printf("connect(%s:%s) failed.\n", argv[1], argv[2]);
        close(sockfd);
        return -1;
    }

    printf("connect ok.\n");
    printf("开始时间：%d\n", time(0));

    for (int i = 0; i < 10; i++) {
        memset(buf, 0, sizeof(buf));
        sprintf(buf, "这是第%d个客户端。", i);

        char tmpbuf[1024];  // 临时的buffer，报文头部+报文内容。
        memset(tmpbuf, 0, sizeof(tmpbuf));
        uint32_t len = strlen(buf);  // 计算报文的大小。
        uint32_t net_len = htonl(len);
        printf("len=%d\n", len);
        printf("net_len=%d\n", net_len);
        memcpy(tmpbuf, &net_len, 4);   // 拼接报文头部。
        memcpy(tmpbuf + 4, buf, len);  // 拼接报文内容。

        send(sockfd, tmpbuf, len + 4, 0);  // 把请求报文发送给服务端。

        recv(sockfd, &len, 4, 0);  // 先读取4字节的报文头部。
        len = ntohl(len);
        printf("recv, len=%d", len);

        memset(buf, 0, sizeof(buf));
        recv(sockfd, buf, len, 0);  // 读取报文内容。

        printf(", data=%s\n", buf);
        sleep(1);
    }
    // sleep(100);

    printf("结束时间：%d\n", time(0));
}
