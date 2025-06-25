#include <cassert>
#include <iostream>
#include <memory_resource>
#include "../../../src/base/Buffer/AutoReleasePool.h"

// 假设 AutoReleasePool 定义已包含在头文件中
// 使用单元测试框架或简单的 main 进行断言测试

int main() {
    constexpr size_t chunk_size = 1024;
    // 上游使用单次性缓冲资源
    std::pmr::monotonic_buffer_resource upstream(chunk_size);
    AutoReleasePool pool(&upstream, chunk_size);

    // 初始状态：base 非空，capacity == chunk_size
    assert(pool.base() != nullptr);
    assert(pool.capacity() == chunk_size);

    // 调用 consume 移动 cur_ 指针
    pool.consume(100);
    assert(pool.capacity() == chunk_size - 100);

    // 增加引用计数
    pool.add_ref();
    pool.add_ref();
    assert(pool.refCnt() == 2);

    // 释放一次（引用计数从2降到1）
    pool.deallocate(nullptr, 0);
    assert(pool.refCnt() == 1);
    assert(!pool.is_released());  // 还未释放内存

    // 标记为不再使用，然后再释放一次（引用计数变为0，触发释放）
    pool.set_unused();
    pool.deallocate(nullptr, 0);
    assert(pool.refCnt() == 0);
    assert(pool.is_released());  // 内存应已释放给上游

    std::cout << "AutoReleasePool 测试通过。\n";
    return 0;
}
