#pragma once

#include <memory_resource>

#include "../../types.h"
#include "./SmartMonoPool.h"

/*
封装 SmartMonoPool，注意，它不继承自memory_resource

操作与 SmartMonoPool 类似

应被上级RAII管理，即所有资源引用计数应为0 再析构

 */
class MonoRecyclePool {
   public:
    MonoRecyclePool(MemoryResource* upstream, size_t chunk_size);
    ~MonoRecyclePool();

    // 单纯换池子，不做复制
    void change_pool();
    // 用该数据初始化新池子，可基于旧池子的数据
    void move_to_new_pool(char* data, size_t bytes);
    bool can_release();
    SmartMonoPool* get_cur_resource();

    // 以下操作调用当前pool的接口
    void* allocate(size_t bytes);
    void deallocate();

    size_t capacity();
    char* base();
    char* data();
    void consume(size_t bytes);

    void add_cur_ref();  // 资源对池的ref
    size_t ref();

   private:
    // 懒惰回收 wait pool，在new_pool中回收
    void recycle_pool();
    // 查看wait pool，释放的加入到 idle。从 idle 中取或者创建一个返回
    MsgPoolPtr new_pool();

   private:
    // 给新 SmartMonoPool 传参
    MemoryResource* upstream_ = nullptr;
    size_t chunk_size_ = 0;

    // 等待释放的 pool，不再分配内存
    std::deque<MsgPoolPtr> wait_release_pools_;
    // 已释放的 SmartMonoPool 对象，可复用
    std::vector<MsgPoolPtr> idle_pools_;

    // 当前使用的对象，声明顺序得放在最下面
    MsgPoolPtr pool_ = new_pool();
};

inline MonoRecyclePool::MonoRecyclePool(MemoryResource* upstream,
                                        size_t chunk_size)
    : upstream_(upstream), chunk_size_(chunk_size) {
    assert(upstream_);
    assert(chunk_size_);
}

inline MonoRecyclePool::~MonoRecyclePool() {
    assert(can_release());
    if (!pool_->is_released()) pool_->release();
}

inline void MonoRecyclePool::change_pool() {
    MsgPoolPtr old_pool = std::move(pool_);
    pool_ = new_pool();

    old_pool->set_unused();
    wait_release_pools_.push_back(std::move(old_pool));
}

inline void MonoRecyclePool::move_to_new_pool(char* data, size_t bytes) {
    if (data >= pool_->base() && data <= pool_->data()) {
        assert(data + bytes <= pool_->data());
    }

    MsgPoolPtr old_pool = std::move(pool_);
    pool_ = new_pool();

    std::memcpy(pool_->data(), data, bytes);
    pool_->consume(bytes);

    old_pool->set_unused();
    wait_release_pools_.push_back(std::move(old_pool));
}

inline bool MonoRecyclePool::can_release() {
    recycle_pool();  // 由于懒回收，需要手动同步一下状态
    return wait_release_pools_.empty() && pool_->ref() == 0;
}

inline SmartMonoPool* MonoRecyclePool::get_cur_resource() {
    return pool_.get();
}

inline void* MonoRecyclePool::allocate(size_t bytes) {
    if (pool_->capacity() < bytes) change_pool();
    return pool_->allocate(bytes);
}

inline void MonoRecyclePool::deallocate() { pool_->deallocate(nullptr, 0, 0); }

inline size_t MonoRecyclePool::capacity() { return pool_->capacity(); }

inline char* MonoRecyclePool::base() { return pool_->base(); }

inline char* MonoRecyclePool::data() { return pool_->data(); }

inline void MonoRecyclePool::consume(size_t bytes) { pool_->consume(bytes); }

inline void MonoRecyclePool::add_cur_ref() { pool_->add_ref(); }

inline size_t MonoRecyclePool::ref() { return pool_->ref(); }

inline MsgPoolPtr MonoRecyclePool::new_pool() {
    // std::cout << "MonoRecyclePool::new_pool" << std::endl;
    recycle_pool();

    if (idle_pools_.empty()) {
        // 构造时自动 init
        MsgPoolPtr ret =
            std::make_unique<SmartMonoPool>(upstream_, chunk_size_);
        return ret;
    }

    // 使用 idle 中的返回
    MsgPoolPtr ret = std::move(idle_pools_.back());
    idle_pools_.pop_back();

    ret->init();
    return ret;
}

inline void MonoRecyclePool::recycle_pool() {
    // std::cout << "MonoRecyclePool::recycle_pool" << std::endl;
    while (!wait_release_pools_.empty()) {
        if (!wait_release_pools_.front()->is_released()) break;

        idle_pools_.push_back(std::move(wait_release_pools_.front()));
        wait_release_pools_.pop_front();
    }
}
