#ifndef CONNECTION
#define CONNECTION
#include <functional>
#include "Socket.h"
#include "Channel.h"
#include "Buffer.h"

class Eventloop;
class Channel;

class Connection
{
private:
    Eventloop *loop_; // 被loop中epoll监视，不可delete loop

    Socket *clientSocket_;   // 管理, 析构函数中释放
    Channel *clientChannel_; // 管理, 析构函数中释放

    std::function<void(Connection *, std::string)> onmessage_cb_;
    std::function<void(Connection *)> sendComplete_cb_;
    std::function<void(Connection *)> close_cb_;
    std::function<void(Connection *)> error_cb_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;

public:
    Connection(Eventloop *loop, Socket *clientSocket);
    ~Connection();

    int fd() const;
    std::string ip() const;
    uint16_t port() const;

    void setOnmessage_cb(std::function<void(Connection *, std::string)> fn);
    void setSendComplete_cb(std::function<void(Connection *)> fn);
    void setClose_cb(std::function<void(Connection *)> fn);
    void setError_cb(std::function<void(Connection *)> fn);

    // 以下为事件 handler, 需要注册到 channel 中被回调
    void onmessage();  // 读事件 handler，客户端发消息
    void onWritable(); // 写事件 handler，尝试发送缓冲区中数据
    void onClose();    // 断开连接 handler，会回调服务端的 onClose
    void onError();    // 错误 handler，会回调服务端的 onError

    // 使用发送缓冲区添加报文头后发送数据
    void send(const std::string &message);
};

#endif // !CONNECTION