#include "EchoServer.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage: ./tcpepoll ip port\n");
        printf("example: ./tcpepoll 192.168.150.128 5085\n\n");
        return -1;
    }

    EchoServer server(argv[1], atoi(argv[2]), 128, 3);
    server.start();

    return 0;
}