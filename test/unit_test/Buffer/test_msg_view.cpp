#include <cassert>
#include <iostream>
#include <memory_resource>
#include <string>
#include <string_view>

#include "../../../src/base/Buffer/MsgView.h"
#include "../../../src/base/Buffer/SmartMonoPool.h"

int main() {
    static std::pmr::unsynchronized_pool_resource std_pool;
    SmartMonoPool pool(&std_pool, 1024);

    std::cout << "Initial ref count: " << pool.ref() << std::endl;

    // 通过SmartMonoPool分配内存块
    char* block1 = static_cast<char*>(pool.allocate(100, alignof(char)));
    assert(block1 != nullptr);
    std::cout << "Allocated 100 bytes, pool ref count: " << pool.ref()
              << std::endl;
    {
        // 创建MsgView，绑定这块内存
        MsgView view1(block1, 100, &pool);
        pool.deallocate(nullptr, 0, 0);  // for block1 ref
        std::cout << "After MsgView view1 created, ref count: " << pool.ref()
                  << std::endl;

        // 移动构造
        MsgView view3 = std::move(view1);
        std::cout
            << "After MsgView view3 move constructed from view1, ref count: "
            << pool.ref() << std::endl;

        // 移动赋值
        MsgView view5 = std::move(view3);
        std::cout << "After MsgView view5 move assigned from view3, ref count: "
                  << pool.ref() << std::endl;

        // 手动调用 add_ref 和 release
        view5.add_ref();  // 增加引用计数
        std::cout << "After add_ref on view5, ref count: " << pool.ref()
                  << std::endl;

        pool.deallocate(nullptr, 0, 0);  // 减少引用计数
        std::cout << "After release on view5, ref count: " << pool.ref()
                  << std::endl;
        pool.set_unused();
    }

    std::optional<MsgView> view6;
    void* addr1;
    {
        std::pmr::string pmr_str(&std_pool);
        pmr_str += "Hello, MsgView!";
        addr1 = pmr_str.data();
        view6.emplace(std::move(pmr_str));
    }

    std::cout << "addr1 = " << addr1 << std::endl;
    std::cout << "view6 data at "
              << static_cast<void*>(const_cast<char*>(view6->data_))
              << std::endl;

    std::cout << "MsgView6 created from pmr_string (with std_pool) = "
              << view6->data_ << std::endl;
    std::cout << "size=" << view6->size_ << std::endl;
    return 0;
}
