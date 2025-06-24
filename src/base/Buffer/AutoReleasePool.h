#pragma once

#include <atomic>
#include <iostream>
#include <memory_resource>

#include "../../types.h"
#include "./BufferPool.h"

// 专供 Connection 的 Buffer 使用的内存池
// 下游仅使用本池的do_deallocate，用于减少引用，并返还内存
// 上游应尽快归还内存，以减少"碎片"
// 可重用，
class AutoReleasePool : public MemoryResource {
   public:
    AutoReleasePool(MemoryResource* upstream, size_t chunk_size);
    ~AutoReleasePool();

    // buffer 使用如下公开接口修改内存池
    char* base();
    char* data();  // 下次写入的位置
    // buffer 确保消费的内存不会超过持有的内存
    void consume();
    size_t capacity();
    bool is_released();

    void add_ref();

    // 若为 nullptr 和 0，则继续使用原有 upstream
    // 调用者应确保已经 release
    void reset(MemoryResource* upstream = nullptr, size_t chunk_size = 0);

   private:
    // 重写的 MemoryResource 接口

    // 禁止分配
    void* do_allocate(size_t bytes, size_t alignment) override;
    // 递减引用计数，为0时释放内存给上游
    void do_deallocate(void* p, size_t bytes, size_t alignment) override;

   private:
    // 此类中非原子类型，仅允许在处理连接的事件循环修改
    std::atomic<size_t> ref_cnt = 0;

    MemoryResource* upstream_ = nullptr;
    char* base_ = nullptr;
    char* cur_ = nullptr;
    size_t total_size_ = 0;
};

inline AutoReleasePool::AutoReleasePool(MemoryResource* upstream,
                                        size_t chunk_size) {
    reset(upstream, chunk_size);
}

inline AutoReleasePool::~AutoReleasePool() {
    assert(is_released(), "AutoReleasePool: desturcting while not released\n");
}
inline char* AutoReleasePool::base() { return base_; }
inline char* AutoReleasePool::data() { return cur_; }

inline void AutoReleasePool::consume(size_t bytes) { cur_ += bytes; }

inline void AutoReleasePool::add_ref() {
    // 仅要求原子序
    ref_cnt.fetch_add(1, std::memory_order_relaxed);
}

inline size_t AutoReleasePool::capacity() {
    return total_size_ - static_cast<size_t>(cur - base_);
}

inline bool AutoReleasePool::is_released() { return base_ == nullptr; }

inline void AutoReleasePool::reset(MemoryResource* upstream,
                                   size_t chunk_size) {
    assert(is_released(), "AutoReleasePool: invalid reset\n");

    if (upstream) upstream_ = upstream;
    if (chunk_size) chunk_size_ = chunk_size;
    assert(upstream_);
    assert(chunk_size_);

    base_ = static_cast<char*>(upstream->allocate(chunk_size));
    total_size_ = chunk_size;
    cur_ = base_;
    ref_cnt.store(0);
}

void* AutoReleasePool::do_allocate(size_t bytes, size_t alignment) {
    throw std::bad_alloc("Buffer: allocate refused\n");
}

inline void AutoReleasePool::do_deallocate(void* p, size_t bytes,
                                           size_t alignment) {
    // 仅要求原子序
    size_t old_cnt = ref_cnt.fetch_sub(1, std::memory_order_relaxed);
    assert(old_cnt != 0);

    if (old_cnt == 1) {
        // 释放内存给上游
        upstream_->deallocate(base_, total_size_);
        base_ = nullptr;
    }
}
