#include <cassert>
#include <cstring>
#include <iostream>
#include <memory_resource>

#include "../../../src/base/Buffer/MsgView.h"
#include "../../../src/base/Buffer/SmartMonoPool.h"

int main() {
    constexpr size_t chunk_size = 256;
    std::pmr::monotonic_buffer_resource upstream(chunk_size);
    SmartMonoPool pool(&upstream, chunk_size);

    // 在池中写入假数据
    char* data = pool.base();
    const char* msg1 = "hello";
    size_t len1 = 5;
    std::memcpy(data, msg1, len1);
    pool.consume(len1);

    // 创建 MsgView，引用计数自增
    MsgView mv1(data, len1, &pool);
    assert(pool.ref() == 1);

    // 复制构造（引用计数再增1）
    MsgView mv2 = mv1;
    assert(pool.ref() == 2);

    // 移动构造（原对象引用计数不变，新对象接管）
    MsgView mv3 = std::move(mv2);
    assert(pool.ref() == 2);
    // mv2.upstream_ 已被置空，不会再影响 pool.ref()

    // 销毁 mv1、mv3
    mv1.~MsgView();
    mv3.~MsgView();
    // 引用计数应归零
    assert(pool.ref() == 0);

    // 如果不再使用，则调用释放
    pool.set_unused();
    assert(pool.is_released());

    std::cout << "MsgView 测试通过。\n";
    return 0;
}
