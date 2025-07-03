#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory_resource>
#include <thread>
#include <vector>

#include "../../../src/base/ServiceProvider.h"
#include "../../../src/base/ThreadPool.h"

// constexpr size_t CHUNK_SIZE = 1024;
constexpr size_t POOL_CAPACITY = 256;

// 线程任务，模拟多次申请内存，检查池容量
void taskFunction(int taskId) {
    auto& pool = ServiceProvider::getLocalMonoRecyclePool();

    std::cout << "Task " << taskId
              << ", thread id: " << std::this_thread::get_id()
              << ", MonoRecyclePool addr: " << &pool
              << ", capacity: " << pool.capacity() << std::endl;

    // 模拟分配 128 字节 3 次
    for (int i = 0; i < 3; ++i) {
        size_t before = pool.capacity();
        void* mem = pool.allocate(128);
        size_t after = pool.capacity();

        pool.deallocate();
        std::cout << "  Alloc 128 bytes, capacity before: " << before
                  << ", after: " << after
                  << ", pool: " << static_cast<void*>(pool.base()) << std::endl;
    }

    // 任务结束，池可能进入等待回收流程
}

int main() {
    // 创建一个全局上游内存资源，用于 MonoRecyclePool
    static char upstream_buf[POOL_CAPACITY * 10];
    static std::pmr::monotonic_buffer_resource upstream(upstream_buf,
                                                        sizeof(upstream_buf));

    // 初始化全局服务点配置
    ServiceProviderConfig config{&upstream, POOL_CAPACITY};
    ServiceProvider sp(std::move(config));

    // 创建线程池，比如4个线程
    ThreadPool pool(4, "worker");

    // 提交10个任务
    for (int i = 0; i < 10; ++i) {
        pool.addTask([i]() { taskFunction(i); });
    }

    // 等待所有任务完成
    pool.stopAll();

    std::cout << "Test finished." << std::endl;

    return 0;
}
