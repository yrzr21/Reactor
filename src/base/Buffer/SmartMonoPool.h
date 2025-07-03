#pragma once

#include <atomic>
#include <cassert>
#include <iostream>
#include <memory_resource>

#include "../../types.h"
#include "./BufferPool.h"

/*
本池设计目标为快速申请、提高复用率，代价是需要在引用计数为0时归还
因此不适用于超长生命周期对象，若长时间不归还，会导致本池已释放的内存成为“碎片”，降低内存利用率

两种用法：
    1. allocate/deallocate。
        deallocate 等价于减小引用计数，参数无意义
        ——适用于已知申请内存的大小情况，例如业务层的用法
    2. 自行使用 data+consume+add_ref 管理池内存分配，然后 deallocate
        ——适用于未知需要多少内存、但最大大小已知的情况，例如 recvbuffer 中的用法

应被上级RAII管理，即所有资源引用计数为0后再析构
 */

class SmartMonoPool : public MemoryResource {
   public:
    SmartMonoPool(MemoryResource* upstream, size_t chunk_size);
    ~SmartMonoPool();  // 应被 RAII 管理，类似于 malloc

    size_t capacity();
    char* base();
    char* data();                // 下次写入的位置
    void consume(size_t bytes);  // 调用者确保消费的内存不会超过持有的内存

    void add_ref();
    size_t ref();

    void set_unused();
    void release();
    bool is_released();

    // 若为 nullptr 和 0，则继续使用原有 upstream
    // 调用者应确保已经 release
    void init(MemoryResource* upstream = nullptr, size_t chunk_size = 0);

   private:
    void* do_allocate(size_t bytes, size_t alignment) override;
    void do_deallocate(void* p, size_t bytes, size_t alignment) override;
    bool do_is_equal(const memory_resource& __other) const noexcept override;

   private:
    // 此类中非原子类型，仅允许在处理连接的事件循环修改
    std::atomic<size_t> ref_cnt = 0;
    // 即便 ref_cnt=0， 若 is_using，也不应释放
    std::atomic_bool is_using = true;

    MemoryResource* upstream_ = nullptr;
    char* base_ = nullptr;
    char* cur_ = nullptr;
    size_t total_size_ = 0;
};

inline SmartMonoPool::SmartMonoPool(MemoryResource* upstream,
                                    size_t chunk_size) {
    init(upstream, chunk_size);
}

inline SmartMonoPool::~SmartMonoPool() { assert(is_released()); }
inline char* SmartMonoPool::base() { return base_; }
inline char* SmartMonoPool::data() { return cur_; }

inline void SmartMonoPool::consume(size_t bytes) {
    cur_ += bytes;
    assert(cur_ <= base_ + total_size_);
}

inline void SmartMonoPool::add_ref() {
    // 仅要求原子序
    ref_cnt.fetch_add(1, std::memory_order_relaxed);
    // std::cout << "refcnt=" << ref_cnt << std::endl;
}

inline size_t SmartMonoPool::ref() { return ref_cnt; }

inline void SmartMonoPool::set_unused() {
    is_using.store(false);
    if (ref_cnt == 0) release();
}

inline size_t SmartMonoPool::capacity() {
    return total_size_ - static_cast<size_t>(cur_ - base_);
}

inline bool SmartMonoPool::is_released() { return base_ == nullptr; }

inline void SmartMonoPool::release() {
    is_using.store(false);
    upstream_->deallocate(base_, total_size_);
    base_ = nullptr;
}

inline void SmartMonoPool::init(MemoryResource* upstream, size_t chunk_size) {
    assert(is_released());

    if (upstream) upstream_ = upstream;
    if (chunk_size) total_size_ = chunk_size;
    assert(upstream_);
    assert(total_size_);

    base_ = static_cast<char*>(upstream_->allocate(total_size_));
    cur_ = base_;
    ref_cnt.store(0);
    is_using.store(true);
}

void* SmartMonoPool::do_allocate(size_t bytes, size_t alignment) {
    if (capacity() < bytes) {
        std::cout << "SmartMonoPool::do_allocate cannot allocate " << bytes
                  << " bytes" << std::endl;
        return nullptr;
    }
    void* ret = data();
    consume(bytes);
    add_ref();
    return ret;
}

inline void SmartMonoPool::do_deallocate(void* p, size_t bytes,
                                         size_t alignment) {
    // 仅要求原子序
    size_t old_cnt = ref_cnt.fetch_sub(1, std::memory_order_relaxed);
    // std::cout << "refcnt=" << ref_cnt << std::endl;
    assert(old_cnt != 0 || is_using);

    if (old_cnt == 1 && !is_using) {
        // 释放内存给上游
        release();
    }
}

inline bool SmartMonoPool::do_is_equal(
    const memory_resource& __other) const noexcept {
    return this == &__other;
}
