#ifndef BUFFER
#define BUFFER
#include <string.h>

#include <string>
#include <vector>

// 4字节长度报文头，自动扩容的 ring buffer
struct Header {
    uint32_t size = -1;
};

class Buffer {
    friend void swap(Buffer &lhs, Buffer &rhs);

   private:
    // header of an uncomplete message
    // current_header_.size = -1 means no complete header yet
    Header current_header_;
    size_t current_header_idx_ = 0;

    std::vector<char> buffer_;  // buffer_ = [reader_idx_, writer_idx_)
    size_t reader_idx_ = 0;     // next read
    size_t writer_idx_ = 0;     // next write
    // r=w means empty or full, using 'full_' to distinguish
    bool full_ = false;

   private:
    // -- helper functions --
    size_t readableSize();
    size_t readableFromIdx(size_t idx);
    size_t writableBytes();

    void makeSpace(size_t size);

    void consumeBytes(size_t size);
    void commitWrite(size_t size);

    bool peekBytes(char *dest, size_t offset, size_t size);
    bool peekBytesAt(size_t idx, char *dest, size_t offset, size_t size);
    bool pokeBytes(const char *data, size_t size);

    void updateCurrentHeader();
    bool isCurrentMessageComplete();

    const char *peek();
    size_t capacity();
    char *data();

   public:
    Buffer(int initialSize = 4096);
    ~Buffer() = default;

    std::string popMessage();
    ssize_t fillFromFd(int fd);
};

// // 交换两个buffer的缓冲区
// inline void swap(Buffer &lhs, Buffer &rhs) {
//     using std::swap;
//     swap(lhs.buffer_, rhs.buffer_);
// }
#endif  // !BUFFER