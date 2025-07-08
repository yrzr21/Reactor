#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#define MAXLEN 1024

// 封装：发送带4B报文头的数据
int send_with_header(int sockfd, const char *data, uint32_t len) {
    uint32_t netlen = htonl(len);
    if (send(sockfd, &netlen, sizeof(netlen), 0) != sizeof(netlen)) {
        perror("send header failed");
        return -1;
    }
    if (send(sockfd, data, len, 0) != len) {
        perror("send body failed");
        return -1;
    }
    return 0;
}

int recv_all_messages(int sockfd, char *buf, uint32_t bufsize) {
    char recvbuf[2048];  // 用于接收原始数据
    int total = recv(sockfd, recvbuf, sizeof(recvbuf), 0);
    if (total <= 0) {
        if (total == 0) {
            printf("server closed connection.\n");
        } else {
            perror("recv failed");
        }
        return -1;
    }

    int offset = 0;
    int msg_count = 0;
    while (offset + 4 <= total) {
        uint32_t netlen;
        memcpy(&netlen, recvbuf + offset, 4);
        uint32_t len = ntohl(netlen);
        offset += 4;

        if (offset + len > total) {
            fprintf(stderr, "incomplete message (expected %u bytes), waiting...\n", len);
            break;
        }

        if (len >= bufsize) {
            fprintf(stderr, "message too large: %u\n", len);
            return -1;
        }

        memcpy(buf, recvbuf + offset, len);
        buf[len] = '\0';
        offset += len;

        printf("recv(%u bytes): %s\n", len, buf);
        msg_count++;
    }

    return msg_count;
}



int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("usage: ./client ip port\n");
        return -1;
    }

    int sockfd;
    struct sockaddr_in servaddr;
    char buf[MAXLEN];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        perror("connect failed");
        close(sockfd);
        return -1;
    }

    printf("connect ok.\n");

    for (int i = 0; i < 200000; i++) {
        memset(buf, 0, sizeof(buf));
        printf("please input: ");
        if (scanf("%s", buf) != 1) break;

        int len = strlen(buf);
        if (send_with_header(sockfd, buf, len) != 0) break;

        memset(buf, 0, sizeof(buf));
        if (recv_all_messages(sockfd, buf, sizeof(buf)) < 0) break;
    }

    close(sockfd);
    return 0;
}
