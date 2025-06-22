#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

std::atomic<int> connectedCount(0);

int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int createAndConnect(const char* ip, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        close(sockfd);
        return -1;
    }

    // 设置非阻塞，防止connect阻塞
    if (setNonBlocking(sockfd) < 0) {
        close(sockfd);
        return -1;
    }

    int ret = connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        if (errno != EINPROGRESS) {
            close(sockfd);
            return -1;
        }
        // EINPROGRESS表示非阻塞连接正在进行中，正常情况
    }

    return sockfd;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0]
                  << " server_ip server_port connection_count\n";
        return 1;
    }

    struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) == 0) {
        std::cout << "RLIMIT_NOFILE soft=" << limit.rlim_cur
                  << ", hard=" << limit.rlim_max << std::endl;
    } else {
        perror("getrlimit failed");
    }

    const char* server_ip = argv[1];
    int server_port = std::stoi(argv[2]);
    int targetConnections = std::stoi(argv[3]);

    std::vector<int> sockets;
    sockets.reserve(targetConnections);

    for (int i = 1; i <= targetConnections; ++i) {
        int sockfd = createAndConnect(server_ip, server_port);
        if (sockfd < 0) {
            std::cerr << "Failed to create/connect socket #" << i << std::endl;
            break;
        }
        sockets.push_back(sockfd);
        connectedCount++;

        if (i % 1000 == 0 || i == targetConnections) {
            std::cout << "Created " << i << " connections" << std::endl;
        }

        // 为了不压垮客户端机器，稍微休眠一下
        if (i % 1000 == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "Total connections established: " << connectedCount.load()
              << std::endl;
    std::cout << "Press Enter to close all connections and exit..."
              << std::endl;
    std::cin.get();

    for (auto fd : sockets) {
        close(fd);
    }

    return 0;
}
