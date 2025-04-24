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
class Connection : public std::enable_shared_from_this<Connection> {
   private:
    // -- 基础资源 --
    SocketPtr socket_;
    ChannelPtr channel_;
    Eventloop *loop_;  // 所处的事件循环

    // -- buffer --
    Buffer input_buffer_;
    Buffer output_buffer_;

    // -- 心跳机制 --
    Timestamp lastEventTime_ = Timestamp::now();
    AtomicBool disconnected_ = false;

    // -- 回调 TcpServer --
    MessageCallback message_callback_;
    ConnectionEventCallback send_complete_callback_;
    ConnectionEventCallback close_callback_;
    ConnectionEventCallback error_callback_;

   public:
    Connection(Eventloop *loop, SocketPtr clientSocket);
    ~Connection() = default;

    // 向客户端发送数据
    void send(std::string &&message);
    // I/O线程中自动添加报文头后，注册写事件;
    // message生命周期不确定，需要使用智能指针
    void sendInIO(std::shared_ptr<std::string> message);

    bool isTimeout(time_t now, int maxGap);

    // -- getter --
    int fd() const;
    std::string ip() const;
    uint16_t port() const;

    // -- setter --
    void setMessageCallback(MessageCallback cb);
    void setSendCompleteCallback(EventCallback cb);
    void setCloseCallback(EventCallback cb);
    void setErrorCallback(EventCallback cb);

    // 被 channel 回调，并会进一步回调 TcpServer
    void handleMessage();
    void handleWritable();
    void handleClose();
    void handleError();

   private:
    bool isReadFinished(int n);
};



#endif  // !CONNECTION