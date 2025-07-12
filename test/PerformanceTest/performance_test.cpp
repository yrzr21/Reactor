#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

using Clock = std::chrono::high_resolution_clock;

// 随机报文长度生成器：中间长度概率更高
int random_msg_size(std::mt19937& rng) {
    std::normal_distribution<> d(512, 150);  // 平均512，stddev=150
    int len = std::round(d(rng));
    if (len < 0) len = 0;
    if (len > 1024) len = 1024;
    return len;
}

// 发送完整 buffer（带 retry）
bool send_full(int sockfd, const char* data, int len) {
    int sent = 0;
    while (sent < len) {
        int n = send(sockfd, data + sent, len - sent, 0);
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}

// 接收完整 buffer（带 retry）
bool recv_full(int sockfd, char* data, int len) {
    int received = 0;
    while (received < len) {
        int n = recv(sockfd, data + received, len - received, 0);
        if (n <= 0) return false;
        received += n;
    }
    return true;
}

void client_worker(int id, const std::string& ip, int port, int request_count,
                   std::atomic<int>& total_success,
                   std::atomic<double>& total_latency) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return;
    }

    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &serv.sin_addr);

    if (connect(sockfd, (sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("connect");
        close(sockfd);
        return;
    }

    std::mt19937 rng(std::random_device{}());

    for (int i = 0; i < request_count; ++i) {
        int payload_len = random_msg_size(rng);
        std::string payload(payload_len, 'x');  // mock 内容

        uint32_t len_net = htonl(payload_len);
        std::string send_buf(reinterpret_cast<char*>(&len_net), 4);
        send_buf += payload;

        auto start = Clock::now();
        if (!send_full(sockfd, send_buf.data(), send_buf.size())) break;

        // 接收 4B 报文长度
        uint32_t resp_len = 0;
        if (!recv_full(sockfd, reinterpret_cast<char*>(&resp_len), 4)) break;
        resp_len = ntohl(resp_len);

        std::vector<char> recv_buf(resp_len);
        if (!recv_full(sockfd, recv_buf.data(), resp_len)) break;

        auto end = Clock::now();
        double latency =
            std::chrono::duration<double, std::milli>(end - start).count();

        total_success++;
        total_latency += latency;
    }

    close(sockfd);
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0]
                  << " <ip> <port> <clients> <requests_per_client>\n";
        return 1;
    }

    std::string ip = argv[1];
    int port = std::stoi(argv[2]);
    int clients = std::stoi(argv[3]);
    int requests = std::stoi(argv[4]);

    std::vector<std::thread> threads;
    std::atomic<int> total_success(0);
    std::atomic<double> total_latency(0);

    for (int i = 0; i < clients; ++i) {
        threads.emplace_back(client_worker, i, ip, port, requests,
                             std::ref(total_success), std::ref(total_latency));
    }

    for (auto& t : threads) t.join();

    std::cout << "Total Success: " << total_success << "\n";
    std::cout << "Average Latency: "
              << (total_success ? total_latency / total_success : 0) << " ms\n";
}
