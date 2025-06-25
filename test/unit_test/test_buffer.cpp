#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

#include "../../src/base/Buffer/Buffer.h"
#include "../../src/base/Buffer/BufferPool.h"
#include "../../src/base/Buffer/GlobalPool.h"
#include "../../src/base/Buffer/MsgView.h"

constexpr size_t MAX_MSG_SIZE = 1024;

void write_fake_messages(int write_fd) {
    struct Header {
        uint32_t size;
    };

    std::vector<std::string> messages = {"Hello, World!", "Test message 2",
                                         "Third one is here"};

    for (const auto& msg : messages) {
        Header header{static_cast<uint32_t>(msg.size())};
        ::write(write_fd, &header, sizeof(Header));
        ::write(write_fd, msg.data(), msg.size());
    }
}

int main() {
    // pipe 模拟 fd 输入
    int fds[2];
    if (pipe(fds) != 0) {
        perror("pipe");
        return 1;
    }

    int read_fd = fds[0];
    int write_fd = fds[1];

    // Step 1: 构造内存池
    GlobalPool global_pool(0.01);  // 10MB 全局内存
    BufferPool buffer_pool(global_pool.get_resource(), 4);

    // Step 2: 构造 Buffer（负责管理报文读取）
    Buffer buffer(buffer_pool.get_resource(), CHUNK_SIZE, MAX_MSG_SIZE);

    // Step 3: 写入模拟报文数据
    write_fake_messages(write_fd);
    close(write_fd);  // 模拟对端关闭连接

    // Step 4: 从 fd 填充并解析报文
    ssize_t ret = buffer.fillFromFd(read_fd);
    assert(ret > 0);

    // Step 5: 取出所有解析好的消息并打印
    auto queue = buffer.popMessages();
    int idx = 1;
    while (!queue.empty()) {
        const auto& msg = queue.front();
        std::cout << "Msg #" << idx++ << ": ";
        std::cout.write(msg.data_, msg.size_);
        std::cout << std::endl;
        queue.pop();
    }

    return 0;
}
