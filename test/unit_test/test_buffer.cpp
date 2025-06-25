#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include <cstring>
#include <iostream>
#include "../../src/base/Buffer/Buffer.h"
#include "../../src/base/Buffer/GlobalPool.h"

void write_msg(int fd, const std::string& body) {
    uint32_t len = body.size();
    write(fd, &len, sizeof(len));
    write(fd, body.data(), len);
}

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int fds[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
    int rfd = fds[0];
    int wfd = fds[1];
    set_nonblock(rfd);

    GlobalPool global(0.1);  // 100MB够测了
    Buffer buffer(global.get_resource(), 4096, 1024);

    // Round 1: 粘包 A+B+C
    {
        std::string A = "HelloA";
        std::string B = "Msg_Body";
        std::string C = "Final_C_Message";

        std::string blob;
        uint32_t a_len = A.size(), b_len = B.size(), c_len = C.size();
        blob.append(reinterpret_cast<char*>(&a_len), 4);
        blob.append(A);

        blob.append(reinterpret_cast<char*>(&b_len), 4);
        blob.append(B);

        blob.append(reinterpret_cast<char*>(&c_len), 4);
        blob.append(C);

        write(wfd, blob.data(), blob.size());
    }

    // Round 2: Header 半包（只写入 header 前2字节）
    {
        uint16_t half = 0x000C;  // 假设未来是长度12（但只写入前2字节）
        write(wfd, &half, sizeof(half));
    }

    // Round 3: 写入剩下的 header（2字节） + 报文内容
    {
        uint16_t rest = 0x0000;  // 补足为完整 header: 0x000C0000 -> 12
        write(wfd, &rest, sizeof(rest));

        std::string body = "HALF_HEADER";  // 11B，头写的是12 → 故意错一字节，触发校验
        body += ".";  // 填满12
        write(wfd, body.data(), body.size());
    }

    // Round 4: 写入一个小报文触发 MAX_COPY
    {
        std::string tiny = "t";
        write_msg(wfd, tiny);
    }

    // Round 5: 写入一个大报文（正好 1020 字节）
    {
        std::string big(1020, 'X');
        write_msg(wfd, big);
    }

    // 读取 & 解析
    for (int i = 0; i < 10; ++i) {
        buffer.fillFromFd(rfd);

        MsgVec msgs = buffer.popMessages();
        while (!msgs.empty()) {
            MsgView m = std::move(msgs.back());
            msgs.pop_back();
            std::string s(m.data_, m.size_);
            std::cout << "Got msg: [" << s << "] size=" << m.size_ << "\n";
        }

        usleep(1000);  // 模拟事件轮询中的调度
    }

    close(rfd);
    close(wfd);
    return 0;
}
