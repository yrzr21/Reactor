#pragma once

#include "./AutoReleasePool.h"

struct MsgView {
    const char* data_;
    size_t size_;
    AutoReleasePool* upstream_;

    MsgView(const char* data, size_t size, AutoReleasePool* upstream);
    ~MsgView();

    MsgView(const MsgView& other);
    MsgView& operator=(const MsgView& other);

    MsgView(MsgView&& other) noexcept;
    MsgView& operator=(MsgView&& other) noexcept;
};

inline MsgView::MsgView(const char* data, size_t size,
                        AutoReleasePool* upstream)
    : data_(data), size_(size), upstream_(upstream) {
    if (upstream_) upstream_->add_ref();
}

inline MsgView::~MsgView() {
    if (upstream_) upstream_->deallocate(nullptr, 0, 0);
}

inline MsgView::MsgView(const MsgView& other)
    : data_(other.data_), size_(other.size_), upstream_(other.upstream_) {
    upstream_->add_ref();
}
inline MsgView& MsgView::operator=(const MsgView& other) {
    if (this != &other) {
        // 释放旧的
        if (upstream_) upstream_->deallocate(nullptr, 0, 0);

        data_ = other.data_;
        size_ = other.size_;
        upstream_ = other.upstream_;
        upstream_->add_ref();
    }
    return *this;
}

inline MsgView::MsgView(MsgView&& other) noexcept
    : data_(other.data_), size_(other.size_), upstream_(other.upstream_) {
    other.upstream_ = nullptr;  // 防止重复释放
    other.data_ = nullptr;
}

inline MsgView& MsgView::operator=(MsgView&& other) noexcept {
    if (this != &other) {
        // 释放旧的
        if (upstream_) upstream_->deallocate(nullptr, 0, 0);

        data_ = other.data_;
        size_ = other.size_;
        upstream_ = other.upstream_;
        other.upstream_ = nullptr;
        other.data_ = nullptr;
    }
    return *this;
}