#pragma once

#include <memory_resource>

#include "../types.h"
#include "./Buffer/MonoRecyclePool.h"

struct ServiceProviderConfig {
    MemoryResource* upstream;
    size_t chunk_size;
};

/*
全局服务点，其功能有：
    1. 为线程提供本地 MonoRecyclePool，而无需修改大量代码，如 ThreadPool
    工作线程的内存申请

松耦合的模型
 */

class ServiceProvider {
   public:
    ServiceProvider(ServiceProviderConfig&& config);

    static MonoRecyclePool& getLocalMonoRecyclePool();

   private:
    inline static ServiceProviderConfig config_;
};

inline ServiceProvider::ServiceProvider(ServiceProviderConfig&& config) {
    config_ = std::move(config);  // 静态成员不能用初始化列表
}

inline MonoRecyclePool& ServiceProvider::getLocalMonoRecyclePool() {
    static thread_local MonoRecyclePool local_pool_(config_.upstream,
                                                    config_.chunk_size);
    return local_pool_;
}
