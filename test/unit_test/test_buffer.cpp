#include "../../src/base/Buffer/Buffer.h"
#include "../../src/base/Buffer/GlobalPool.h"
#include "../../src/base/Buffer/BufferPool.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>

int main() {
    // 设置 pipe，读端非阻塞
    int fds[2];
    if (pipe(fds) == -1) {
        perror("pipe");
        return 1;
    }
    int flags = fcntl(fds[0], F_GETFL, 0);
    fcntl(fds[0], F_SETFL, flags | O_NONBLOCK);

    // 构造 pool 与 buffer
    GlobalPool global(0.1);  // 0.1 GB
    BufferPool buf_pool(global.get_resource(), 1);  // 每 chunk 1 个 block
    Buffer buffer(buf_pool.get_resource(), 4096, 1024);

    // 构造一个完整的报文：4B 头 + N B 数据
    const char* msg = "Hello, Reactor!";
    uint32_t len = strlen(msg);
    char data[1024] = {0};
    std::memcpy(data, &len, sizeof(uint32_t));  // 4 字节长度头
    std::memcpy(data + sizeof(uint32_t), msg, len);

    // 写入 pipe
    write(fds[1], data, sizeof(uint32_t) + len);

    // 调用 fillFromFd
    buffer.fillFromFd(fds[0]);

    // 拿出并消费消息，确保析构前引用清零
    {
        MsgQueue msgs = buffer.popMessages();
        while (!msgs.empty()) {
            MsgView m = std::move(msgs.front());
            msgs.pop();
            std::string s(m.data_, m.size_);
            std::cout << "Got msg: " << s << std::endl;
        }
    }

    // 析构 buffer 会自动检查 refCnt、清理 pool
    close(fds[0]);
    close(fds[1]);
    return 0;
}
