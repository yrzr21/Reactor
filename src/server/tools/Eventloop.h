#include <vector>
#ifndef EVENTLOOP
#define EVENTLOOP
#include "Epoll.h"

class Epoll;   // 需要前置声明
class Channel; // 需要前置声明

class Eventloop
{
private:
    Epoll *ep_ = nullptr;

public:
    Eventloop(); // 创建 Epoll
    ~Eventloop();

    void run();
    void updateChannel(Channel *ch); // 把channel中的event加入红黑树中或修改channel
};

#endif // !EVENTLOOP