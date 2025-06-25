#pragma once

#include "./AutoReleasePool.h"

// 引用内存池中的数据，应尽快归还
// 自动增加减少内存池计数
struct MsgView {
    const char* data_;
    size_t size_;
    AutoReleasePool* upstream_;

    MsgView(const char* data, size_t size, AutoReleasePool* upstream)
        : data_(data), size_(size), upstream_(upstream) {
        upstream_->add_ref();
    }

    ~MsgView() {
        // 仅递减计数，AutoReleasePool 会自动释放
        if (upstream_) upstream_->deallocate(nullptr, 0, 0);
    }
};
