#pragma once

#include <memory_resource>

#include "../types.h"
#include "./Buffer/MonoRecyclePool.h"

struct ServiceProviderConfig {
    MemoryResource* upstream = nullptr;
    size_t io_chunk_size = 0;

    // max_blocks_per_chunk largest_required_pool_block
    PoolOptions work_options = {40, 1024};
};

/*
全局服务点，其功能有：
    1. 为线程提供本地 MonoRecyclePool，而无需修改大量代码，如 ThreadPool
    工作线程的内存申请

松耦合的模型
 */

class ServiceProvider {
   public:
    ServiceProvider(const ServiceProviderConfig& config);

    // 大块、一次性分配、大小各异内存
    static MonoRecyclePool& getLocalMonoRecyclePool();
    // 小块、多次分配、内存
    static UnsynchronizedPool& getLocalUnsyncPool();

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
    return local_pool_;
}

inline UnsynchronizedPool& ServiceProvider::getLocalUnsyncPool() {
    static thread_local UnsynchronizedPool local_pool_(config_.work_options,
                                                       config_.upstream);
    return local_pool_;
}
