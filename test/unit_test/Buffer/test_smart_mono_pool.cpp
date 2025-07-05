#include <cassert>
#include <cstring>
#include <iostream>
#include <memory_resource>
#include <vector>

#include "../../../src/base/Buffer/SmartMonoPool.h"

constexpr size_t CHUNK_SIZE = 1024;
std::pmr::monotonic_buffer_resource upstream;
SmartMonoPool pool(&upstream, CHUNK_SIZE);

void print_info() {
    std::cout << "[SmartMonoPool] 容量: " << pool.capacity()
              << ", ref=" << pool.ref() << std::endl;
}

int main() {
    print_info();

    {
        std::pmr::vector<char> vec(&pool);  // 会触发 add_ref()
        vec.resize(100);
        std::memcpy(vec.data(), "SmartMonoPool test vector", 26);
        print_info();

        assert(std::strcmp(vec.data(), "SmartMonoPool test vector") == 0);
        pool.add_ref();  // 模拟另一个使用者
        assert(pool.ref() == 2);
    }
    assert(pool.ref() == 1);
    pool.set_unused();

    // 仍可使用
    assert(!pool.is_released());
    {
        std::pmr::string s("goodbye", &pool);
        // 这里用的是 SSO，容量不变，也会正常增加索引
        print_info();

        assert(s == "goodbye");

        s.reserve(100);
        print_info();
        std::cout << "string capacity=" << s.capacity() << std::endl;
    }
    print_info();

    pool.deallocate(nullptr, 0, 0);  // 另一使用者释放引用

    pool.set_unused();  // 此时 ref=0 且 is_using=false，会触发 release
    assert(pool.is_released());

    std::cout << "[SmartMonoPool] 所有测试通过\n";
    return 0;
}
