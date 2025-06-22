#ifndef CONNECTION
#define CONNECTION
#include <atomic>
#include <functional>
#include <memory>

#include "../../types.h"
#include "../../base/Buffer.h"
#include "../Event/Channel.h"
#include "Socket.h"
#include "../../base/Timestamp.h"

// 封装与客户端的连接
class Connection : public std::enable_shared_from_this<Connection> {
   private:
    // -- 基础资源 --
    SocketPtr socket_;
    ChannelPtr channel_;

    // -- buffer --
    Buffer input_buffer_;
    Buffer output_buffer_;

    Timestamp lastEventTime_ = Timestamp::now();
    AtomicBool disconnected_ = false;

    // -- 回调 TcpServer --
    MessageHandler handle_message_;
    ConnectionEventHandler handle_send_complete_;
    ConnectionEventHandler handle_close_;
    ConnectionEventHandler handle_error_;

   public:
    Eventloop *loop_;  // 所处的事件循环
    Connection(Eventloop *loop, SocketPtr clientSocket);
    ~Connection();
    // ~Connection() = default;

    void enable();

    void postSend(std::string &&message);
    void prepareSend(MessagePtr &&message);

    bool isTimeout(time_t now, int maxGap);

    // -- getter --
    int fd() const;
    std::string ip() const;
    uint16_t port() const;

    // -- setter --
    void setMessageHandler(MessageHandler cb);
    void setSendCompleteHandler(ConnectionEventHandler cb);
    void setCloseHandler(ConnectionEventHandler cb);
    void setErrorHandler(ConnectionEventHandler cb);

    // -- handler --
    void onMessage();
    void onWritable();
    void onClose();
    void onError();
};

#endif  // !CONNECTION