#include "Buffer.h"

#include <iostream>

Buffer::Buffer(int initialSize) : buffer_(initialSize) {}

size_t Buffer::readableSize() {
    if (full_) return capacity();

    if (writer_idx_ >= reader_idx_) {
        return writer_idx_ - reader_idx_;
    } else {
        return capacity() - reader_idx_ + writer_idx_;
    }
}

size_t Buffer::writableBytes() { return capacity() - readableSize(); }

// 扩容并整理缓冲 & 读写指针
void Buffer::makeSpace() {
    std::vector<char> newBuf(capacity() * 2);

    // 拷贝现有数据
    size_t readable = readableSize();
    peekBytes(newBuf.data(), 0, readable);

    // 通过 swap 扩容
    buffer_.swap(newBuf);

    reader_idx_ = 0;
    writer_idx_ = readable;
    full_ = false;
    // std::cout << "make space to" << capacity() << std::endl;
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
    if (readableSize() < size) return false;
    dest += offset;

    size_t first = std::min(size, capacity() - reader_idx_);
    std::memcpy(dest, data() + reader_idx_, first);

    if (first < size) {
        std::memcpy(dest + first, data(), size - first);
    }
    return true;
}

// 把数据添加到ring buffer
// 通过拷贝读写ring buffer，不调整读写指针
bool Buffer::pokeBytes(const char* src, size_t size) {
    if (size > writableBytes()) {
        makeSpace();
    }

    size_t first = std::min(size, capacity() - writer_idx_);
    std::memcpy(data() + writer_idx_, src, first);

    if (first < size) {
        std::memcpy(data(), src, size - first);
    }
    return true;
}

// 解析并消费 header，不完整则不消费
void Buffer::parseHeader() {
    if (has_header_) return;

    size_t readable = readableSize();
    if (readable < sizeof(Header)) return;
    has_header_ = true;

    peekBytes((char*)&current_header_, 0, sizeof(Header));
    consumeBytes(sizeof(Header));

    current_header_.size = ntohl(current_header_.size);
}

bool Buffer::isCurrentMessageComplete() {
    if (!has_header_) return false;
    return readableSize() >= current_header_.size;
}

// 把所有的数据都读到 buffer 里，对端关闭返回0
ssize_t Buffer::fillFromFd(int fd) {
    // ET，所以需要一次读取buffer大小，直到读完
    // printf("Buffer::fillFromFd %d\n", fd);
    uint32_t nread = 0;
    while (true) {
        if (full_) makeSpace();

        size_t first = std::min(writableBytes(), capacity() - writer_idx_);
        // printf("Buffer::fillFromFd first=%d\n", first);

        // read
        int n = ::read(fd, data() + writer_idx_, first);
        if (n <= 0) {
            // 被中断
            if (n < 0 && errno == EINTR) continue;
            // 读取完毕，非阻塞 socket 上无数据可读
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
            // 对端关闭或致命错误
            return -1;
        }
        nread += n;

        // 不可移到外面统一 commit，每次循环都要借助读写指针判断状态
        commitWrite(n);
    }
    return nread;
}

// 自动加上报文头
void Buffer::pushMessage(MessagePtr&& message) {
    uint32_t size = message->size();
    uint32_t net_size = htonl(size);
    // printf("Buffer::pushMessage, net_size=%d,size=%d\n",net_size,size);

    pokeBytes((char*)&net_size, sizeof(net_size));
    commitWrite(sizeof(net_size));

    pokeBytes(message->data(), size);
    commitWrite(size);
}

// 取出下一个报文，没or不完整则返回空
std::string Buffer::popMessage() {
    parseHeader();
    if (!isCurrentMessageComplete()) return {};
    uint32_t msg_size = current_header_.size;

    std::string msg;
    msg.resize(msg_size);
    peekBytes(msg.data(), 0, msg_size);
    consumeBytes(msg_size);

    has_header_ = false;

    // std::cout << "popMessage: " << msg << std::endl;
    return msg;
}

// 读取数据，发送给fd
ssize_t Buffer::sendAllToFd(int fd) {
    ssize_t nsend = 0;
    size_t readable = readableSize();
    if (readable == 0) [[unlikely]]
        return 0;

    size_t first = std::min(readable, capacity() - reader_idx_);
    ssize_t n = ::send(fd, peek(), first, 0);
    if (n <= 0) {
        // 发送缓冲区满了
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return 0;
        // 致命错误 or 致命错误
        return -1;
    }

    consumeBytes(n);
    nsend += n;
    if (nsend < first || nsend == readable)
        return nsend;  // 发送缓冲区满了 or 发完了

    n = ::send(fd, data(), readable - first, 0);
    if (n <= 0) {
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) return nsend;
        return -1;
    }
    consumeBytes(n);
    nsend += n;

    return nsend;
}

bool Buffer::empty() { return readableSize() == 0; }