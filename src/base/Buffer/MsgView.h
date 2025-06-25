struct MsgView {
    const char* data_;
    size_t size_;
    AutoReleasePool* upstream_;

    MsgView(const char* data, size_t size, AutoReleasePool* upstream)
        : data_(data), size_(size), upstream_(upstream) {
        if (upstream_) upstream_->add_ref();
    }

    ~MsgView() {
        if (upstream_) upstream_->deallocate(nullptr, 0, 0);
    }

    // 禁用拷贝
    MsgView(const MsgView&) = delete;
    MsgView& operator=(const MsgView&) = delete;

    MsgView(MsgView&& other) noexcept
        : data_(other.data_), size_(other.size_), upstream_(other.upstream_) {
        other.upstream_ = nullptr;  // 防止重复释放
    }

    MsgView& operator=(MsgView&& other) noexcept {
        if (this != &other) {
            // 释放旧的
            if (upstream_) upstream_->deallocate(nullptr, 0, 0);

            data_ = other.data_;
            size_ = other.size_;
            upstream_ = other.upstream_;
            other.upstream_ = nullptr;
        }
        return *this;
    }
};
