#pragma once

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory_resource>
#include <mutex>
#include <unordered_set>
#include <vector>

#include "../../types.h"

constexpr size_t CHUNK_SIZE = 4096;

// 每个事件循环的 buffer pool，只支持单线程资源分配
// 应被上级RAII管理
class BufferPool {
   private:
    UnsynchronizedPool pool_;

   public:
    BufferPool(MemoryResource* upstream, size_t block_per_chunk)
        : pool_(PoolOptions{block_per_chunk, CHUNK_SIZE}, upstream) {}

    MemoryResource* get_resource() { return &pool_; }
};
