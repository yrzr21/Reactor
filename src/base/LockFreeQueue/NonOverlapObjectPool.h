#pragma once

#include <atomic>
#include <cassert>
#include <deque>
#include <map>

#include "LockFreeQueue.h"
#include "Region.h"
#include "VirtualRegionManager.h"

/* 服务器初始化的时候调用这个函数 */

/* 应保证对象大小小于4MB的区域大小 */

template <typename T>
class NonOverlapObjectPool {
   public:
    NonOverlapObjectPool();
    ~NonOverlapObjectPool();

    T* allocate();
    void deallocate(T* p);

    void setUnused();

   private:
    bool validAddr(T* p);
    void tryUnmap();
    void changeCurRegion();

   private:
    std::unique_ptr<Region> cur_region_ = nullptr;
    size_t cur_region_number_ = 0;
    size_t next_index_ = 0;
    bool is_using_ = true;

    /* region_number region */
    std::unordered_map<size_t, std::unique_ptr<Region>> wait_unmap_regions_;
};

template <typename T>
inline NonOverlapObjectPool<T>::NonOverlapObjectPool() {
    changeCurRegion();
}

template <typename T>
inline NonOverlapObjectPool<T>::~NonOverlapObjectPool() {
    tryUnmap();
    assert(wait_unmap_regions_.size() == 0);
    assert(cur_region_->ref() == 0);
}

template <typename T>
inline T* NonOverlapObjectPool<T>::allocate() {
    assert(is_using_);
    cur_region_->addRef();

    T* object_ptr = reinterpret_cast<T*>(cur_region_) + next_index_;
    next_index_++;
    if (sizeof(T) * next_index_ >= cur_region_->size()) {
        changeCurRegion();
    }

    return object_ptr;
}

template <typename T>
inline void NonOverlapObjectPool<T>::deallocate(T* p) {
    size_t region_number =
        VirtualRegionManager::addrRegion(reinterpret_cast<uintptr_t>(p));

    assert(validAddr(p));

    if (region_number == cur_region_number_) {
        cur_region_->reduceRef();
    } else {
        auto region_iter = wait_unmap_regions_.find(region_number);
        if (region_iter->second->ref() == 0)
            wait_unmap_regions_.erase(region_iter);
    }
}

template <typename T>
inline void NonOverlapObjectPool<T>::setUnused() {
    is_using_ = false;
    tryUnmap();
}

template <typename T>
inline bool NonOverlapObjectPool<T>::validAddr(T* p) {
    size_t region_number =
        VirtualRegionManager::addrRegion(reinterpret_cast<uintptr_t>(p));

    return region_number == cur_region_number_ ||
           wait_unmap_regions_.contains(region_number);
}

template <typename T>
inline void NonOverlapObjectPool<T>::changeCurRegion() {
    if (cur_region_) {
        wait_unmap_regions_.emplace(cur_region_number_, std::move(cur_region_));
    }

    uintptr_t new_region_start = VirtualRegionManager::allocRegion();
    cur_region_number_ = VirtualRegionManager::addrRegion(new_region_start);
    size_t bytes = VirtualRegionManager::regionBytes();
    cur_region_ = std::make_unique<Region>(new_region_start, bytes);
}

template <typename T>
inline void NonOverlapObjectPool<T>::tryUnmap() {
    if (wait_unmap_regions_.empty()) return;

    auto region_iter = wait_unmap_regions_.begin();
    while (region_iter != wait_unmap_regions_.end()) {
        if (region_iter->second->ref() == 0)
            region_iter = wait_unmap_regions_.erase(region_iter);
        else
            region_iter++;
    }
}
