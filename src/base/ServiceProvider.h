#pragma once

#include <memory_resource>

#include "../types.h"
#include "./Buffer/MonoRecyclePool.h"
#include "./Buffer/SyncPool.h"

struct ServiceProviderConfig {
    MemoryResource* upstream = nullptr;
    size_t io_chunk_size = 0;

    // max_blocks_per_chunk largest_required_pool_block
    PoolOptions work_options = {40, 1024};
};

/*
全局服务点，其功能有：
    1. 为线程提供本地 MonoRecyclePool/SyncPool，而无需修改大量代码

松耦合的模型
 */

class ServiceProvider {
   public:
    ServiceProvider(const ServiceProviderConfig& config);

    // 大块、一次性分配、大小各异内存
    static MonoRecyclePool& getLocalMonoRecyclePool();

    // local 也要 sync，因为容器可能被其他线程访问
    static SyncPool& getLocalSyncPool();

   private:
    inline static ServiceProviderConfig config_;
};

inline ServiceProvider::ServiceProvider(const ServiceProviderConfig& config) {
    config_ = config;  // 静态成员不能用初始化列表
}

inline MonoRecyclePool& ServiceProvider::getLocalMonoRecyclePool() {
    static UpstreamProvider getter = [upstream = config_.upstream] {
        return upstream;
    };
    static thread_local MonoRecyclePool local_pool_(getter,
                                                    config_.io_chunk_size);
    // std::cout << "[tid=" << std::this_thread::get_id() << "] local mono pool = " << static_cast<void*>(&local_pool_)<<", base="
    //         << static_cast<void*>(local_pool_.base()) << std::endl;
    return local_pool_;
}

inline SyncPool& ServiceProvider::getLocalSyncPool() {
    static thread_local SyncPool local_pool_(config_.upstream,
                                             config_.work_options);
    return local_pool_;
}
