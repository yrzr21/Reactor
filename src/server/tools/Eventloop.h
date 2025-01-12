#include <vector>
#include "Epoll.h"

class Eventloop
{
private:
    Epoll *ep_= nullptr;

public:
    Eventloop(); // 创建 Epoll
    ~Eventloop();

    void run();
    Epoll* ep();
};