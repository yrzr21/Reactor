#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

void send_messages(const std::string& ip, uint16_t port, uint64_t send_count,
                   uint64_t payload_size, std::atomic<uint64_t>& counter) {
    uint64_t sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        // perror("socket");
        return;
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid IP: " << ip << std::endl;
        close(sockfd);
        return;
    }

    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        // perror("connect");
        close(sockfd);
        return;
    }

    // 准备发送缓冲区，先4字节长度头，后面是payload_size字节数据（用0填充）
    std::vector<char> buf(4 + payload_size, 0);
    uint32_t net_len = htonl(payload_size);
    memcpy(buf.data(), &net_len, 4);

    for (uint64_t i = 0; i < send_count; ++i) {
        ssize_t sent = 0;
        while (sent < (ssize_t)buf.size()) {
            ssize_t n = send(sockfd, buf.data() + sent, buf.size() - sent, 0);
            if (n <= 0) {
                // perror("send");
                close(sockfd);
                return;
            }
            sent += n;
        }
        ++counter;
    }

    close(sockfd);
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0]
                  << " <ip> <port> <concurrency> <send_count> <payload_size>"
                  << std::endl;
        return 1;
    }

    std::string ip = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
    uint64_t concurrency = std::stoi(argv[3]);
    uint64_t send_count = std::stoi(argv[4]);
    uint64_t payload_size = std::stoi(argv[5]);

    std::atomic<uint64_t> total_sent{0};

    std::vector<std::thread> threads;
    threads.reserve(concurrency);

    auto start = std::chrono::steady_clock::now();

    for (uint64_t i = 0; i < concurrency; ++i) {
        threads.emplace_back(send_messages, ip, port, send_count, payload_size,
                             std::ref(total_sent));
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();

    std::cerr << "Total messages sent: " << total_sent.load() << std::endl;
    std::cerr << "Elapsed time: " << seconds << " seconds" << std::endl;
    std::cerr << "Throughput: " << (total_sent.load() / seconds)
              << " messages/sec" << std::endl;
    std::cerr << "Throughput: "
              << (total_sent.load() * payload_size / seconds / 1024 / 1024)
              << " MB/s" << std::endl;

    return 0;
}
