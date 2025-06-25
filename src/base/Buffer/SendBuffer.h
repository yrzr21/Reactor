#pragma once

#include <arpa/inet.h>
#include <sys/uio.h>

#include <deque>

#include "../../types.h"
#include "./MsgView.h"
#include "./RecvBuffer.h"

// 向上游申请头，用于发送数据
// 业务层与本层通信使用 MsgView
// 应确保在析构时已发送全部消息

// 应被上级RAII管理
class SendBuffer {
   public:
    SendBuffer(MemoryResource* upstream);
    ~SendBuffer();

    void pushMsg(MsgVec&& msgs);
    size_t sendAllToFd(int fd);

    // 清除所有未发送的消息，业务层在生命结束前应主动调用
    void clearPendings();

   private:
    // 足够常用，所以直接向池子申请
    Header* new_header();
    // 清理发送完的报文
    void updatePendings(size_t cur_index);
    // 根据成功发送的字节数更新当前发送状态，会修改 vec 中元素，以及 start
    void updateState(IoVecs& vec, size_t nwrite, size_t& start);

   private:
    MemoryResource* upstream_;  // 申请 header 用

    // 二者大小应为一比一
    // 这里需要deq而不用vec是因为确实需要FIFO的语义
    HeaderDeq pending_headers_;  // 手动申请手动释放
    MsgDeq pending_msgs_;        // 自动归还，不管
};

inline SendBuffer::SendBuffer(MemoryResource* upstream) : upstream_(upstream) {}

inline SendBuffer::~SendBuffer() {
    assert(pending_headers_.empty());
    assert(pending_msgs_.empty());
}

inline void SendBuffer::pushMsg(MsgVec&& msgs) {
    for (auto&& msg : msgs) {
        pending_msgs_.push_back(std::move(msg));

        Header* header = new_header();
        header->size = htonl(pending_msgs_.back().size_);
        pending_headers_.push_back(header);
    }
}

inline size_t SendBuffer::sendAllToFd(int fd) {
    // build
    IoVecs iovs;
    for (size_t i = 0; i < pending_headers_.size(); i++) {
        iovs.emplace_back(pending_headers_[i], sizeof(Header));
        iovs.emplace_back(const_cast<char*>(pending_msgs_[i].data_),
                          pending_msgs_[i].size_);
    }

    // send
    size_t nwrite = 0;
    size_t index = 0;  // 下一个待发送的 index，=iovs.size表示全部发送完毕
    while (index < iovs.size()) {
        ssize_t n = ::writev(fd, &iovs[index], iovs.size() - index);
        if (n < 0) {
            if (errno == EINTR) continue;  // interrupt
            // 与 read 不同，小于0表示写满了
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            return -1;  // fatal error
        }
        nwrite += n;

        updateState(iovs, n, index);
    }

    // update
    updatePendings(index);
    return nwrite;
}

inline void SendBuffer::clearPendings() {
    while (!pending_headers_.empty()) {
        Header* front = pending_headers_.front();
        upstream_->deallocate(front, sizeof(Header));
        pending_headers_.pop_front();
    }
}

inline Header* SendBuffer::new_header() {
    return reinterpret_cast<Header*>(upstream_->allocate(sizeof(Header)));
}

inline void SendBuffer::updatePendings(size_t cur_index) {
    // cur_index 是 header 和 vec 一共成功释放的
    for (size_t i = 0; i < cur_index; i++) {
        if (i % 2 == 1) {
            pending_msgs_.pop_front();
            continue;
        }
        Header* front = pending_headers_.front();
        upstream_->deallocate(front, sizeof(Header));

        pending_headers_.pop_front();
    }
}

inline void SendBuffer::updateState(IoVecs& vec, size_t nwrite, size_t& start) {
    while (start < vec.size() && nwrite >= vec[start].iov_len) {
        nwrite -= vec[start].iov_len;
        start++;
    }
    // nwrite <= iovlen or send complete
    if (start < vec.size()) {
        iovec& iov = vec[start];

        iov.iov_base = reinterpret_cast<char*>(iov.iov_base) + nwrite;
        iov.iov_len -= nwrite;
        assert(iov.iov_len > 0);
    }
}
