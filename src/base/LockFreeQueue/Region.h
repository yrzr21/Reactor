#pragma once

#include <sys/mman.h>
#include <unistd.h>

/*

负责映射，RAII，构造时映射，析构时取消映射

todo：处理映射失败发出的异常
todo：重合了怎么办？

*/
class Region {
   public:
    Region(size_t bytes);  // 自动页对齐
    ~Region();

    void* base();
    size_t size();

    Region(const Region&) = delete;
    Region& operator=(const Region&) = delete;
    Region(Region&& other) noexcept;
    Region& operator=(Region&& other) noexcept;

   private:
    static size_t page_align(size_t original_bytes);
    static bool overlap(const Region& l, const Region& r);  // 是否重合

   private:
    void* p_ = nullptr;
    size_t bytes_ = 0;
};

inline Region::Region(size_t bytes) : bytes_(Region::page_align(bytes)) {
    // 懒分配
    p_ = mmap(nullptr, bytes, PROT_READ | PROT_WRITE,
              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (p_ == MAP_FAILED) {
        throw std::runtime_error("mmap failed");
    }
}

inline Region::~Region() {
    if (p_) munmap(p_, bytes_);
}

inline void* Region::base() { return p_; }

inline size_t Region::size() { return bytes_; }

inline Region::Region(Region&& other) noexcept
    : p_(other.p_), bytes_(other.bytes_) {
    other.p_ = nullptr;
    other.bytes_ = 0;
}

inline Region& Region::operator=(Region&& other) noexcept {
    if (this != &other) {
        if (p_) munmap(p_, bytes_);
        p_ = other.p_;
        bytes_ = other.bytes_;
        other.p_ = nullptr;
        other.bytes_ = 0;
    }
    return *this;
}

inline size_t Region::page_align(size_t original_bytes) {
    size_t page_size = static_cast<size_t>(::getpagesize());
    return (original_bytes + page_size - 1) & ~(page_size - 1);
}

inline bool Region::overlap(const Region& l, const Region& r) { return false; }

