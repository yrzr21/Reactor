#pragma once

#include "./AutoReleasePool.h"

class MsgView {
   public:
    const char* data_;
    size_t size_;

    MsgView(const char* data, size_t size, AutoReleasePool* upstream);
    ~MsgView();

    MsgView(const MsgView& other);
    MsgView& operator=(const MsgView& other);

    MsgView(MsgView&& other) noexcept;
    MsgView& operator=(MsgView&& other) noexcept;

    void release();

   private:
    AutoReleasePool* upstream_;
};

inline MsgView::MsgView(const char* data, size_t size,
                        AutoReleasePool* upstream)
    : data_(data), size_(size), upstream_(upstream) {
    if (upstream_) upstream_->add_ref();
}

inline MsgView::~MsgView() { release(); }

inline MsgView::MsgView(const MsgView& other)
    : data_(other.data_), size_(other.size_), upstream_(other.upstream_) {
    upstream_->add_ref();
}
inline MsgView& MsgView::operator=(const MsgView& other) {
    if (this != &other) {
        // 释放旧的
        if (upstream_) release();

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
        if (upstream_) release();

        data_ = other.data_;
        size_ = other.size_;
        upstream_ = other.upstream_;
        upstream_->add_ref();
        other.release();
    }
    return *this;
}

inline void MsgView::release() {
    if (upstream_) {
        upstream_->deallocate(nullptr, 0, 0);
        upstream_ = nullptr;
        data_ = nullptr;
    }
}
