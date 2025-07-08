#pragma once

#include <cstring>
#include <functional>
#include <memory_resource>

#include "../../types.h"
#include "./SmartMonoPool.h"

/*
- 用于管理SmartMonoPool的类，在SmartMonoPool内存不足时需手动切换新池，
旧池释放后自动进行回收
- 适用于一次性、单线程内存分配的情形，禁止用于多次、多线程内存分配
- 支持动态上游池，析构时检查wait池是否释放，并尝试释放当前池

使用方式：
    1. get_cur_resource 获取当前SmartMonoPool，需自行保证分配的内存不超过池上限
    2. move_to_new_pool/change_pool 换池子
    3. can_release 判断当前管理的所有 SmartMonoPool 是否可全部释放

todo：支持多线程内存分配
 */
class SmartMonoManager {
   public:
    SmartMonoManager(UpstreamProvider getter, size_t chunk_size);
    ~SmartMonoManager();

    // 单纯换池子，不做复制
    SmartMonoPool* change_pool();
    // 用该数据初始化新池子，可基于旧池子的数据
    // 若有不完整未分配数据在旧池子，用户应主动更新
    SmartMonoPool* move_to_new_pool(char* data, size_t bytes);
    bool can_release();
    SmartMonoPool* get_cur_resource();

    size_t capacity();  // 针对当前SmartMonoPool

   private:
    // 懒惰回收 wait pool，在new_pool中回收
    void recycle_pool();
    // 查看wait pool，释放的加入到 idle。从 idle 中取或者创建一个返回
    MsgPoolPtr new_pool();

   private:
    // 给新 SmartMonoPool 传参
    UpstreamProvider get_upstream_;
    size_t chunk_size_ = 0;

    // 等待释放的 pool，不再分配内存
    std::deque<MsgPoolPtr> wait_release_pools_;
    // 已释放的 SmartMonoPool 对象，可复用
    std::vector<MsgPoolPtr> idle_pools_;

    // 当前使用的对象，声明顺序得放在最下面
    MsgPoolPtr pool_ = new_pool();
};

inline SmartMonoManager::SmartMonoManager(UpstreamProvider getter,
                                          size_t chunk_size)
    : get_upstream_(std::move(getter)), chunk_size_(chunk_size) {
    assert(get_upstream_);
    assert(chunk_size_);
    // std::cout << "[tid=" << std::this_thread::get_id()
    //           << "] SmartMonoManager Constructed" << std::endl;
}

inline SmartMonoManager::~SmartMonoManager() {
    assert(can_release());
    if (!pool_->is_released()) pool_->release();
}

inline SmartMonoPool* SmartMonoManager::change_pool() {
    // std::cout << "[tid=" << std::this_thread::get_id()
    //           << "] SmartMonoManager::change_pool" << std::endl;
    MsgPoolPtr old_pool = std::move(pool_);
    pool_ = new_pool();

    old_pool->set_unused();
    wait_release_pools_.push_back(std::move(old_pool));

    return get_cur_resource();
}

inline SmartMonoPool* SmartMonoManager::move_to_new_pool(char* data,
                                                         size_t bytes) {
    // std::cout << "[tid=" << std::this_thread::get_id()
    //           << "] SmartMonoManager::move_to_new_pool" << std::endl;
    if (data >= pool_->base() && data <= pool_->data()) {
        assert(data + bytes <= pool_->data());
    }

    MsgPoolPtr old_pool = std::move(pool_);
    pool_ = new_pool();

    std::memcpy(pool_->data(), data, bytes);
    pool_->consume(bytes);

    old_pool->set_unused();
    wait_release_pools_.push_back(std::move(old_pool));

    return get_cur_resource();
}

inline bool SmartMonoManager::can_release() {
    recycle_pool();  // 由于懒回收，需要手动同步一下状态
    // std::cout << "[tid=" << std::this_thread::get_id()
    //           << "] can_release: ref=" << pool_->ref()
    //           << ", empty=" << wait_release_pools_.empty()
    //           << ", pool=" << static_cast<void*>(pool_->base())
    //           << ", capacity=" << pool_->capacity() << std::endl;
    return wait_release_pools_.empty() && pool_->ref() == 0;
}

inline SmartMonoPool* SmartMonoManager::get_cur_resource() {
    return pool_.get();
}

inline size_t SmartMonoManager::capacity() { return pool_->capacity(); }

inline MsgPoolPtr SmartMonoManager::new_pool() {
    // std::cout << "[tid=" << std::this_thread::get_id()
    //           << "] SmartMonoManager::new_pool" << std::endl;
    recycle_pool();

    if (idle_pools_.empty()) {
        // 构造时自动 init
        MsgPoolPtr ret =
            std::make_unique<SmartMonoPool>(get_upstream_(), chunk_size_);
        return ret;
    }

    // 使用 idle 中的返回
    MsgPoolPtr ret = std::move(idle_pools_.back());
    idle_pools_.pop_back();

    ret->init(get_upstream_(), chunk_size_);
    return ret;
}

inline void SmartMonoManager::recycle_pool() {
    // std::cout << "SmartMonoManager::recycle_pool" << std::endl;
    while (!wait_release_pools_.empty()) {
        if (!wait_release_pools_.front()->is_released()) break;

        idle_pools_.push_back(std::move(wait_release_pools_.front()));
        wait_release_pools_.pop_front();
    }
}
