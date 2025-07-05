#include <cassert>
#include <iostream>
#include <memory_resource>
#include <string>
#include <string_view>

#include "../../../src/base/Buffer/MsgView.h"
#include "../../../src/base/Buffer/SmartMonoPool.h"

static std::pmr::unsynchronized_pool_resource std_pool;
SmartMonoPool pool(&std_pool, 1024);

void print_info() { std::cout << "ref=" << pool.ref() << std::endl; }

int main() {
    std::cout << "Initial ref count: " << pool.ref() << std::endl;

    // 通过SmartMonoPool分配内存块
    char* block1 = static_cast<char*>(pool.allocate(100, alignof(char)));
    assert(block1 != nullptr);
    {
        // 创建MsgView，绑定这块内存
        MsgView view1(block1, 100, &pool);
        print_info();                    // 2
        pool.deallocate(nullptr, 0, 0);  // for block1 ref

        // 移动构造
        MsgView view3 = std::move(view1);

        // 移动赋值
        MsgView view5 = std::move(view3);
        print_info();  // 1

        pool.set_unused();
    }

    std::optional<MsgView> view6;
    void* addr1;
    {
        std::pmr::string pmr_str(&std_pool);
        // avoid sso
        pmr_str += "Hello, MsgView!";
        pmr_str += "Hello, MsgView!";
        pmr_str += "Hello, MsgView!";
        pmr_str += "Hello, MsgView!";
        pmr_str += "Hello, MsgView!";
        addr1 = pmr_str.data();
        view6.emplace(std::move(pmr_str));
    }
    assert(addr1 == view6->data_);  // 移动所有权给 msg

    std::cout << "addr1 = " << addr1 << std::endl;
    std::cout << "view6 data at "
              << static_cast<void*>(const_cast<char*>(view6->data_))
              << std::endl;

    std::cout << "data = " << view6->data_ << ", size = " << view6->size_
              << std::endl;
    return 0;
}
