#ifndef CONNECTION
#define CONNECTION
#include <atomic>
#include <functional>
#include <memory>

#include "../../base/Buffer/MonoRecyclePool.h"
#include "../../base/Buffer/RecvBuffer.h"
#include "../../base/Buffer/SendBuffer.h"
#include "../../base/ServiceProvider.h"
#include "../../base/Timestamp.h"
#include "../../types.h"
#include "../Event/Channel.h"
#include "Socket.h"

struct ConnectionConfig {
    Eventloop *loop;
    SocketPtr clientSocket;
};

// 封装与客户端的连接
class Connection : public std::enable_shared_from_this<Connection> {
   private:
    // -- 基础资源 --
    SocketPtr socket_;
    ChannelPtr channel_;

    // -- buffer --
    std::optional<RecvBuffer> input_buffer_;
    std::optional<SendBuffer> output_buffer_;

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
    Connection(ConnectionConfig &&config);
    ~Connection();
    // ~Connection() = default;

    void enable();
    void initBuffer(RecvBufferConfig config);

    template <typename T>  // MsgView/MsgVec, MsgVec可选择合并或分裂
    void postSend(T &&message, bool split = true);

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

   private:
    void enableWrite();  // postSend 会把它交给io线程执行
};

#endif  // !CONNECTION