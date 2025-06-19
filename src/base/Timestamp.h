#ifndef TIMESTAMP
#define TIMESTAMP
#include <string>

class Timestamp
{
private:
    time_t secSinceEpoch_; // 1970 到现在的秒数

public:
    Timestamp();                      // 初始化为当前时间
    Timestamp(int64_t secSinceEpoch); // 初始化为特定时间

    static Timestamp now(); // 返回当前时间的Timestamp对象。

    time_t toint() const;         // 返回整数表示的时间。
    std::string tostring() const; // 返回字符串表示的时间，格式：yyyy-mm-dd hh24:mi:ss
};

#endif // !TIMESTAMP