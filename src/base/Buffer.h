#ifndef BUFFER
#define BUFFER
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstring>
#include <string>
#include <vector>

#include "../types.h"

// 4字节长度报文头，自动扩容的 ring buffer
struct Header {
    uint32_t size = -1;
};

class Buffer {
    friend void swap(Buffer &lhs, Buffer &rhs);

   private:
    // header of an uncomplete message
    // current_header_.size = -1 means no complete header yet
    bool full_ = false;        // 是否已满
    bool has_header_ = false;  // header 是否有效
    Header current_header_;
    // size_t current_header_idx_ = 0;

    std::vector<char> buffer_;  // buffer_ = [reader_idx_, writer_idx_)
    size_t reader_idx_ = 0;     // next read
    size_t writer_idx_ = 0;     // next write
    // r=w means empty or full, using 'full_' to distinguish

   private:
    // -- helper functions --
    size_t readableSize();
    size_t writableBytes();

    void makeSpace();

    void consumeBytes(size_t size);
    void commitWrite(size_t size);

    bool peekBytes(char *dest, size_t offset, size_t size);
    // bool peekBytesAt(size_t idx, char *dest, size_t offset, size_t size);
    bool pokeBytes(const char *data, size_t size);

    void parseHeader();
    bool isCurrentMessageComplete();

    const char *peek();
    size_t capacity();
    char *data();

   public:
    Buffer(int initialSize = 4096);
    ~Buffer() = default;

    void pushMessage(MessagePtr &&message);
    std::string popMessage();

    ssize_t fillFromFd(int fd);
    ssize_t sendAllToFd(int fd);

    bool empty();
};

// // 交换两个buffer的缓冲区
// inline void swap(Buffer &lhs, Buffer &rhs) {
//     using std::swap;
//     swap(lhs.buffer_, rhs.buffer_);
// }
#endif  // !BUFFER