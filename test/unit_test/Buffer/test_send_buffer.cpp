#include <fcntl.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <memory_resource>
#include <vector>

#include "../../../src/base/Buffer/SendBuffer.h"
#include "../../../src/base/Buffer/SmartMonoPool.h"

void test_sendbuffer() {
    // 准备基础内存池
    char buffer[1024];
    std::pmr::monotonic_buffer_resource upstream_pool(buffer, sizeof(buffer));

    // 创建 AutoReleasePool（用于 MsgView）
    SmartMonoPool auto_pool(&upstream_pool, 512);

    // 构造测试数据
    const char* msg1 = "Hello, ";
    const char* msg2 = "world!";
    size_t len1 = strlen(msg1);
    size_t len2 = strlen(msg2);

    // 把数据复制到 auto_pool 中
    char* p1 = auto_pool.data();
    std::memcpy(p1, msg1, len1);
    auto_pool.consume(len1);

    char* p2 = auto_pool.data();
    std::memcpy(p2, msg2, len2);
    auto_pool.consume(len2);

    // 构造 MsgView
    MsgView mv1(p1, len1, &auto_pool);
    MsgView mv2(p2, len2, &auto_pool);
    // ref=2

    // 构造 SendBuffer 并添加 MsgView
    SendBuffer sendbuf(&upstream_pool);
    MsgVec vec = {std::move(mv1), std::move(mv2)};
    sendbuf.pushMessage(std::move(vec));  // 多段合成一个SendUnit

    // 创建管道模拟fd写入
    int fds[2];
    assert(pipe(fds) == 0);

    // 设置非阻塞也可以（测试 EAGAIN）
    size_t sent = sendbuf.sendAllToFd(fds[1]);
    std::cout << "Sent bytes: " << sent << std::endl;

    // 读取数据
    char read_buf[128] = {};
    ssize_t n = read(fds[0], read_buf, sizeof(read_buf));
    std::cout << "Received bytes: " << n << std::endl;

    // 读取的数据应该是 header + msg1 + msg2
    Header* hdr = reinterpret_cast<Header*>(read_buf);
    std::cout << "Header size = " << hdr->size << std::endl;
    std::string body(read_buf + sizeof(Header), n - sizeof(Header));
    std::cout << "Body = " << body << std::endl;

    // 验证
    assert(hdr->size == len1 + len2);
    assert(body == "Hello, world!");
    assert(sendbuf.empty());

    // 主动释放（测试引用计数）
    auto_pool.set_unused();
    assert(auto_pool.is_released());

    close(fds[0]);
    close(fds[1]);

    std::cout << "Test passed ✅" << std::endl;
}

int main() {
    test_sendbuffer();
    return 0;
}
