#include <cassert>
#include <cstring>
#include <iostream>
#include <memory_resource>
#include <vector>

#include "../../../src/base/Buffer/SmartMonoPool.h"

int main() {
    constexpr size_t CHUNK_SIZE = 1024;
    std::pmr::monotonic_buffer_resource upstream;

    SmartMonoPool pool(&upstream, CHUNK_SIZE);

    std::cout << "[test] 初始容量: " << pool.capacity() << "\n";

    {
        std::pmr::vector<char> vec(&pool);  // 会触发 add_ref()
        vec.resize(100);
        std::memcpy(vec.data(), "SmartMonoPool test vector", 26);

        pool.add_ref();  // 模拟另一个使用者

        std::cout << "[test] vec 写入成功，内容: " << vec.data() << "\n";
        assert(std::strcmp(vec.data(), "SmartMonoPool test vector") == 0);

        // 注意此时 ref == 2（vec + 手动 add_ref）
        assert(pool.ref() == 2);
    }

    std::cout << "[test] vec 析构后 ref = " << pool.ref() << "\n";
    assert(pool.ref() == 1);

    // 设置为 unused，尚未触发释放
    pool.set_unused();
    assert(!pool.is_released());

    // 最后再释放一次引用
    {
        std::pmr::string s("goodbye", &pool);
        assert(s == "goodbye");
        std::cout << "[test] 构造 string 成功\n";
    }

    pool.deallocate(nullptr, 0, 0);  // 另一使用者释放引用

    pool.set_unused();  // 此时 ref=0 且 is_using=false，会触发 release
    assert(pool.is_released());

    std::cout << "[test] 所有测试通过\n";
    return 0;
}
