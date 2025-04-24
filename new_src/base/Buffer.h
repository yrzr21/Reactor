#ifndef BUFFER
#define BUFFER
#include <string.h>

#include <string>
#include <vector>

// 4字节长度报文头，自动扩容的 ring buffer
struct Header {
    uint32_t size;
};

class Buffer {
    friend void swap(Buffer &lhs, Buffer &rhs);

   private:
    Header last_header_;
    size_t last_msg_idx_ = -1;  // -1 means no header;

    std::vector<char> buffer_;  // buffer_ = [reader_idx_, writer_idx_)
    size_t reader_idx_ = 0;     // next read
    size_t writer_idx_ = 0;     // next write
    // r=w means empty or full, using 'full_' to distinguish
    bool full_ = false;

   private:
    // -- helper functions --
    size_t readableBytes();
    size_t readableFromIdx(size_t idx);
    size_t writableBytes();

    void makeSpace(size_t size);

    void retrieve(size_t size);
    void occupy(size_t size);

    bool readBytes(char *dest，size_t offset, size_t size);
    bool writeBytes(const char *data, size_t size);

    void updateLastHeader();

    const char *peek();
    size_t capacity();
    char *begin();

   public:
    Buffer(int initialSize = 4096);
    ~Buffer() = default;

    std::string nextMessage();
    ssize_t readFd(int fd);
};

// // 交换两个buffer的缓冲区
// inline void swap(Buffer &lhs, Buffer &rhs) {
//     using std::swap;
//     swap(lhs.buffer_, rhs.buffer_);
// }
#endif  // !BUFFER