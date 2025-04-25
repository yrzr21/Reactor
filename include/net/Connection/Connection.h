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

    void postSend(std::string &&message);
    void prepareSend(MessagePtr message);

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

    // -- handler --
    void handleMessage();
    void handleWritable();
    void handleClose();
    void handleError();
};



#endif  // !CONNECTION