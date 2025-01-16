#include "Buffer.h"

Buffer::Buffer(/* args */)
{
}

Buffer::~Buffer()
{
}

void Buffer::append(const char *data, size_t size)
{
    this->buffer_.append(data, size);
}

void Buffer::append(const std::string &data, size_t size)
{
    this->buffer_.append(data, 0, size);
}

void Buffer::appendMessage(const char *data, size_t size)
{
    this->buffer_.append((char *)&size, 4);
    this->buffer_.append(data, size);
    printf("append message, buffer = %s\n", this->buffer_.c_str());
}

size_t Buffer::size()
{
    return this->buffer_.size();
}

const char *Buffer::data()
{
    return this->buffer_.data();
}

void Buffer::clear()
{
    this->buffer_.clear();
}

void Buffer::erase(int pos, int n)
{
    this->buffer_.erase(pos, n);
}

// 采用指定报文长度的方式
std::string Buffer::nextMessage()
{
    uint32_t len;
    memcpy(&len, this->buffer_.data(), 4);
    if (this->buffer_.size() < len + 4) // 不完整报文
        return {};

    std::string ret = this->buffer_.substr(4, len);
    this->buffer_.erase(0, len + 4);
    return ret;
}
