#include <cassert>
#include <iostream>
#include <memory_resource>
#include <thread>
#include <vector>

#include "../../../src/base/Buffer/SyncPool.h"

constexpr size_t THREAD_COUNT = 100;
constexpr size_t ALLOC_UNIT_BYTES = 64;
constexpr size_t ALLOC_PER_THREAD = 1000;

class SimpleUpstream : public std::pmr::memory_resource {
   public:
    ~SimpleUpstream() { assert(bytes_in_use == 0); }

   protected:
    std::atomic<size_t> bytes_in_use = 0;

    void* do_allocate(size_t bytes, size_t alignment) override {
        bytes_in_use += bytes;
        return ::operator new(bytes);
    }

    void do_deallocate(void* p, size_t bytes, size_t alignment) override {
        bytes_in_use -= bytes;
        ::operator delete(p);
    }

    bool do_is_equal(
        const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

void test_basic() {
    SimpleUpstream upstream;
    PoolOptions options{.max_blocks_per_chunk = 1000,
                        .largest_required_pool_block = ALLOC_UNIT_BYTES};
    SyncPool pool(&upstream, options);

    std::vector<void*> allocated[THREAD_COUNT];
    std::vector<std::thread> workers;
    for (size_t t = 0; t < THREAD_COUNT; ++t) {
        workers.emplace_back([&, t]() {
            for (size_t i = 0; i < ALLOC_PER_THREAD; ++i) {
                void* p = pool.allocate(ALLOC_UNIT_BYTES);
                assert(p != nullptr);
                std::this_thread::sleep_for(std::chrono::microseconds(10));
                allocated[t].push_back(p);
            }
        });
    }
    for (auto& th : workers) th.join();

    pool.request_release_prev();
    assert(!pool.releasable());

    for (size_t t = 0; t < THREAD_COUNT; ++t) {
        for (void* p : allocated[t]) {
            pool.deallocate(p, ALLOC_UNIT_BYTES);
        }
    }

    // 这里应该失败
    void* should_be_null = pool.allocate(ALLOC_UNIT_BYTES);
    assert(should_be_null == nullptr);

    std::cout << "[PASS] SyncPool basic test passed.\n";
}

void test_container() {
    SimpleUpstream upstream;
    PoolOptions options{.max_blocks_per_chunk = 1000,
                        .largest_required_pool_block = ALLOC_UNIT_BYTES};
    SyncPool pool(&upstream, options);

    std::vector<void*> allocated[THREAD_COUNT];
    std::vector<std::thread> workers;
    for (size_t t = 0; t < THREAD_COUNT; ++t) {
        workers.emplace_back([&, t]() {
            std::vector<std::pmr::string> allocated;
            for (size_t i = 0; i < ALLOC_PER_THREAD; ++i) {
                allocated.emplace_back(&pool);
                allocated[i].resize(ALLOC_UNIT_BYTES);
                assert(allocated[i].capacity() == ALLOC_UNIT_BYTES);

                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10 + t));
            assert(!pool.releasable());
        });
    }
    for (auto& th : workers) th.join();

    pool.request_release_prev();
    assert(pool.releasable());

    // 这里应该失败
    void* should_be_null = pool.allocate(ALLOC_UNIT_BYTES);
    assert(should_be_null == nullptr);

    std::cout << "[PASS] SyncPool container test passed.\n";
}

int main() {
    test_container();
    return 0;
}
