#pragma once

#include <cstring>

#include "./SmartMonoPool.h"

template <typename PmrContainer>
void* detach_pmr_ownership(PmrContainer&& container) {
    auto* raw = new PmrContainer{std::move(container)};
    // 通过泄露的方式取消控制权，调用者应自行管理容器内存
    return raw->data();
}

/*
管理一段连续内存，可以兼容上游SmartMonoPool/MemoryResource

需要持有提供内存资源的上游池指针，而非动态获取上游池指针
 */
class MsgView {
    constexpr static size_t SSO_THRESHOLD = 32;

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

    // 适用于不通过 MsgView 构造，但仍想指向该块内存的对象
    void add_ref();
    void release();

   private:
    MemoryResource* upstream_ = nullptr;
    SmartMonoPool* upstream_smp_ = nullptr;  // 特殊功能的池
};

inline MsgView::MsgView(const char* data, size_t size, SmartMonoPool* upstream)
    : data_(data), size_(size), upstream_(upstream), upstream_smp_(upstream) {
    assert(upstream_smp_);
    upstream_smp_->add_ref();
}

inline MsgView::MsgView(std::pmr::string&& str)
    : size_(str.size()), upstream_(str.get_allocator().resource()) {
    if (size_ < SSO_THRESHOLD) {
        // 小于 SSO 阈值，直接拷贝
        // std::cout << "SSO" << std::endl;
        char* copy = static_cast<char*>(upstream_->allocate(size_));
        std::memcpy(copy, str.data(), size_);
        data_ = copy;
    } else {
        data_ = static_cast<const char*>(detach_pmr_ownership(std::move(str)));
    }
}

inline MsgView::~MsgView() { release(); }

inline MsgView::MsgView(MsgView&& other) noexcept
    : data_(other.data_),
      size_(other.size_),
      upstream_(other.upstream_),
      upstream_smp_(other.upstream_smp_) {
    other.upstream_ = nullptr;  // 防止重复释放
    other.upstream_smp_ = nullptr;
    other.data_ = nullptr;
}

inline MsgView& MsgView::operator=(MsgView&& other) noexcept {
    if (this != &other) {
        // 释放旧的
        if (upstream_) release();

        data_ = other.data_;
        size_ = other.size_;
        upstream_ = other.upstream_;
        upstream_smp_ = other.upstream_smp_;

        other.upstream_ = nullptr;
        other.upstream_smp_ = nullptr;
        other.data_ = nullptr;
    }
    return *this;
}

inline void MsgView::add_ref() {
    assert(upstream_smp_);
    upstream_smp_->add_ref();
}

inline void MsgView::release() {
    if (upstream_) {
        upstream_->deallocate(nullptr, 0, 0);
        upstream_ = nullptr;
        upstream_smp_ = nullptr;
        data_ = nullptr;
    }
}
