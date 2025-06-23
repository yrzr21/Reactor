#include <atomic>
#include <barrier>
#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../../src/base/Buffer/BufferPool.h"
#include "../../src/base/Buffer/GlobalPool.h"

constexpr int THREAD_COUNT = 8;
constexpr int ALLOC_PER_THREAD = 10000;

// 每个线程各自使用独立的 BufferPool
void thread_worker(std::pmr::memory_resource* upstream, int thread_id,
                   std::barrier<>& start_barrier, std::atomic<int>& ok_count) {
    BufferPool pool(upstream, 512);  // 每线程独立 pool，使用共享上游资源

    std::vector<size_t> sizes = {64, 256, 1024};
    std::mt19937 rng(thread_id * 997);  // 每个线程不同种子
    std::uniform_int_distribution<> dist(0, 2);

    start_barrier.arrive_and_wait();  // 所有线程同步启动

    for (int i = 0; i < ALLOC_PER_THREAD; ++i) {
        size_t sz = sizes[dist(rng)];
        try {
            auto msg = pool.allocate_msg(sz);
            assert(msg && msg->size() == sz);
            (*msg)[0] = static_cast<char>(thread_id);  // 写入数据验证
            ok_count.fetch_add(1, std::memory_order_relaxed);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Thread " << thread_id
                      << " failed to allocate: " << e.what() << "\n";
        }
    }
}

int main() {
    GlobalPool global_pool(0.25);  // 256MB，总体 buffer，仅分配一次
    std::pmr::memory_resource* upstream = global_pool.get_resource();

    std::atomic<int> total_success{0};
    std::barrier sync_point(THREAD_COUNT);

    std::vector<std::thread> threads;

    auto start = std::chrono::steady_clock::now();

    // 启动线程，每线程拥有独立 BufferPool，复用共享内存资源
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back(thread_worker, upstream, i,
                             std::ref(sync_point), std::ref(total_success));
    }

    for (auto& t : threads) t.join();

    auto end = std::chrono::steady_clock::now();

    std::cout << "[OK] Allocations completed: " << total_success.load()
              << " in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " ms\n";

    return 0;
}
