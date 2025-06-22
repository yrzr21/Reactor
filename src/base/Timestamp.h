#ifndef TIMESTAMP
#define TIMESTAMP
#include <time.h>

#include <string>

class Timestamp {
   private:
    time_t second_;  // 1970 到现在的秒数

   public:
    Timestamp() : second_(time(0)) {}
    Timestamp(int64_t second) : second_(second) {}

    static Timestamp now() { return Timestamp(time(0)); }

    time_t toint() const;  // 返回整数表示的时间。

    // 返回字符串表示的时间，格式：yyyy-mm-dd hh24:mi:ss
    std::string tostring() const;
};

#endif  // !TIMESTAMP