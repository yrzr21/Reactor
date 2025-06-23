#include <signal.h>

#include <iostream>

#include "./server/EchoServer.h"

EchoServer *server;

void stopServer(int sig) {
    printf("sig=%d\n", sig);
    server->stop();
    printf("server已停止。\n");
    delete server;
    printf("delete server\n");
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 8) {
        std::cout << "invalid argument： " << std::endl;
        for (size_t i = 0; i < argc; i++) {
            std::cout << argv[i] << std::endl;
        }

        return -1;
    }

    signal(SIGTERM, stopServer);
    signal(SIGINT, stopServer);

    // std::cout << "start" << std::endl;
    // server = new EchoServer(argv[1], atoi(argv[2]), 128, 3, 3, 60, 10);
    server =
        new EchoServer(argv[1], atoi(argv[2]), atoi(argv[3]), atoi(argv[4]),
                       atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));
    server->start();

    return 0;
}