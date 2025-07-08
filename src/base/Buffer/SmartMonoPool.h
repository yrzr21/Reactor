#pragma once

#include <atomic>
#include <cassert>
#include <iostream>
#include <memory_resource>

#include "../../types.h"

/*
- 模拟monotonic_buffer_resource分配内存，但可用于未知大小对象分配，通过提供池内部指针的方式实现
- 基于引用计数，在合适时机可主动释放池

内存分配方式：
    1. allocate/deallocate：已知大小，自动修改计数并分配内存。deallocate 等价于减小引用计数
    2. data+consume+add_ref+deallocate：未知大小，自行管理计数与内存分配

行为：
    1. 模拟monotonic_buffer_resource，提供池内部指针用于未知大小对象分配
    2. 在!is_using且ref_cnt==0时自动释放内存池的内存
    3. 初始化时申请一次内存，其余时刻除非显式重新 init，不再向上级申请内存
    4. 持有提供内存资源的上游池指针，而非动态获取上游池指针
    5. 允许对已禁止分配的池增加引用计数，这一般通过已分配的内存实现
限制：
    1. do_allocate/init 必须在单线程内部进行
    2. 其余线程可修改引用计数，不应向此池申请内存
    3. 仅适用于一次性分配内存的线程，小心 string 的 SSO
    4. SmartMonoPool的持有者应保证SmartMonoPool的资源释放
 */

class SmartMonoPool : public MemoryResource {
   public:
    SmartMonoPool(MemoryResource* upstream, size_t chunk_size);
    ~SmartMonoPool();  // 应被 RAII 管理，类似于 malloc

    char* base();
    char* data();                // 下次写入的位置
    void consume(size_t bytes);  // 调用者确保消费的内存不会超过持有的内存
    size_t capacity();

    // 若为 nullptr 和 0，则继续使用原有 upstream
    // 调用者应确保已经 release
    void init(MemoryResource* upstream = nullptr, size_t chunk_size = 0);

    // 以下允许多线程访问，上面的不允许
    void add_ref();
    size_t ref();
    void set_unused();
    void
    release();  // 调用者需确保当前池有效，只有最后一个持有引用的线程可以调用它
    bool is_released();

   private:
    void* do_allocate(size_t bytes, size_t alignment) override;
    // 以下允许多线程访问，上面的不允许
    void do_deallocate(void* p, size_t bytes, size_t alignment) override;
    bool do_is_equal(const memory_resource& __other) const noexcept override;

   private:
    std::atomic<size_t> ref_cnt = 0;   // ref_cnt==0 && !is_using 时释放
    std::atomic_bool is_using = true;  // 禁止使用后不再分配内存
    std::atomic_bool released = true;  // 防止重复释放

    // 仅允许单线程修改
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
    assert(cur_ <= base() + total_size_);
}

inline void SmartMonoPool::add_ref() {
    // 仅要求原子性
    ref_cnt.fetch_add(1, std::memory_order_relaxed);
    // std::cout << "refcnt=" << ref_cnt << std::endl;
}

inline size_t SmartMonoPool::ref() {
    // 仅要求原子性
    return ref_cnt.load(std::memory_order_relaxed);
}

inline void SmartMonoPool::set_unused() {
    /*
    如果乱序先执行if，可能丢失释放时机：
        1.线程A：set_unused 先执行if，看到ref_cnt==1，不释放
        2.线程B：do_deallocate，看到is_using==true，修改ref_cnt==0，不释放，返回
        3.线程A：is_using==false，返回
    二者本应其中一个释放内存池，但都没有释放
    */

    // 禁止前后乱序
    is_using.store(false, std::memory_order_seq_cst);
    if (ref_cnt.load(std::memory_order_seq_cst) == 0) release();
}

inline size_t SmartMonoPool::capacity() {
    return total_size_ - static_cast<size_t>(cur_ - base());
}

inline bool SmartMonoPool::is_released() {
    return released.load(std::memory_order_relaxed);
}

inline void SmartMonoPool::release() {
    /*
        release 可能被多次进入，需要 flag is_released，保证只有一个线程进入
        成功cas(is_released==false)，阻止后续乱序
        失败(is_released==true)，允许后续乱序
    */
    bool expected = false;
    if (!released.compare_exchange_strong(expected, true,
                                          std::memory_order_acquire,
                                          std::memory_order_relaxed))
        return;  // released==true，直接返回

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

    ref_cnt.store(0, std::memory_order_relaxed);
    released.store(false, std::memory_order_relaxed);

    // 前面的不能跑到这后面，否则is_using为真但base_等变量的可能还没初始化
    is_using.store(true, std::memory_order_release);
}

inline void* SmartMonoPool::do_allocate(size_t bytes, size_t alignment) {
    // 如果进入if分支，那么if后的指令的提前执行都会作废，所以只保证原子性就行
    if (capacity() < bytes || !is_using.load(std::memory_order_relaxed)) {
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
    assert(old_cnt != 0);

    if (old_cnt == 1 && !is_using.load(std::memory_order_relaxed)) {
        // 释放内存给上游
        release();
    }
}

inline bool SmartMonoPool::do_is_equal(
    const memory_resource& __other) const noexcept {
    return this == &__other;
}
