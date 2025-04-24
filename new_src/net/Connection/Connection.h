#ifndef CONNECTION
#define CONNECTION
#include <atomic>
#include <functional>
#include <memory>

#include "Buffer.h"
#include "Channel.h"
#include "Socket.h"
#include "Timestamp.h"
#include "types.h"

// 封装与客户端的连接
template <uint16_t BUFFER_TYPE = 1>
class Connection : public std::enable_shared_from_this<Connection> {
   private:
    // -- 基础资源 --
    SocketPtr socket_;
    ChannelPtr channel_;
    Eventloop *loop_;  // 所处的事件循环

    // -- buffer --
    Buffer inputBuffer_ = BUFFER_TYPE;
    Buffer outputBuffer_ = BUFFER_TYPE;

    // -- 心跳机制 --
    Timestamp lastEventTime_ = Timestamp::now();
    std::atomic_bool isDisconnected_ = false;

    // -- 回调 TcpServer --
    MessageCallback onMessage_;
    EventCallback onSendComplete_;
    EventCallback onClose_;
    EventCallback onError_;

   public:
    Connection(Eventloop *loop, SocketPtr clientSocket);
    ~Connection() = default;

    // 向客户端发送数据
    void send(std::string &&message);
    // I/O线程中自动添加报文头后，注册写事件;
    // message生命周期不确定，需要使用智能指针
    void sendInIO(std::shared_ptr<std::string> message);

    bool isTimeout(time_t now, int maxGap);

    int fd() const;
    std::string ip() const;
    uint16_t port() const;

    void setOnmessage_cb(MessageCallback fn);
    void setSendComplete_cb(EventCallback fn);
    void setClose_cb(EventCallback fn);
    void setError_cb(EventCallback fn);

    // 被 channel 回调，并会进一步回调 TcpServer
    void handleMessage();
    void handleWritable();
    void handleClose();
    void handleError();

   private:
    bool isReadFinished(int n);
};

#endif  // !CONNECTION