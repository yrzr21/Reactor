#include <signal.h>
#include "./server/EchoServer.h"

EchoServer *server;

void stopServer(int sig)
{
    printf("sig=%d\n", sig);
    server->stop();
    printf("server已停止。\n");
    delete server;
    printf("delete server\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("usage: ./tcpepoll ip port\n");
        printf("example: ./tcpepoll 192.168.150.128 5085\n\n");
        return -1;
    }

    signal(SIGTERM, stopServer);
    signal(SIGINT, stopServer);

    server = new EchoServer(argv[1], atoi(argv[2]), 128, 30, 0, 60, 10);
    server->start();

    return 0;
}