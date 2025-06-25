#include <fcntl.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>

#include "../../../src/base/Buffer/AutoReleasePool.h"
#include "../../../src/base/Buffer/GlobalPool.h"
#include "../../../src/base/Buffer/MsgView.h"
#include "../../../src/base/Buffer/RecvBuffer.h"
#include "../../../src/base/Buffer/SendBuffer.h"

int set_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    // 建立非阻塞 pipe
    int fds[2];
    if (pipe(fds) != 0) {
        perror("pipe");
        return 1;
    }
    set_non_blocking(fds[0]);
    set_non_blocking(fds[1]);

    // 使用全局内存池 + buffer pool
    GlobalPool global_pool(0.01);  // 10MB
    BufferPool buffer_pool(global_pool.get_resource(), 1);
    MemoryResource* mem = buffer_pool.get_resource();

    // ======== 构造发送缓冲区并填入 MsgView =========
    SendBuffer sender(mem);

    AutoReleasePool pool(mem, 1024);

    const char* msg1 = "foo";
    const char* msg2 = "barbaz";

    std::memcpy(pool.data(), msg1, strlen(msg1));
    pool.consume(strlen(msg1));
    MsgView v1(pool.base(), strlen(msg1), &pool);

    std::memcpy(pool.data(), msg2, strlen(msg2));
    pool.consume(strlen(msg2));
    MsgView v2(pool.base() + strlen(msg1), strlen(msg2), &pool);

    MsgVec vec;
    vec.push_back(std::move(v1));
    vec.push_back(std::move(v2));

    sender.pushMsg(std::move(vec));

    size_t written = sender.sendAllToFd(fds[1]);
    std::cout << "Sender wrote: " << written << " bytes\n";

    // ======== 构造接收缓冲区并调用 fillFromFd =========
    RecvBuffer receiver(mem, 1024, 256);

    size_t nread = receiver.fillFromFd(fds[0]);
    std::cout << "Receiver read: " << nread << " bytes\n";

    MsgVec result = receiver.popMessages();
    assert(result.size() == 2);

    std::string recv1(result[0].data_, result[0].size_);
    std::string recv2(result[1].data_, result[1].size_);

    std::cout << "recv1 = " << recv1 << "\n";
    std::cout << "recv2 = " << recv2 << "\n";

    assert(recv1 == "foo");
    assert(recv2 == "barbaz");

    close(fds[0]);
    close(fds[1]);

    std::cout << "✅ 测试通过：SendBuffer 自动加头 + RecvBuffer 自动解析成功\n";

    pool.release();
    return 0;
}
