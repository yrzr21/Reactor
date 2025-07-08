#pragma once

#include <atomic>
#include <memory_resource>

#include "../../types.h"

/*
- 基于unsynchronized_pool_resource，可在合适时机释放内存给上游以复用内存
- 适用于已知大小、多线程、同一对象多次进行的内存分配
- SSO 等不申请内存但仍可能在池生命周期结束后引用的情形须由使用者自行管理

没有用引用计数的方式分配内存，因为对于此类，对象申请和释放内存并不能和其生命周期绑定
 */

class SyncPool : public MemoryResource {
   public:
    SyncPool(MemoryResource* upstream, PoolOptions options);
    ~SyncPool();

    void request_release_prev();
    bool is_released();
    bool releasable();
    void release();  // 强制release

   private:
    void* do_allocate(size_t bytes, size_t alignment) override;
    void do_deallocate(void* p, size_t bytes, size_t alignment) override;
    bool do_is_equal(const memory_resource& __other) const noexcept override;

   private:
    std::atomic<size_t> bytes_in_use_ = 0;
    std::atomic_bool release_ = false;

    std::pmr::synchronized_pool_resource pool_;
};

inline SyncPool::SyncPool(MemoryResource* upstream, PoolOptions options)
    : pool_(options, upstream) {}

inline SyncPool::~SyncPool() {
    assert(bytes_in_use_.load(std::memory_order_relaxed) == 0);
    release(); // 析构时没设置release_也强制释放
}

inline void SyncPool::request_release_prev() {
    release_.store(true, std::memory_order_relaxed);
    if (releasable()) release();
}

inline void SyncPool::release() {
    pool_.release();  // 幂等，所以允许多个线程进入
}

inline bool SyncPool::releasable() {
    return release_.load(std::memory_order_relaxed) &&
           bytes_in_use_.load(std::memory_order_relaxed) == 0;
}

inline void* SyncPool::do_allocate(size_t bytes, size_t alignment) {
    if (release_.load(std::memory_order_relaxed)) {
        std::cout << "SyncPool::do_allocate failed" << std::endl;
        return nullptr;
    }

    bytes_in_use_.fetch_add(bytes, std::memory_order_relaxed);
    return pool_.allocate(bytes, alignment);
}

inline void SyncPool::do_deallocate(void* p, size_t bytes, size_t alignment) {
    // releasable 不会重排到之前，因为依赖，可以忽略内存序
    bytes_in_use_.fetch_sub(bytes, std::memory_order_relaxed);
    pool_.deallocate(p, bytes, alignment);

    if (releasable()) release();
}

inline bool SyncPool::do_is_equal(
    const memory_resource& __other) const noexcept {
    return this == &__other;
}
