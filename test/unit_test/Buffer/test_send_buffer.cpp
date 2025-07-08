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

using namespace std;

MsgView make_msg_view(pmr::string str) {
    MsgView mv(std::move(str));
    return mv;
};

void push_data(MsgVec& msgs) {
    msgs.emplace_back(make_msg_view("ok,"));
    msgs.emplace_back(make_msg_view("fine,"));
    msgs.emplace_back(make_msg_view("whatever,"));
}

void parse_msgs_and_print(int fd) {
    char buf[4096];
    int n = read(fd, buf, 4096);

    int offset = 0;
    while (offset < n) {
        assert(offset < 4096);
        Header* header = reinterpret_cast<Header*>(buf + offset);
        size_t size = ntohl(header->size);
        std::cout << "msg size = " << size << ", data = ";
        string msg(buf + offset + sizeof(Header), size);
        std::cout << msg << std::endl;

        offset += (sizeof(Header) + size);
    }
}

int main() {
    int fd[2];
    pipe(fd);

    static std::pmr::monotonic_buffer_resource upstream;
    SendBuffer sendbuf(&upstream);

    std::cout << "测试多个msg view作为一个send unit发送" << std::endl;
    MsgVec msgs1;
    push_data(msgs1);
    sendbuf.pushMessage(std::move(msgs1));
    sendbuf.sendAllToFd(fd[1]);
    parse_msgs_and_print(fd[0]);

    std::cout << "\n测试每个msg view均单独作为一个send unit发送" << std::endl;
    MsgVec msgs2;
    push_data(msgs2);
    sendbuf.pushMessages(std::move(msgs2));
    sendbuf.sendAllToFd(fd[1]);
    parse_msgs_and_print(fd[0]);

    std::cout << "\n测试多种情况的send unit发送" << std::endl;
    MsgVec msgs3;
    push_data(msgs3);
    sendbuf.pushMessage(std::move(msgs3));
    MsgVec msgs4;
    push_data(msgs4);
    sendbuf.pushMessages(std::move(msgs4));
    sendbuf.sendAllToFd(fd[1]);
    parse_msgs_and_print(fd[0]);

    std::cout << "Test passed" << std::endl;
    close(fd[0]);
    close(fd[1]);

    return 0;
}
