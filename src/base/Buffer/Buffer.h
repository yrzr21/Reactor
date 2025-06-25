#pragma once

#include <cassert>
#include <cstring>
#include <iostream>
#include <memory_resource>
#include <stack>

#include "../../types.h"
#include "./AutoReleasePool.h"
#include "./BufferPool.h"
#include "./MsgView.h"

struct Header {
    uint32_t size = -1;
};

// 低于这个大小、却越过阈值的报文直接拷贝
constexpr size_t MAX_COPY = 512;

// 所有报文大小都不大于 MAX_MSG_SIZE，一个缓冲区一定装得下
class Buffer {
   public:
    Buffer(MemoryResource* upstream, size_t chunk_size, size_t max_msg_size);
    ~Buffer();

    size_t fillFromFd(int fd);

    // 把所有消息给业务层
    MsgQueue popMessages();

   private:
    // 查看wait pool，释放的加入到 idle。从 idle 中取或者创建一个返回
    // 一种懒惰的回收 wait pool的方式
    MsgPoolPtr new_pool();

    // 回收 wait_release_pools_ 中已经释放的 pool
    void recycle_pool();

    // 解析 pool 中报文，完整的push到队列中
    void parseAndPush();
    void handleFullIncomplete();

   private:
   public:
    // 仅在 new_pool 中使用
    MemoryResource* upstream_;
    size_t chunk_size_;

    size_t max_msg_size_;  // 申请创建新池的阈值

    // 指向当前pool的未处理报文的header
    char* cur_unhandled_ptr_ = nullptr;
    size_t next_read_ = 0;

    // 已交付给业务层的报文索引的池
    std::deque<MsgPoolPtr> wait_release_pools_;
    // 空闲
    std::vector<MsgPoolPtr> idle_pools_;
    // 没超过阈值的当前报文，可能不完整

    MsgPoolPtr pool_;  // 声明顺序得放在上面两个的下面
    // 尚未处理的、已解析的完整报文
    MsgQueue pending_msgs_;
};

inline Buffer::Buffer(MemoryResource* upstream, size_t chunk_size,
                      size_t max_msg_size)
    : upstream_(upstream),
      chunk_size_(chunk_size),
      max_msg_size_(max_msg_size),
      pool_(new_pool()) {
    // std::cout << "buffer construct" << std::endl;
}

inline Buffer::~Buffer() {
    assert(wait_release_pools_.empty());
    assert(pending_msgs_.empty());
    assert(pool_->refCnt() == 0);
    pool_->release();
}

inline MsgPoolPtr Buffer::new_pool() {
    std::cout << "new_pool" << std::endl;
    recycle_pool();

    if (idle_pools_.empty()) {
        MsgPoolPtr ret =
            std::make_unique<AutoReleasePool>(upstream_, chunk_size_);
        cur_unhandled_ptr_ = ret->base();
        next_read_ = ret->capacity();
        return ret;
    }

    // 使用 idle 中的返回
    MsgPoolPtr ret = std::move(idle_pools_.back());
    idle_pools_.pop_back();

    ret->init();
    cur_unhandled_ptr_ = ret->base();
    next_read_ = ret->capacity();
    return ret;
}

inline void Buffer::recycle_pool() {
    std::cout << "recycle" << std::endl;
    while (!wait_release_pools_.empty()) {
        if (!wait_release_pools_.front()->is_released()) break;

        idle_pools_.push_back(std::move(wait_release_pools_.front()));
        wait_release_pools_.pop_front();
    }
}

inline void Buffer::parseAndPush() {
    char* end = pool_->data();

    while (cur_unhandled_ptr_ != end) {
        size_t total = static_cast<size_t>(end - cur_unhandled_ptr_);
        if (total < sizeof(Header)) return;  // 头不完整

        Header header = *reinterpret_cast<Header*>(cur_unhandled_ptr_);
        if (header.size > max_msg_size_)  // error
            throw std::bad_exception();

        if (total < sizeof(Header) + header.size) return;  // 头完整报文不完整

        // 构建完整报文并存储
        char* data = cur_unhandled_ptr_ + sizeof(Header);
        pending_msgs_.emplace_back(data, header.size, pool_.get());

        // 更新
        cur_unhandled_ptr_ = data + header.size;
    }
}

inline void Buffer::handleFullIncomplete() {
    char* end = pool_->data();
    size_t unhandled = static_cast<size_t>(end - cur_unhandled_ptr_);

    // debug 用，检查此处的报文是否是不完整的
    if (unhandled > sizeof(Header)) {
        Header header = *reinterpret_cast<Header*>(cur_unhandled_ptr_);
        assert(header.size + sizeof(Header) > unhandled);
    }

    // 报文够小，直接拷贝到新池子
    if (unhandled <= MAX_COPY) {
        MsgPoolPtr old_pool = std::move(pool_);
        char* old_unhandled_ptr_ = cur_unhandled_ptr_;
        pool_ = new_pool();

        std::memcpy(pool_->data(), old_unhandled_ptr_, unhandled);
        pool_->consume(unhandled);
        next_read_ = pool_->capacity();

        old_pool->set_unused();
        wait_release_pools_.push_back(std::move(old_pool));
        return;
    }

    // 报文不完整，下一次系统调用读完整的报文，然后换池子
    Header header = *reinterpret_cast<Header*>(cur_unhandled_ptr_);
    size_t msg_size = sizeof(Header) + header.size;
    assert(unhandled < msg_size);

    next_read_ = msg_size - unhandled;
}

inline size_t Buffer::fillFromFd(int fd) {
    size_t nread = 0;
    while (true) {
        int n = ::read(fd, pool_->data(), next_read_);
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

        parseAndPush();
        // 现在 cur_unhandled_ptr_ 要么指向不完整的报文，要么无报文

        if (pool_->capacity() < max_msg_size_) {
            handleFullIncomplete();
        } else {
            next_read_ = pool_->capacity();
        }
    }

    return nread;
}

MsgQueue Buffer::popMessages() {
    MsgQueue ret;
    ret.swap(pending_msgs_);
    return ret;
}
