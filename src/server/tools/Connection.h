#ifndef CONNECTION
#define CONNECTION
#include <functional>
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

    std::function<void(Connection *connection)> close_cb_;
    std::function<void(Connection *connection)> error_cb_;

public:
    Connection(Eventloop *loop, Socket *clientSocket);
    ~Connection();

    int fd() const;
    std::string ip() const;
    uint16_t port() const;

    void setClose_cb(std::function<void(Connection *connection)> fn);
    void setError_cb(std::function<void(Connection *connection)> fn);

    void onmessage(); // 读事件 handler，需要注册到channel中。客户端发消息
    void onClose();   // 断开连接 handler，需要注册到 channel 中，会回调服务端的 onClose
    void onError();   // 错误 handler，需要注册到 channel 中，会回调服务端的 onError
};

#endif // !CONNECTION