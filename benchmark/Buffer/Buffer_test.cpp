// test_Buffer.cpp
#include "Buffer.h"

#include <arpa/inet.h>    // htonl, ntohl
#include <fcntl.h>        // fcntl
#include <sys/socket.h>   // socketpair
#include <unistd.h>       // close, write
#include <iostream>
#include <string>
#include <vector>

// 简单的测试 harness
static int tests_passed = 0;
static int tests_total  = 0;

#define RUN_TEST(fn)                                \
    do {                                            \
        ++tests_total;                             \
        std::cout << "Running " << #fn << "... ";  \
        if (fn()) {                                \
            std::cout << "PASSED\n";               \
            ++tests_passed;                        \
        } else {                                    \
            std::cout << "FAILED\n";               \
        }                                           \
    } while (0)

// Helper: 构造 [4B length][payload] 报文
static std::vector<char> makePacket(const std::string &msg) {
    uint32_t n = htonl((uint32_t)msg.size());
    std::vector<char> pkt;
    pkt.resize(sizeof(n) + msg.size());
    std::memcpy(pkt.data(), &n, sizeof(n));
    std::memcpy(pkt.data() + sizeof(n), msg.data(), msg.size());
    return pkt;
}

// Helper: 把 pkt 写入 socket，并把 socket 设为非阻塞
static int setupSocket(const std::vector<char> &pkt, int &out_fd) {
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) return -1;
    // sv[0] 写入端，sv[1] 读取端
    // 设为非阻塞
    int flags = fcntl(sv[1], F_GETFL, 0);
    fcntl(sv[1], F_SETFL, flags | O_NONBLOCK);
    // 写入数据
    size_t total = 0;
    while (total < pkt.size()) {
        ssize_t n = write(sv[0], pkt.data() + total, pkt.size() - total);
        if (n <= 0) return -1;
        total += n;
    }
    close(sv[0]);
    out_fd = sv[1];
    return 0;
}

// Test1: 单条完整消息
bool testSingleMessage() {
    std::string msg = "hello_world";
    auto pkt = makePacket(msg);

    int fd;
    if (setupSocket(pkt, fd) < 0) return false;

    Buffer buf;
    // 读取所有可用数据
    if (buf.fillFromFd(fd) < 0) { close(fd); return false; }
    // pop
    auto got = buf.popMessage();
    close(fd);

    return got == msg;
}

// Test2: 两条消息连发
bool testMultipleMessages() {
    std::string m1 = "foo", m2 = "barbaz";
    auto p1 = makePacket(m1), p2 = makePacket(m2);

    std::vector<char> both;
    both.insert(both.end(), p1.begin(), p1.end());
    both.insert(both.end(), p2.begin(), p2.end());

    int fd;
    if (setupSocket(both, fd) < 0) return false;

    Buffer buf;
    if (buf.fillFromFd(fd) < 0) { close(fd); return false; }

    bool ok = true;
    ok &= (buf.popMessage() == m1);
    ok &= (buf.popMessage() == m2);
    close(fd);
    return ok;
}

// Test3: 先发半个头，后发剩余数据
bool testPartialHeaderThenContinue() {
    std::string msg = "partial_test";
    auto pkt = makePacket(msg);

    // 拆成两段：先发前2字节
    std::vector<char> part1(pkt.begin(), pkt.begin() + 2);
    std::vector<char> part2(pkt.begin() + 2, pkt.end());

    int sv[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) < 0) return false;
    // 非阻塞读端
    int flags = fcntl(sv[1], F_GETFL, 0);
    fcntl(sv[1], F_SETFL, flags | O_NONBLOCK);

    // 写入第一段
    if (write(sv[0], part1.data(), part1.size()) != (ssize_t)part1.size()) {
        close(sv[0]); close(sv[1]); return false;
    }

    Buffer buf;
    // 先读半包，popMessage 应为空
    buf.fillFromFd(sv[1]);
    if (!buf.popMessage().empty()) {
        close(sv[0]); close(sv[1]); return false;
    }

    // 写剩余数据
    if (write(sv[0], part2.data(), part2.size()) != (ssize_t)part2.size()) {
        close(sv[0]); close(sv[1]); return false;
    }
    close(sv[0]);

    // 再读
    buf.fillFromFd(sv[1]);
    auto got = buf.popMessage();
    close(sv[1]);

    return got == msg;
}

int main() {
    RUN_TEST(testSingleMessage);
    RUN_TEST(testMultipleMessages);
    RUN_TEST(testPartialHeaderThenContinue);

    std::cout << "\nTests passed: "
              << tests_passed << " / " << tests_total << std::endl;
    return (tests_passed == tests_total) ? 0 : 1;
}
