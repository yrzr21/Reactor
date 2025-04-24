#include "Buffer.h"

Buffer::Buffer(int initialSize) : buffer_(initialSize) {}

size_t Buffer::readableSize() { return readableFromIdx(reader_idx_); }

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
size_t Buffer::writableBytes() { return capacity() - readableSize(); }

// 扩容并整理缓冲 & 读写指针
// makeSpace(0)表示扩容两倍
void Buffer::makeSpace(size_t size) {
    if (writableBytes() >= size) return;

    // 扩容为2倍，或满足需求
    int newSize = std::max(capacity() * 2, capacity() + size);
    std::vector<char> newBuf(newSize);

    // 拷贝现有数据
    size_t readable = readableSize();
    peekBytes(newBuf.data(), 0, readable);
    buffer_.swap(newBuf);

    reader_idx_ = 0;
    writer_idx_ = readable;
    full_ = false;
}

// 移动写指针来申请内存
// 调用前请确保有足够的数据可以读写
void Buffer::commitWrite(size_t size) {
    writer_idx_ = (writer_idx_ + size) % capacity();

    full_ = (writer_idx_ == reader_idx_);
}

// 调整读指针，回收内存
// 调用前请确保有足够的数据可以读写
void Buffer::consumeBytes(size_t size) {
    reader_idx_ = (reader_idx_ + size) % capacity();

    full_ = false;
}

// 查看
const char* Buffer::peek() { return data() + reader_idx_; }
size_t Buffer::capacity() { return buffer_.size(); }
char* Buffer::data() { return buffer_.data(); }

// 把数据读到 dest 偏移 offset 位置，不调整读指针
// 通过拷贝读写ring buffer，不调整读写指针
bool Buffer::peekBytes(char* dest, size_t offset, size_t size) {
    return peekBytesAt(reader_idx_, dest, offset, size);
}

bool Buffer::peekBytesAt(size_t idx, char* dest, size_t offset, size_t size) {
    if (readableFromIdx(idx) < size) return false;
    dest += offset;

    size_t first = std::min(size, capacity() - idx);
    std::memcpy(dest, data() + idx, first);

    if (first < size) {
        std::memcpy(dest + first, data(), size - first);
    }
    return true;
}

// 把数据添加到ring buffer
// 通过拷贝读写ring buffer，不调整读写指针
bool Buffer::pokeBytes(const char* src, size_t size) {
    if (size > writableBytes()) return false;

    size_t first = std::min(size, capacity() - writer_idx_);
    std::memcpy(data() + writer_idx_, src, first);

    if (first < size) {
        std::memcpy(data(), src, size - first);
    }
    return true;
}

// 更新 current_header_ 和 current_header_idx_
void Buffer::updateCurrentHeader() {
    while (true) {
        // no header, read header
        if (current_header_.size == -1) {
            if (readableFromIdx(current_header_idx_) < sizeof(Header)) return;
            peekBytesAt(current_header_idx_, (char*)&current_header_, 0,
                        sizeof(Header));
            // current_header_.size = ntohl(current_header_.size);
        }

        // is msg complete?
        if (!isCurrentMessageComplete()) return;

        // msg complete, update header
        size_t size = sizeof(Header) + current_header_.size;
        current_header_.size = -1;
        current_header_idx_ = (current_header_idx_ + size) % capacity();
    }
}

bool Buffer::isCurrentMessageComplete() {
    if (current_header_.size == -1) return false;

    size_t size = sizeof(Header) + current_header_.size;
    return readableFromIdx(current_header_idx_) >= size;
}

// 把所有的数据都读到 buffer 里，对端关闭返回0
ssize_t Buffer::fillFromFd(int fd) {
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
        int n = ::read(fd, data() + writer_idx_, first);
        if (n == 0) {
            return 0;  // 对端关闭
        } else if (n < 0) {
            if (errno == EINTR) continue;  // 被中断
            // 读取完毕
            if (errno == EAGAIN || errno == EWOULDBLOCK) return nread;
            return -1;  // 其他错误
        }
        nread += n;
        commitWrite(n);

        updateCurrentHeader();
    }
}

void Buffer::pushMessage(MessagePtr&& message) {
    uint32_t size = message->size();
    pokeBytes((char*)&size, sizeof(size));
    pokeBytes(message->data(), size);
}

// 取出下一个报文，没or不完整则返回空
std::string Buffer::popMessage() {
    if (readableSize() <= sizeof(Header)) return {};  // 没报文头

    // 读头部+转换字节序
    uint32_t netLen;
    peekBytes(&netLen, 0, sizeof(Header));
    uint32_t len = ntohl(netLen);
    if (readableSize() < len + sizeof(Header)) return {};  // 数据不完整

    consumeBytes(sizeof(Header));

    std::string msg(len);
    peekBytes(msg.data(), 0, len);
    consumeBytes(len);

    return msg;
}

// 读取数据，发送给fd
ssize_t Buffer::sendAllToFd(int fd) {
    ssize_t nsend = 0;

    size_t readable = readableSize();
    // if (readable == 0) return 0;

    size_t first = std::min(readable, capacity() - reader_idx_);
    ssize_t n = ::send(fd, peek(), first, 0);
    if (n == 0) return 0;
    if (n > 0) {
        consumeBytes(n);
        nsend += n;
    }

    if (first < readable) {
        ssize_t n = ::send(fd, data(), readable - first, 0);
        if (n == 0) return nsend;
        if (n > 0) {
            consumeBytes(n);
            nsend += n;
        }
    }

    return nsend;
}

bool Buffer::empty() { return readableSize() == 0; }