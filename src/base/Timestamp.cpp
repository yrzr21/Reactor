#include "Timestamp.h"


time_t Timestamp::toint() const { return second_; }

std::string Timestamp::tostring() const {
    char buf[32] = {0};
    tm *tm_time = localtime(&this->second_);
    snprintf(buf, 20, "%4d-%02d-%02d %02d:%02d:%02d", tm_time->tm_year + 1900,
             tm_time->tm_mon + 1, tm_time->tm_mday, tm_time->tm_hour,
             tm_time->tm_min, tm_time->tm_sec);
    return buf;
}