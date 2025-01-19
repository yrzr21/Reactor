#ifndef BUFFER
#define BUFFER
#include <string>
#include <string.h>

class Buffer
{
private:
    std::string buffer_;
    const uint16_t messageType_; // 报文类型。0：无分割符，1：4字节长度报文头，2：\r\n\r\n分隔符

public:
    Buffer(uint16_t type = 1);
    ~Buffer();

    void append(const char *data, size_t size);
    void append(const std::string &data, size_t size);
    void appendMessage(const char *data, size_t size); // 自动添加4B长度报文头

    size_t size();
    const char *data();
    void clear();

    void erase(int pos, int n);

    std::string nextMessage(); // 从自己的buffer中取出下一个报文，若为空则无下一个完整报文
};

#endif // !BUFFER