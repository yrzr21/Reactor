#pragma once
#include <arpa/inet.h>
#include <sys/uio.h>

#include "../../types.h"
#include "./MsgView.h"
#include "./SendBuffer.h"

// 发送时使用未完成的 send unit 构造 std::vector<iovec>
// SendUnit 自动管理内存
// 可使用多段内存拼接为一个send unit
class SendUnit {
   public:
    // 自动创建 header. upstream 是 header 的 upstream
    SendUnit(MsgView&& mv, MemoryResource* upstream);
    SendUnit(MsgVec&& mv, MemoryResource* upstream);
    ~SendUnit();

    SendUnit(const SendUnit& other) = delete;
    SendUnit& operator=(const SendUnit& other) = delete;
    SendUnit(SendUnit&& other) noexcept;
    SendUnit& operator=(SendUnit&& other) noexcept;

    IoVecs get_iovecs();           // 返回没发送完的部分
    size_t advance(size_t bytes);  // 消耗 bytes，返回消耗后的 bytes
    bool finished();

   private:
    void init_header();

   private:
    MemoryResource* upstream_ = nullptr;
    Header* header_base_ = nullptr;
    MsgVec msg_view_vec_;
    size_t nsent_ = 0;
};

inline SendUnit::SendUnit(MsgView&& mv, MemoryResource* upstream)
    : upstream_(upstream) {
    msg_view_vec_.push_back(std::move(mv));
    init_header();
}

inline SendUnit::SendUnit(MsgVec&& mvec, MemoryResource* upstream)
    : upstream_(upstream), msg_view_vec_(std::move(mvec)) {
    init_header();
}
inline SendUnit::~SendUnit() {
    assert(upstream_);
    assert(header_base_);
    upstream_->deallocate(header_base_, sizeof(Header));
    // msg_view_ 自动释放
}

inline SendUnit::SendUnit(SendUnit&& other) noexcept
    : upstream_(other.upstream_),
      header_base_(other.header_base_),
      msg_view_vec_(std::move(other.msg_view_vec_)),
      nsent_(other.nsent_) {
    other.upstream_ = nullptr;
    other.header_base_ = nullptr;
    other.nsent_ = -1;
}

inline SendUnit& SendUnit::operator=(SendUnit&& other) noexcept {
    if (this == &other) return *this;

    upstream_ = other.upstream_;
    header_base_ = other.header_base_;
    msg_view_vec_ = std::move(other.msg_view_vec_);
    nsent_ = other.nsent_;

    other.upstream_ = nullptr;
    other.header_base_ = nullptr;
    other.nsent_ = -1;

    return *this;
}

inline IoVecs SendUnit::get_iovecs() {
    if (finished()) return {};
    IoVecs ret;

    size_t offset = nsent_;
    // header
    if (offset < sizeof(Header)) {
        char* base = reinterpret_cast<char*>(header_base_) + offset;
        ret.emplace_back(base, sizeof(Header) - offset);
        offset = 0;
    } else {
        offset -= sizeof(Header);
    }

    // sent
    size_t start = 0;
    while (start < msg_view_vec_.size() &&
           offset >= msg_view_vec_[start].size_) {
        offset -= msg_view_vec_[start].size_;
        start++;
    }
    if (start >= msg_view_vec_.size()) return ret;

    // unsent
    // offset < msg_view_vec_[start].size_
    char* base = const_cast<char*>(msg_view_vec_[start].data_) + offset;
    ret.emplace_back(base, msg_view_vec_[start].size_ - offset);
    start++;

    while (start < msg_view_vec_.size()) {
        char* base = const_cast<char*>(msg_view_vec_[start].data_);
        ret.emplace_back(base, msg_view_vec_[start].size_);
        start++;
    }
    return ret;
}

inline size_t SendUnit::advance(size_t bytes) {
    size_t remain_to_sent = header_base_->size + sizeof(Header) - nsent_;
    size_t consumed = std::min(bytes, remain_to_sent);

    nsent_ += consumed;
    return bytes - consumed;
}

inline bool SendUnit::finished() {
    return nsent_ == header_base_->size + sizeof(Header);
}

inline void SendUnit::init_header() {
    assert(upstream_);
    header_base_ =
        reinterpret_cast<Header*>(upstream_->allocate(sizeof(Header)));
    header_base_->size = 0;
    for (const auto& mv : msg_view_vec_) header_base_->size += mv.size_;
}
