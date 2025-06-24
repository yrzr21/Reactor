#pragma once

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory_resource>
#include <mutex>
#include <unordered_set>
#include <vector>

#include "../../types.h"

constexpr static size_t MAX_MSG_SIZE = 1024;
// 每个事件循环的 buffer pool，只支持单线程资源分配
class BufferPool {
   private:
    UnsynchronizedPool pool_;

   public:
    BufferPool(MemoryResource* upstream, size_t block_per_chunk)
        : pool_(PoolOptions{block_per_chunk, MAX_MSG_SIZE}, upstream) {}

    // 为了速度这么写，可维护性还算 ok
    std::unique_ptr<PmrCharVec> allocate_msg(size_t size) {
        // 支持三种大小,要求大的是小的的整数倍
        switch (size) {
            case 64:
            case 256:
            case MAX_MSG_SIZE:
                return std::make_unique<PmrCharVec>(size, &pool_);
            default:
                throw std::runtime_error("Unsupported message size");
        }
    }
    MemoryResource* get_resource() { return &pool_; }
};
·