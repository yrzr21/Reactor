#pragma once

#include <sys/mman.h>
#include <unistd.h>

#include <atomic>
#include <cassert>

/*

负责映射，RAII，构造时映射，析构时取消映射

todo：处理映射失败发出的异常
todo：重合了怎么办？

*/
class Region {
   public:
    Region(uintptr_t start, size_t bytes);  // 自动页对齐
    ~Region();

    void* base();
    size_t size();
    size_t ref();

    void addRef();
    void reduceRef();

    Region(const Region&) = delete;
    Region& operator=(const Region&) = delete;
    Region(Region&& other) noexcept;
    Region& operator=(Region&& other) noexcept;

   private:
    void* p_ = nullptr;
    size_t bytes_ = 0;
    std::atomic<size_t> ref_cnt_ = 0;  // atomic?
};

inline Region::Region(uintptr_t start, size_t bytes)
    : p_(reinterpret_cast<void*>(start)), bytes_(bytes) {
    // 懒分配
    mmap(p_, bytes_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1,
         0);

    // if (p_ == MAP_FAILED) {
    //     throw std::runtime_error("mmap failed");
    // }
}

inline Region::~Region() {
    if (p_) munmap(p_, bytes_);
}

inline void* Region::base() { return p_; }

inline size_t Region::size() { return bytes_; }

inline size_t Region::ref() { return ref_cnt_; }

inline void Region::addRef() { ref_cnt_++; }

inline void Region::reduceRef() {
    assert(ref_cnt_);
    ref_cnt_--;
}

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
