#include <cassert>
#include <iostream>
#include <memory_resource>
#include "../../../src/base/Buffer/GlobalPool.h"
#include "../../../src/base/Buffer/BufferPool.h"

int main() {
    // 测试 GlobalPool
    GlobalPool globalPool(1.0);  // 1GB
    std::pmr::memory_resource* gres = globalPool.get_resource();
    void* gp = gres->allocate(100);
    assert(gp != nullptr);
    gres->deallocate(gp, 100);
    std::cout << "GlobalPool 测试通过。\n";

    // 测试 BufferPool（假设 UnsynchronizedPool 可工作）
    BufferPool bufPool(gres, 2);  // upstream 还是使用 globalPool
    std::pmr::memory_resource* rres = bufPool.get_resource();
    void* bp1 = rres->allocate(50);
    void* bp2 = rres->allocate(50);
    assert(bp1 != nullptr && bp2 != nullptr);
    rres->deallocate(bp1, 50);
    rres->deallocate(bp2, 50);
    std::cout << "BufferPool 测试通过。\n";
    return 0;
}
