#pragma once

#include <arpa/inet.h>
#include <sys/uio.h>

#include <deque>

#include "../../types.h"
#include "./MsgView.h"
#include "./RecvBuffer.h"
#include "./SendUnit.h"

// 向上游申请头，用于发送数据
// 业务层与本层通信使用 MsgView
// 应确保在析构时已发送全部消息

// 应被上级RAII管理
class SendBuffer {
   public:
    SendBuffer(MemoryResource* upstream);
    ~SendBuffer();

    // 每个msg view都是一个单独的send unit
    void pushMessages(MsgVec&& msgs);
    // 多个msg view拼接为一个send unit
    void pushMessage(MsgVec&& msg);
    // push 一个
    void pushMessage(MsgView&& msg);

    // 牺牲一点性能，每次发送都构造iovec，换取可控性
    size_t sendAllToFd(int fd);
    // todo sendFile 等函数
    bool empty();

    // 清除所有未发送的消息，业务层在生命结束前应主动调用
    void clearPendings();

   private:
    // 更新pending_units_。writev 保证 nwrite 不大于发送单元总大小
    void updateState(size_t nwrite);

   private:
    MemoryResource* upstream_;  // 留给SendUnit申请header用

    std::deque<SendUnit> pending_units_;
};

inline SendBuffer::SendBuffer(MemoryResource* upstream) : upstream_(upstream) {}

inline SendBuffer::~SendBuffer() { assert(pending_units_.empty()); }

inline void SendBuffer::pushMessages(MsgVec&& msgs) {
    for (auto& msg : msgs) {
        pending_units_.emplace_back(std::move(msg), upstream_);
    }
    // std::cout << "[tid=" << std::this_thread::get_id()
    //           << "] SendBuffer::pushMessages finished" << std::endl;
}

inline void SendBuffer::pushMessage(MsgVec&& msg) {
    pending_units_.emplace_back(std::move(msg), upstream_);
}

inline void SendBuffer::pushMessage(MsgView&& msg) {
    pending_units_.emplace_back(std::move(msg), upstream_);
}

inline size_t SendBuffer::sendAllToFd(int fd) {
    // std::cout << "[tid=" << std::this_thread::get_id()
    //           << "] SendBuffer::sendAllToFd" << std::endl;
    size_t nwrite = 0;
    while (!pending_units_.empty()) {
        IoVecs iovs;
        for (auto& unit : pending_units_) {
            auto unit_iovs = unit.get_iovecs();
            std::move(unit_iovs.begin(), unit_iovs.end(),
                      std::back_inserter(iovs));
        }

        ssize_t n = ::writev(fd, iovs.data(), iovs.size());
        if (n < 0) {
            if (errno == EINTR) continue;  // interrupt
            // 与 read 不同，小于0表示写满了
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                errno = 0;  // 防止上层误读
                break;
            }
            return -1;  // fatal error
        }
        nwrite += n;

        updateState(n);
    }
    return nwrite;
}

inline void SendBuffer::updateState(size_t nwrite) {
    size_t i = 0;
    while (nwrite > 0) {
        assert(i <= pending_units_.size());
        nwrite = pending_units_[i].advance(nwrite);

        if (!pending_units_[i].finished()) break;
        i++;
    }
    pending_units_.erase(pending_units_.begin(), pending_units_.begin() + i);
}

inline bool SendBuffer::empty() { return pending_units_.empty(); }

inline void SendBuffer::clearPendings() { pending_units_.clear(); }