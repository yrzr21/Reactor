#include "Buffer.h"

Buffer::Buffer(int initialSize) : buffer_(initialSize) {}

size_t Buffer::readableBytes() {
    if (full_) return capacity();

    if (writer_idx_ >= reader_idx_) {
        return writer_idx_ - reader_idx_;
    } else {
        return capacity() - reader_idx_ + writer_idx_;
    }
}

size_t Buffer::readableFromIdx(size_t idx) {
    if (idx >= capacity()) return 0;
    if (idx < reader_idx_ || idx >= writer_idx_) return 0;

    if (idx == reader_idx_ && full_) return capacity();

    if (writer_idx_ >= reader_idx_) {
        return writer_idx_ - idx;
    } else {
        return capacity() - idx + writer_idx_;
    }
}
size_t Buffer::writableBytes() { return capacity() - readableBytes(); }

// 扩容并整理缓冲 & 读写指针
// makeSpace(0)表示扩容两倍
void Buffer::makeSpace(size_t size) {
    if (writableBytes() >= size) return;

    // 扩容为2倍，或满足需求
    int newSize = std::max(capacity() * 2, capacity() + size);
    std::vector<char> newBuf(newSize);

    // 拷贝现有数据
    readBytes(newBuf.data(), 0, readableBytes());
    buffer_.swap(newBuf);

    reader_idx_ = 0;
    writer_idx_ = readable;
    full_ = false;
}

// 移动写指针来申请内存
// 调用前请确保有足够的数据可以读写
void Buffer::occupy(size_t size) {
    // if (size > writableBytes()) return false;
    writer_idx_ = (writer_idx_ + size) % capacity();

    full_ = (writer_idx_ == reader_idx_);
    return true;
}

// 调整读指针，回收内存
// 调用前请确保有足够的数据可以读写
void Buffer::retrieve(size_t size) {
    // if (size > readableBytes()) return false;
    reader_idx_ = (reader_idx_ + size) % capacity();

    full_ = false;
    return true;
}

// 查看
const char* Buffer::peek() { return begin() + reader_idx_; }
size_t Buffer::capacity() { return buffer_.size(); }
char* Buffer::begin() { return buffer_.data(); }

// 把数据读到 dest 偏移 offset 位置，不调整读指针
// 通过拷贝读写ring buffer，不调整读写指针
bool Buffer::readBytes(char* dest，size_t offset, size_t size) {
    if (size > readableBytes()) return false;
    dest += offset;

    size_t first = std::min(size, capacity() - reader_idx_);
    std::memcpy(dest, reader_idx_, first);

    if (first < size) {
        std::memcpy(dest + first, begin(), size - first);
    }
    return true;
}

// 把数据添加到ring buffer
// 通过拷贝读写ring buffer，不调整读写指针
bool Buffer::writeBytes(const char* src, size_t size) {
    if (size > writableBytes()) return false;

    size_t first = std::min(size, capacity() - writer_idx_);
    std::memcpy(begin() + writer_idx_, src, first);

    if (first < size) {
        std::memcpy(begin(), src, size - first);
    }
    return true;
}

void Buffer::updateLastHeader() {
    while (true) {
        // read header
        if (last_msg_idx_ == -1 && readableBytes() >= sizeof(Header)) {
            readBytes(&last_header_, 0, sizeof(Header));
            // last_header_.size = ntohl(last_header_.size);
            last_msg_idx_ = reader_idx_ + sizeof(Header);
        }

        // is msg complete?
        int msg_size = last_header_.size;
        if (last_msg_idx_ != -1 && readableFromIdx(last_msg_idx_) >= msg_size) {
            last_msg_idx_ = -1;
            last_header_.size = -1;
        }
    }
}

// // 把所有的数据都读到 buffer 里，对端关闭返回0
// ssize_t Buffer::readFd(int fd) {
//     char buffer[65536];

//     // ET，所以需要一次读取buffer大小，直到读完
//     uint32_t nread = 0;
//     while (true) {
//         // bzero(&buffer, sizeof(buffer));
//         int n = ::read(fd, buffer, sizeof(buffer));

//         if (n > 0) {
//             nread += n;
//             writeBytes(buffer, n);
//         } else if (n == 0) {
//             return 0;  // 对端关闭
//         } else {
//             if (errno == EINTR) continue;  // 被中断
//             // 读取完毕
//             if (errno == EAGAIN || errno == EWOULDBLOCK) return nread;
//             return -1;  // 其他错误
//         }
//     }
// }
// 把所有的数据都读到 buffer 里，对端关闭返回0
ssize_t Buffer::readFd(int fd) {
    // ET，所以需要一次读取buffer大小，直到读完
    uint32_t nread = 0;
    while (true) {
        int first;
        if (writer_idx_ > reader_idx_) {
            first = capacity() - writer_idx_;
        } else {
            first = reader_idx_ - writer_idx_;
        }

        // read
        int n = ::read(fd, begin() + writer_idx_, first);
        if (n == 0) {
            return 0;  // 对端关闭
        } else if (n < 0) {
            if (errno == EINTR) continue;  // 被中断
            // 读取完毕
            if (errno == EAGAIN || errno == EWOULDBLOCK) return nread;
            return -1;  // 其他错误
        }
        nread += n;
        occupy(n);

        updateLastHeader();

        // read header
        if (last_msg_idx_ == -1 && readableBytes() >= sizeof(Header)) {
            readBytes(&last_header_, 0, sizeof(Header));
            // last_header_.size = ntohl(last_header_.size);
            last_msg_idx_ = reader_idx_ + sizeof(Header);
        }

        // is msg complete?
        int msg_size = last_header_.size;
        if (last_msg_idx_ != -1 && readableFromIdx(last_msg_idx_) >= msg_size) {
            last_msg_idx_ = -1;
            last_header_.size = -1;
            return nread;
        }

        if (last_msg_idx_ != -1) {
        } else {
        }

        if (last_header_.size == -1 && n >= sizeof(Header)) {
            readBytes(&Header, 0, sizeof(Header));
            retrieve(sizeof(Header));
        }
        if (n < capacity() - writer_idx_) return nread;

        if (last_header_.size == -1) {
            if (writableBytes() < sizeof(Header)) makeSpace(0);
            n = ::read(fd, begin(), reader_idx_ - begin());
        }

        // 读取协议头
        if (readableBytes() + n >= sizeof(Header)) {
            readBytes(&Header, 0, sizeof(Header));
            retrieve(sizeof(Header));
        }
    }
}

// 取出下一个报文，没or不完整则返回空
std::string Buffer::nextMessage() {
    if (readableBytes() <= sizeof(Header)) return {};  // 没报文头

    // 读头部+转换字节序
    uint32_t netLen;
    readBytes(&netLen, 0, sizeof(Header));
    uint32_t len = ntohl(netLen);
    if (readableBytes() < len) return {};  // 长度不够
    retrieve(sizeof(Header));

    std::string msg(len);
    readBytes(msg.data(), 0, len);
    retrieve(len);

    return msg;
}
