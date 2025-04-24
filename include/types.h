#ifndef TYPES
#define TYPES

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <string>

// 前向声明
class Eventloop;
class Channel;
class Connection;
class Epoll;

// 智能指针别名
using SocketPtr = std::unique_ptr<Socket>;
using ChannelPtr = std::unique_ptr<Channel>;
using EpollPtr = std::unique_ptr<Epoll>;
using ConnectionPtr = std::shared_ptr<Connection>;
using LoopPtr = std::unique_ptr<Eventloop>;

// 基础类型别名
using Task = std::function<void()>;
using TaskQueue = std::queue<Task>;
using Mutex = std::mutex;
using ConnectionMap = std::map<int, ConnectionPtr>;
using AtomicBool = std::atomic_bool;

// 回调函数别名
using MessageCallback = std::function<void(ConnectionPtr, std::string &)>;
using ChannelCallback = std::function<void()>;
using EventCallback = std::function<void(ConnectionPtr)>;
using TimerCallback = std::function<void(int)>;
using TimeoutCallback = std::function<void(class Eventloop *)>;
using NewConncetionCallBack = std::function<void(SocketPtr)>;

// 事件处理类型
enum class HandlerType { Readable, Writable, Closed, Error };

// epoll 操作
enum class EpollOp : int {
    Add = EPOLL_CTL_ADD,
    Modify = EPOLL_CTL_MOD,
    Del = EPOLL_CTL_DEL
};

#endif  // !TYPES
