#pragma once

#include <sys/types.h>

#include <memory>
#include <memory_resource>

#include "../../types.h"

// 仅允许一次申请？
class GlobalPool {
   private:
    std::unique_ptr<char[]> buffer_;
    MonotonicPool pool_;
    SynchronizedPool s_pool_;  // 包装 MonotonicPool

   public:
    // PoolOptions{1, 0} 表示 SynchronizedPool 任何资源请求都直接丢给
    // MonotonicPool 且 SynchronizedPool 维护了线程安全
    GlobalPool(double size_gb)
        : buffer_(std::make_unique<char[]>(size_gb * 1024 * 1024 * 1024)),
          pool_(buffer_.get(), size_gb * 1024 * 1024 * 1024),
          s_pool_(PoolOptions{1, 0}, &pool_) {}
    ~GlobalPool() = default;

    MemoryResource* get_resource() { return &s_pool_; }

    // 禁止拷贝禁止移动，全局唯一
    GlobalPool(const GlobalPool&) = delete;
    GlobalPool& operator=(const GlobalPool&) = delete;
    GlobalPool(GlobalPool&&) = delete;
    GlobalPool& operator=(GlobalPool&&) = delete;
};
