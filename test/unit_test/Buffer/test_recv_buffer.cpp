#include <fcntl.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory_resource>
#include <vector>

#include "../../../src/base/Buffer/MsgView.h"
#include "../../../src/base/Buffer/RecvBuffer.h"

std::vector<char> make_msg(const std::string& payload) {
    uint32_t len = htonl(payload.size());  // 网络字节序
    std::vector<char> msg(sizeof(uint32_t) + payload.size());
    memcpy(msg.data(), &len, sizeof(uint32_t));
    memcpy(msg.data() + sizeof(uint32_t), payload.data(), payload.size());
    return msg;
}

// 设置 fd 为非阻塞
void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int fds[2];
    pipe(fds);
    set_nonblocking(fds[0]);

    auto getter = [] {
        static std::pmr::monotonic_buffer_resource upstream;
        return &upstream;
    };
    RecvBuffer buffer(getter, 1024, 512);

    // 写入多个报文（拼接写）
    std::vector<std::string> payloads = {"abc", "12345", "net-msg", "EOF"};
    std::vector<char> all;

    for (auto& s : payloads) {
        auto msg = make_msg(s);
        all.insert(all.end(), msg.begin(), msg.end());
    }

    // 一次性写入 write 端
    ssize_t written = write(fds[1], all.data(), all.size());
    assert(written == static_cast<ssize_t>(all.size()));
    std::cout << "Written bytes: " << written << std::endl;

    // 调用 RecvBuffer 读取
    size_t nread = buffer.fillFromFd(fds[0]);
    std::cout << "Read bytes: " << nread << std::endl;

    // 提取消息
    MsgVec msgs = buffer.popMessages();
    std::cout << "Got " << msgs.size() << " messages" << std::endl;
    assert(msgs.size() == payloads.size());

    for (size_t i = 0; i < msgs.size(); ++i) {
        std::string s(msgs[i].data_, msgs[i].size_);
        std::cout << "msg[" << i << "] = " << s << std::endl;
        assert(s == payloads[i]);
    }

    std::cout << "✅ RecvBuffer 非阻塞测试通过！" << std::endl;
    return 0;
}
