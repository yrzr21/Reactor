#include <time.h>

#include "Timestamp.h"

Timestamp::Timestamp() { this->secSinceEpoch_ = time(0); }

Timestamp::Timestamp(int64_t secsinceepoch) : secSinceEpoch_(secsinceepoch) {}

// 当前时间。
Timestamp Timestamp::now() { return Timestamp(); }

time_t Timestamp::toint() const { return secSinceEpoch_; }

std::string Timestamp::tostring() const
{
    char buf[32] = {0};
    tm *tm_time = localtime(&this->secSinceEpoch_);
    snprintf(buf, 20, "%4d-%02d-%02d %02d:%02d:%02d",
             tm_time->tm_year + 1900,
             tm_time->tm_mon + 1,
             tm_time->tm_mday,
             tm_time->tm_hour,
             tm_time->tm_min,
             tm_time->tm_sec);
    return buf;
}