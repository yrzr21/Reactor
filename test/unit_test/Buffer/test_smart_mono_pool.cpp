#include <cassert>
#include <cstring>
#include <iostream>
#include <memory_resource>
#include <vector>

#include "../../../src/base/Buffer/SmartMonoPool.h"

constexpr size_t CHUNK_SIZE = 1024;

void print_info(SmartMonoPool& pool) {
    std::cout << "[SmartMonoPool] 容量: " << pool.capacity()
              << ", ref=" << pool.ref() << std::endl;
}

void test_normal() {
    std::pmr::monotonic_buffer_resource upstream;
    SmartMonoPool pool(&upstream, CHUNK_SIZE);
    // print_info(pool);

    {
        std::pmr::vector<char> vec(&pool);  // 会触发 add_ref()
        vec.resize(100);
        std::memcpy(vec.data(), "SmartMonoPool test vector", 26);
        // print_info(pool);

        assert(std::strcmp(vec.data(), "SmartMonoPool test vector") == 0);
        pool.add_ref();  // 模拟另一个使用者
        assert(pool.ref() == 2);
    }
    assert(pool.ref() == 1);
    pool.deallocate(nullptr, 0, 0);  // 另一使用者释放引用
    assert(!pool.is_released());
    assert(pool.ref() == 0);

    pool.set_unused();
    assert(pool.is_released());
    std::cout << "[SmartMonoPool] test_normal 通过\n";
}

void test_muti_thread() {
    constexpr size_t POOL_SIZE = 1024 * 1024;  // 1MB
    constexpr size_t ALLOC_COUNT = 50;
    constexpr int THREAD_COUNT = 100;

    std::pmr::unsynchronized_pool_resource upstream;
    SmartMonoPool pool(&upstream, POOL_SIZE);

    for (size_t i = 0; i < ALLOC_COUNT; i++) {
        pool.allocate(10, 0);
    }

    std::vector<std::thread> threads;
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads.emplace_back([&pool, i]() {
            // 多线程仅修改计数，不分配内存
            pool.add_ref();
            std::this_thread::sleep_for(std::chrono::milliseconds(10 + i * 5));
            pool.deallocate(nullptr, 0, 0);
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    pool.set_unused();
    for (size_t i = 0; i < ALLOC_COUNT; i++) pool.deallocate(nullptr, 0, 0);

    for (auto& t : threads) t.join();
    std::cout
        << "[SmartMonoPool] test_muti_thread: after all threads join, ref="
        << pool.ref() << std::endl;
    assert(pool.is_released());
    std::cout << "[SmartMonoPool] test_muti_thread 通过\n";
}

int main() {
    test_normal();
    test_muti_thread();

    std::cout << "[SmartMonoPool] 所有测试通过\n";
    return 0;
}
