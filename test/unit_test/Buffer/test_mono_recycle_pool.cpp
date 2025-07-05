#include <cassert>
#include <cstring>
#include <iostream>
#include <memory_resource>
#include <vector>

#include "../../../src/base/Buffer/MonoRecyclePool.h"

int main() {
    constexpr size_t CHUNK_SIZE = 64;
    auto getter = [] {
        static std::pmr::monotonic_buffer_resource upstream;
        return &upstream;
    };
    MonoRecyclePool pool(getter, CHUNK_SIZE);

    std::cout << "\n==== Test: 初始写入 ====\n";

    // 模拟写入数据
    const char* msg1 = "Hello, SmartMonoPool!";
    size_t len1 = std::strlen(msg1) + 1;

    char* buf1 = pool.data();
    std::memcpy(buf1, msg1, len1);
    pool.consume(len1);

    // 记录当前资源
    auto* pool1 = static_cast<SmartMonoPool*>(pool.get_cur_resource());

    // 标记这段内存被引用
    pool1->add_ref();
    std::cout << "写入数据: " << buf1 << "\n";
    assert(std::strcmp(buf1, msg1) == 0);

    std::cout << "\n==== Test: 切换池子 ====\n";

    // 切换池子，旧池被标记 unused，但仍被引用，不会立即释放
    pool.change_pool();
    assert(!pool1->is_released());  // 还没释放

    std::cout << "\n==== Test: 引用归还 ====\n";

    // 模拟用户用完数据，归还引用
    pool1->deallocate(nullptr, 0, 0);
    assert(pool1->is_released());
    std::cout << "引用归还后旧池已释放\n";

    std::cout << "\n==== Test: 复用旧池 ====\n";

    // 此时新池再切换一次，应能回收刚才那个已释放的旧池
    pool.change_pool();
    assert(pool.get_cur_resource() == pool1);

    // 再写一次数据
    const char* msg2 = "Reused!";
    char* buf2 = pool.data();
    std::memcpy(buf2, msg2, std::strlen(msg2) + 1);
    pool.consume(std::strlen(msg2) + 1);

    std::cout << "复用池写入: " << buf2 << "\n";
    assert(std::strcmp(buf2, msg2) == 0);

    std::cout << "\n✅ 所有测试通过\n";
    return 0;
}
