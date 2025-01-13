#ifndef CONNECTION
#define CONNECTION
#include "Socket.h"
#include "Channel.h"

class Eventloop;
class Channel;

class Connection
{
private:
    Eventloop *loop_; // 被loop中epoll监视，不可delete loop

    Socket *clientSocket_;   // 管理, 析构函数中释放
    Channel *clientChannel_; // 管理, 析构函数中释放

public:
    Connection(Eventloop *loop, Socket *clientSocket);
    ~Connection();
    
    void onmessage(); // 读事件 handler，需要注册到channel中。客户端发消息

};

#endif // !CONNECTION