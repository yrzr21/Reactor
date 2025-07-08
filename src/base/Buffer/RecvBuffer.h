#pragma once

#include <arpa/inet.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory_resource>
#include <stack>

#include "../../types.h"
#include "./MonoRecyclePool.h"
#include "./MsgView.h"
#include "./SmartMonoPool.h"

struct Header {
    uint32_t size = -1;
};

struct RecvBufferConfig {
    size_t chunk_size;  // 每个 mono pool 的大小
    // chunk_size >= max_msg_size，最好是其数倍
    size_t max_msg_size;  // 本缓冲区的最大报文的大小
};

// 低于这个大小、却越过阈值的报文直接拷贝
// 缓冲区不够时换池子的临界值
constexpr size_t MAX_COPY = 512;

// 所有报文大小都不大于 MAX_MSG_SIZE，一个缓冲区一定装得下
// 应被上级RAII管理
class RecvBuffer {
   public:
    RecvBuffer(UpstreamProvider getter, size_t chunk_size, size_t max_msg_size);
    ~RecvBuffer();

    size_t fillFromFd(int fd);

    // 把所有消息给业务层
    MsgVec popMessages();

   private:
    void parseAndPush();
    void handleFullIncomplete();

   private:
    size_t max_msg_size_;  // 申请创建新池的阈值

    // 指向当前pool的未处理报文的header
    char* cur_unhandled_ptr_ = nullptr;
    size_t next_read_ = 0;

    MonoRecyclePool pool_;
    // 尚未处理的、已解析的完整报文
    MsgVec pending_msgs_;
};

inline RecvBuffer::RecvBuffer(UpstreamProvider getter, size_t chunk_size,
                              size_t max_msg_size)
    : max_msg_size_(max_msg_size), pool_(std::move(getter), chunk_size) {
    // std::cout << "buffer construct" << std::endl;
    cur_unhandled_ptr_ = pool_.base();
    next_read_ = pool_.capacity();
}

inline RecvBuffer::~RecvBuffer() {
    pending_msgs_.clear();

    // // todo 延迟回收资源
    // if(!pool_.can_release()){

    // }
    assert(pool_.can_release());
}

inline void RecvBuffer::parseAndPush() {
    char* end = pool_.data();

    while (cur_unhandled_ptr_ != end) {
        size_t total = static_cast<size_t>(end - cur_unhandled_ptr_);
        if (total < sizeof(Header)) return;  // 头不完整

        Header header = *reinterpret_cast<Header*>(cur_unhandled_ptr_);
        header.size = ntohl(header.size);
        if (header.size > max_msg_size_)  // error
            throw std::bad_exception();

        if (total < sizeof(Header) + header.size) return;  // 头完整报文不完整

        // 构建完整报文并存储
        char* data = cur_unhandled_ptr_ + sizeof(Header);
        pending_msgs_.emplace_back(data, header.size, pool_.get_cur_resource());

        // 更新
        cur_unhandled_ptr_ = data + header.size;
    }
}

inline void RecvBuffer::handleFullIncomplete() {
    char* end = pool_.data();
    size_t unhandled = static_cast<size_t>(end - cur_unhandled_ptr_);

    // debug 用，检查此处的报文是否是不完整的
    if (unhandled > sizeof(Header)) {
        Header header = *reinterpret_cast<Header*>(cur_unhandled_ptr_);
        assert(header.size + sizeof(Header) > unhandled);
    }

    // 报文够小，直接拷贝到新池子
    if (unhandled <= MAX_COPY) {
        pool_.move_to_new_pool(cur_unhandled_ptr_, unhandled);

        cur_unhandled_ptr_ = pool_.base();
        next_read_ = pool_.capacity();
        return;
    }

    // 报文不完整，下一次系统调用读完整的报文，然后换池子
    Header header = *reinterpret_cast<Header*>(cur_unhandled_ptr_);
    size_t msg_size = sizeof(Header) + header.size;
    assert(unhandled < msg_size);

    next_read_ = msg_size - unhandled;
}

inline size_t RecvBuffer::fillFromFd(int fd) {
    size_t nread = 0;
    while (true) {
        int n = ::read(fd, pool_.data(), next_read_);
        if (n <= 0) {
            if (n < 0 && errno == EINTR) continue;  // 被中断
            // 读取完毕，非阻塞 socket 上无数据可读
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
            if (n == 0) return 0;  // 对端关闭
            // 致命错误
            std::cerr << "read() failed, errno=" << errno << ", n=" << n
                      << std::endl;
            return -1;
        }
        pool_.consume(n);
        nread += n;

        parseAndPush();
        // 现在 cur_unhandled_ptr_ 要么指向不完整的报文，要么无报文

        if (pool_.capacity() < max_msg_size_) {
            handleFullIncomplete();
        } else {
            next_read_ = pool_.capacity();
        }
    }

    return nread;
}

inline MsgVec RecvBuffer::popMessages() {
    MsgVec ret;
    ret.swap(pending_msgs_);
    return ret;
}
