#pragma once

#include <cstring>

#include "./SmartMonoPool.h"

/*
管理一段连续内存，可以兼容上游SmartMonoPool/MemoryResource

需要持有提供内存资源的上游池指针，而非动态获取上游池指针
 */
class MsgView {
   public:
    const char* data_ = nullptr;
    size_t size_ = 0;

    MsgView(const char* data, size_t size, SmartMonoPool* upstream);
    MsgView(std::pmr::string&& str);  // 这种构造情况下禁止使用 add_ref 函数
    ~MsgView();

    MsgView(const MsgView& other) = delete;
    MsgView& operator=(const MsgView& other) = delete;

    MsgView(MsgView&& other) noexcept;
    MsgView& operator=(MsgView&& other) noexcept;

    void add_ref();
    void release();

   private:
    // 两套管理体系
    SmartMonoPool* upstream_ = nullptr;

    // 仅在 std::pmr::string 场景下使用，用于自动释放
    std::unique_ptr<std::pmr::string> object_;
};

inline MsgView::MsgView(const char* data, size_t size, SmartMonoPool* upstream)
    : data_(data), size_(size), upstream_(upstream) {
    upstream_->add_ref();
}

inline MsgView::MsgView(std::pmr::string&& str) : size_(str.size()) {
    object_ = std::make_unique<std::pmr::string>(std::move(str));
    data_ = object_->data();
}

inline MsgView::~MsgView() { release(); }

inline MsgView::MsgView(MsgView&& other) noexcept
    : data_(other.data_),
      size_(other.size_),
      upstream_(other.upstream_),
      object_(std::move(other.object_)) {
    other.upstream_ = nullptr;  // 防止重复释放
    other.data_ = nullptr;
}

inline MsgView& MsgView::operator=(MsgView&& other) noexcept {
    if (this != &other) {
        // 释放旧的
        if (upstream_ || object_) release();

        data_ = other.data_;
        size_ = other.size_;
        upstream_ = other.upstream_;
        object_ = std::move(other.object_);

        other.upstream_ = nullptr;
        other.data_ = nullptr;
    }
    return *this;
}

inline void MsgView::add_ref() {
    assert(upstream_);
    upstream_->add_ref();
}

inline void MsgView::release() {
    if (upstream_) {
        upstream_->deallocate(nullptr, 0, 0);
        upstream_ = nullptr;
        data_ = nullptr;
    } else {
        object_.reset();
    }
}
