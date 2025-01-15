#ifndef BUFFER
#define BUFFER
#include <string>
#include <string.h>

class Buffer
{
private:
    std::string buffer_;

public:
    Buffer();
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