#include "NonOverlapObjectPool.h"

NonOverlapObjectPool::NonOverlapObjectPool() { changeCurRegion(); }

NonOverlapObjectPool::~NonOverlapObjectPool() {
    tryUnmap();
    assert(wait_unmap_regions_.size() == 0);
    assert(cur_region_->ref() == 0);
}

void* NonOverlapObjectPool::allocate(size_t bytes) {
    assert(is_using_);
    if (consumed_bytes_ + bytes > cur_region_->size()) {
        changeCurRegion();
    }

    cur_region_->addRef();
    uintptr_t addr =
        reinterpret_cast<uintptr_t>(cur_region_.get()) + consumed_bytes_;
    consumed_bytes_ += bytes;
    return reinterpret_cast<void*>(addr);
}

void NonOverlapObjectPool::deallocate(void* p) {
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

void NonOverlapObjectPool::setUnused() {
    is_using_ = false;
    tryUnmap();
}

bool NonOverlapObjectPool::validAddr(void* p) {
    size_t region_number =
        VirtualRegionManager::addrRegion(reinterpret_cast<uintptr_t>(p));

    return region_number == cur_region_number_ ||
           wait_unmap_regions_.contains(region_number);
}

void NonOverlapObjectPool::changeCurRegion() {
    if (cur_region_) {
        wait_unmap_regions_.emplace(cur_region_number_, std::move(cur_region_));
    }

    uintptr_t new_region_start = ServiceProvider::allocRegion();
    size_t bytes = VirtualRegionManager::regionBytes();
    cur_region_ = std::make_unique<Region>(new_region_start, bytes);

    cur_region_number_ = VirtualRegionManager::addrRegion(new_region_start);
    consumed_bytes_ = 0;
}

void NonOverlapObjectPool::tryUnmap() {
    if (wait_unmap_regions_.empty()) return;

    auto region_iter = wait_unmap_regions_.begin();
    while (region_iter != wait_unmap_regions_.end()) {
        if (region_iter->second->ref() == 0)
            region_iter = wait_unmap_regions_.erase(region_iter);
        else
            region_iter++;
    }
}
