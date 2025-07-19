#pragma once

#include <atomic>
#include <cassert>
#include <deque>
#include <map>

#include "Region.h"
#include "VirtualRegionManager.h"
#include "../ServiceProvider.h"

/* 服务器初始化的时候调用这个函数 */

/* 应保证对象大小小于4MB的区域大小 */

/*

为保证速度，分配不加锁，可设为线程私有避免竞态

行为逻辑与 SmartMonoPool 类似，但是用mmap

*/

class NonOverlapObjectPool {
   public:
    NonOverlapObjectPool();
    ~NonOverlapObjectPool();

    void* allocate(size_t bytes);
    void deallocate(void* p);

    void setUnused();

   private:
    bool validAddr(void* p);
    void tryUnmap();
    void changeCurRegion();

   private:
    std::unique_ptr<Region> cur_region_ = nullptr;
    size_t cur_region_number_ = 0;
    size_t consumed_bytes_ = 0;
    bool is_using_ = true;

    /* region_number region */
    std::unordered_map<size_t, std::unique_ptr<Region>> wait_unmap_regions_;
};
