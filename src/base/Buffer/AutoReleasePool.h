#pragma once

#include <atomic>
#include <iostream>
#include <memory_resource>

#include "../../types.h"
#include "./BufferPool.h"

// 下游仅使用本池的do_deallocate，用于减少引用，并返还内存
// 上游应尽快归还内存，以减少"碎片"

// 应被上级RAII管理
class AutoReleasePool : public MemoryResource {
   public:
    AutoReleasePool(MemoryResource* upstream, size_t chunk_size);
    ~AutoReleasePool();  // 应被 RAII 管理，类似于 malloc

    // buffer 使用如下公开接口修改内存池
    char* base();
    char* data();  // 下次写入的位置
    // buffer 确保消费的内存不会超过持有的内存
    void consume(size_t bytes);
    size_t capacity();
    bool is_released();
    void release();

    void add_ref();
    size_t refCnt();
    void set_unused();

    // 若为 nullptr 和 0，则继续使用原有 upstream
    // 调用者应确保已经 release
    void init(MemoryResource* upstream = nullptr, size_t chunk_size = 0);

   private:
    // 重写的 MemoryResource 接口

    // 禁止分配
    void* do_allocate(size_t bytes, size_t alignment) override;
    // 递减引用计数，为0时释放内存给上游
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

inline AutoReleasePool::AutoReleasePool(MemoryResource* upstream,
                                        size_t chunk_size) {
    init(upstream, chunk_size);
}

inline AutoReleasePool::~AutoReleasePool() { assert(is_released()); }
inline char* AutoReleasePool::base() { return base_; }
inline char* AutoReleasePool::data() { return cur_; }

inline void AutoReleasePool::consume(size_t bytes) { cur_ += bytes; }

inline void AutoReleasePool::add_ref() {
    // 仅要求原子序
    ref_cnt.fetch_add(1, std::memory_order_relaxed);
    std::cout << "refcnt=" << ref_cnt << std::endl;
}

inline size_t AutoReleasePool::refCnt() { return ref_cnt; }

inline void AutoReleasePool::set_unused() {
    is_using.store(false);
    if (ref_cnt == 0) release();
}

inline size_t AutoReleasePool::capacity() {
    return total_size_ - static_cast<size_t>(cur_ - base_);
}

inline bool AutoReleasePool::is_released() { return base_ == nullptr; }

inline void AutoReleasePool::release() {
    is_using.store(false);
    upstream_->deallocate(base_, total_size_);
    base_ = nullptr;
}

inline void AutoReleasePool::init(MemoryResource* upstream, size_t chunk_size) {
    assert(is_released());

    if (upstream) upstream_ = upstream;
    if (chunk_size) total_size_ = chunk_size;
    assert(upstream_);
    assert(total_size_);

    base_ = static_cast<char*>(upstream->allocate(chunk_size));
    cur_ = base_;
    ref_cnt.store(0);
    is_using.store(true);
}

void* AutoReleasePool::do_allocate(size_t bytes, size_t alignment) {
    throw std::bad_alloc();
}

inline void AutoReleasePool::do_deallocate(void* p, size_t bytes,
                                           size_t alignment) {
    // 仅要求原子序
    size_t old_cnt = ref_cnt.fetch_sub(1, std::memory_order_relaxed);
    std::cout << "refcnt=" << ref_cnt << std::endl;
    assert(old_cnt != 0 || is_using);

    if (old_cnt == 1 && !is_using) {
        // 释放内存给上游
        release();
    }
}

inline bool AutoReleasePool::do_is_equal(
    const memory_resource& __other) const noexcept {
    return this == &__other;
}
