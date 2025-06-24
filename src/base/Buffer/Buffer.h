#pragma once

#include <iostream>
#include <memory_resource>
#include <stack>

#include "../../types.h"
#include "./AutoReleasePool.h"
#include "./BufferPool.h"

struct Header {
    uint32_t size = -1;
};

using MsgPoolPtr = std::unique_ptr<AutoReleasePool>;
using MsgQueue = std::queue<std::pmr::string>;

// 所有报文大小都不大于 MAX_MSG_SIZE，一个缓冲区一定装得下
class Buffer {
   public:
    Buffer(MemoryResource* upstream, size_t chunk_size, size_t max_msg_size);
    ~Buffer();

    size_t fillFromFd(int fd);

    // 把所有消息给业务层
    MsgQueue popMessages();

   private:
    bool isCurMessageComplete();

    // 查看wait pool，释放的加入到 idle。从 idle 中取或者创建一个返回
    // 一种懒惰的回收 wait pool的方式
    MsgPoolPtr new_pool();

    // 独占 pool
    void parse_pending_msgs(MsgPoolPtr pool, MsgQueue& container);
    // 回收 wait_release_pools_ 中已经释放的 pool
    void recycle_pool();

   private:
    MemoryResource* upstream_;  // 仅在 new_pool 中使用
    size_t chunk_size_;
    size_t max_msg_size_;  // 申请创建新池的阈值

    // 指向当前pool的未处理报文的header
    char* cur_unhandled_ptr_ = nullptr;
    // 这个header是否有效，因为读取到的header可能不完整
    bool cur_header_valid_ = false;
    // 没超过阈值的当前报文，可能不完整
    MsgPoolPtr pool_;
    // 超过阈值的、待处理的完整报文
    std::queue<MsgPoolPtr> pending_pools_;
    // 已交付给业务层的报文
    std::queue<MsgPoolPtr> wait_release_pools_;
    // 空闲
    std::stack<MsgPoolPtr> idle_pools_;

    // 尚未处理的、已解析的完整报文
    // pending_pools_ 似乎就不必要了
    MsgQueue pending_msgs_;
}

inline Buffer::Buffer(MemoryResource* upstream, size_t chunk_size,
                      size_t max_msg_size)
    : upstream_(upstream),
      chunk_size_(chunk_size),
      max_msg_size_(max_msg_size),
      pool_(new_pool()) {
}

inline Buffer::~Buffer() {
    assert(pending_pools_.empty());
    assert(wait_release_pools_.empty());
}

inline MsgPoolPtr Buffer::new_pool() {
    recycle_pool();

    if (idle_pools_.empty()) {
        MsgPoolPtr ret =
            std::make_unique<AutoReleasePool>(upstream_, chunk_size_);
        cur_unhandled_ptr_ = ret.base();
        return ret;
    }

    // 使用 idle 中的返回
    MsgPoolPtr ret = std::move(idle_pools_.top());
    idle_pools_.pop();
    cur_unhandled_ptr_ = ret.base();

    ret.reset();
    return ret;
}

inline void Buffer::parse_pending_msgs(MsgPoolPtr pool, MsgQueue& container) {
    char* base = pool->base();
    char* end = pool->data();
    while (base != end) {
        Header header = *reinterpret_cast<Header*>(base);
        char* data = base + sizeof(Header);

        // 错误报文
        // 似乎后者不应该是错误，而是不完整就直接截断申请下一个内存池了
        if (header.size > max_msg_size_ ||
            end - base < sizeof(Header) + header.size) {
            throw std::bad_exception("wrong msg received");
        }

        // data size upstream
        std::pmr::string msg(data, header.size, pool.get());

        container.push(std::move(msg));
        pool->add_ref();

        base += (sizeof(Header) + header.size);
    }

    wait_release_pools_.push(std::move(pool));
}

inline void Buffer::recycle_pool() {
    while (!wait_release_pools_.empty()) {
        if (!wait_release_pools_.front()->is_released()) break;

        idle_pools_.push(std::move(wait_release_pools_.front()));
        wait_release_pools_.pop();
    }
}

inline size_t Buffer::fillFromFd(int fd) {
    size_t nread = 0;
    while (true) {
        size_t n = ::read(fd, pool_->data(), pool_->capacity());
        if (n <= 0) {
            // 被中断
            if (n < 0 && errno == EINTR) continue;
            // 读取完毕，非阻塞 socket 上无数据可读
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
            // 对端关闭或致命错误
            return -1;
        }
        pool_->consume(n);
        nread += n;

        if (pool_->capacity() < max_msg_size_) {
            pending_pools_.push(std::move(pool_));
            pool_ = new_pool();
        }
    }

    return nread;
}

MsgQueue Buffer::popMessages() {
    MsgQueue ret;

    // 回收 pending 池中的报文
    while (!pending_pools_.empty()) {
        MsgPoolPtr cur_pool = std::move(pending_pools_.front());
        pending_pools_.pop();

        parse_pending_msgs(std::move(cur_pool), ret);
    }

    // 单独处理当前 pool，可能会遇到不完整报文
    char* base = cur_unhandled_ptr_;
    char* end = pool_->data();
    while (base != end) {
        Header header = *reinterpret_cast<Header*>(base);
        char* data = base + sizeof(Header);

        // 不完整报文，大小不足header和大于header的均用此判断
        if (end - base < sizeof(Header) + header.size) {
            break;
        }

        if (header.size > max_msg_size_)
            throw std::bad_exception("wrong msg received");

        // data size upstream
        std::pmr::string msg(data, header.size, pool_.get());

        ret.push(std::move(msg));
        pool_->add_ref();

        base += (sizeof(Header) + header.size);
    }

    cur_pool_unhandled_idx_ = base;
    // 无需判断是否需将当前 pool 回收，因其剩余容量肯定大于 max msg

    return ret;
}

inline bool Buffer::isCurMessageComplete() {
    return false;
    //
}
