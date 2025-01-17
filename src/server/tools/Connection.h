#ifndef CONNECTION
#define CONNECTION
#include <functional>
#include <memory>
#include <atomic>
#include "Socket.h"
#include "Channel.h"
#include "Buffer.h"

class Eventloop;
class Channel;
class Connection;

using conn_sptr = std::shared_ptr<Connection>;

class Connection : public std::enable_shared_from_this<Connection>
{
private:
    Eventloop *loop_; // 被loop中epoll监视，不可delete loop

    std::unique_ptr<Socket> clientSocket_;  
    std::unique_ptr<Channel> clientChannel_; 

    std::function<void(conn_sptr, std::string &)> onmessage_cb_;
    std::function<void(conn_sptr)> sendComplete_cb_;
    std::function<void(conn_sptr)> close_cb_;
    std::function<void(conn_sptr)> error_cb_;

    Buffer inputBuffer_;
    Buffer outputBuffer_;

    std::atomic_bool isDisconnected_;

public:
    Connection(Eventloop *loop, std::unique_ptr<Socket> clientSocket);
    ~Connection();

    int fd() const;
    std::string ip() const;
    uint16_t port() const;

    void setOnmessage_cb(std::function<void(conn_sptr, std::string &)> fn);
    void setSendComplete_cb(std::function<void(conn_sptr)> fn);
    void setClose_cb(std::function<void(conn_sptr)> fn);
    void setError_cb(std::function<void(conn_sptr)> fn);

    // 以下为事件 handler, 需要注册到 channel 中被回调
    void onmessage();  // 读事件 handler，客户端发消息
    void onWritable(); // 写事件 handler，尝试发送缓冲区中数据
    void onClose();    // 断开连接 handler，会回调服务端的 onClose
    void onError();    // 错误 handler，会回调服务端的 onError

    // 使用发送缓冲区添加报文头后发送数据
    void send(const std::string &message);
};

#endif // !CONNECTION