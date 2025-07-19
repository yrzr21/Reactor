#pragma once

#include <memory_resource>

#include "../types.h"
#include "./Buffer/SmartMonoManager.h"
#include "./Buffer/SyncPool.h"
#include "./LockFreeQueue/Region.h"
#include "./LockFreeQueue/VirtualRegionManager.h"
#include "./LockFreeQueue/NonOverlapObjectPool.h"

struct ServiceProviderConfig {
    MemoryResource* upstream = nullptr;
    // 应足以承载多个连接的一次申请的缓冲区，大小一般为缓冲区的整数倍
    size_t io_chunk_size = 0;
    // 当剩余容量低于这个阈值，更换当前内存池，大小一般为缓冲区的整数倍
    size_t io_min_remain_bytes = 0;

    // max_blocks_per_chunk largest_required_pool_block
    PoolOptions work_options = {40, 1024};
};

/*
全局服务点，其功能有：
    1. 为线程提供本地 SmartMonoManager/SyncPool，而无需修改大量代码

松耦合的模型
 */

class ServiceProvider {
   public:
    ServiceProvider(const ServiceProviderConfig& config);

    // 大块、一次性分配、大小各异内存
    static SmartMonoManager& getLocalMonoRecyclePool();

    // local 也要 sync，因为容器可能被其他线程访问
    static SyncPool& getLocalSyncPool();

    static Region allocRegion();
    static void* allocNonOverlap(size_t bytes);
    static void deallocNonOverlap(void* p);

   private:
    inline static ServiceProviderConfig config_;

    static thread_local NonOverlapObjectPool noop_;
};

inline ServiceProvider::ServiceProvider(const ServiceProviderConfig& config) {
    config_ = config;  // 静态成员不能用初始化列表
}

inline SmartMonoManager& ServiceProvider::getLocalMonoRecyclePool() {
    static UpstreamProvider getter = [upstream = config_.upstream] {
        return upstream;
    };
    static thread_local SmartMonoManager local_pool_(getter,
                                                     config_.io_chunk_size);
    // std::cout << "[tid=" << std::this_thread::get_id()
    //           << "] local mono manager = " <<
    //           static_cast<void*>(&local_pool_)
    //           << ", cur pool=" <<
    //           static_cast<void*>(local_pool_.get_cur_resource())
    //           << ", capacity=" << local_pool_.capacity();
    if (local_pool_.capacity() < config_.io_min_remain_bytes)
        local_pool_.change_pool();
    // std::cout << ", actual capacity=" << local_pool_.capacity() << std::endl;

    return local_pool_;
}

inline SyncPool& ServiceProvider::getLocalSyncPool() {
    static thread_local SyncPool local_pool_(config_.upstream,
                                             config_.work_options);
    // std::cout << "[tid=" << std::this_thread::get_id()
    //           << "] getLocalSyncPool, local_pool="
    //           << static_cast<void*>(&local_pool_) << std::endl;
    return local_pool_;
}

inline Region ServiceProvider::allocRegion() {
    static VirtualRegionManager vrm;
    uintptr_t addr = vrm.allocRegion();
    size_t bytes = VirtualRegionManager::regionBytes();
    return Region{addr, bytes};
}

inline void* ServiceProvider::allocNonOverlap(size_t bytes) {
    return noop_.allocate(bytes);
}

inline void ServiceProvider::deallocNonOverlap(void* p) { noop_.deallocate(p); }
