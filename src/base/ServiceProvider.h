#pragma once

#include <memory_resource>

#include "../types.h"
#include "./Buffer/MonoRecyclePool.h"

struct ServiceProviderConfig {
    MemoryResource* upstream = nullptr;
    size_t chunk_size = 0;
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

    static MonoRecyclePool& getLocalMonoRecyclePool();

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
    static thread_local MonoRecyclePool local_pool_(getter, config_.chunk_size);
    return local_pool_;
}
