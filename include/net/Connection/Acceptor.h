#ifndef ACCEPTOR
#define ACCEPTOR
#include <errno.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <functional>

#include "Channel.h"
#include "Socket.h"
#include "types.h"

int newListenFd();

// 监听新连接，将其封装为Socket后抛给TcpServer处理
class Acceptor {
   private:
    int backlog_;
    Socket socket_;

    Channel channel_;
    Eventloop *loop_;

    // 回调 TcpServer
    NewConncetionCallBack new_connection_callback_;

   public:
    Acceptor(const std::string &ip, uint16_t port, Eventloop *loop,
             int backlog);
    ~Acceptor() = default;

    // 开始监听
    void listen();

    // 被 channel 回调，调用 accept 得到一个 fd，并构造为 Socket，回调TcpServer
    void handleNewConnection();

    // -- setter --
    void setNewConnectionCallBack(NewConncetionCallBack fn);
};

#endif  // !ACCEPTOR