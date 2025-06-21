#ifndef TYPES
#define TYPES

#include <sys/epoll.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

// 前向声明
class Eventloop;
class Channel;
class Connection;
class Epoll;
class Socket;

// 指针
using SocketPtr = std::unique_ptr<Socket>;
using ChannelPtr = std::unique_ptr<Channel>;
using EpollPtr = std::unique_ptr<Epoll>;
using ConnectionPtr = std::shared_ptr<Connection>;
using LoopPtr = std::unique_ptr<Eventloop>;
using MessagePtr = std::unique_ptr<std::string>;

using Task = std::move_only_function<void()>;
using TaskQueue = std::queue<Task>;
using Mutex = std::mutex;
using UniqueLock = std::unique_lock<Mutex>;
using CondVar = std::condition_variable;
using Thread = std::thread;
using ThreadVec = std::vector<Thread>;
using ConstStr = const std::string;
using ConnectionMap = std::map<int, ConnectionPtr>;
using AtomicBool = std::atomic_bool;
using IntVector = std::vector<int>;
using Seconds = std::chrono::seconds;
using MutexGuard = std::lock_guard<std::mutex>;
using PChannelVector = std::vector<Channel *>;

// 回调函数别名
using MessageHandler = std::function<void(ConnectionPtr, MessagePtr)>;
using ChannelEventHandler = std::function<void()>;
using ConnectionEventHandler = std::function<void(ConnectionPtr)>;
using TimerHandler = std::function<void(IntVector &)>;
using LoopTimeoutHandler = std::function<void(Eventloop *)>;
using NewConnectionHandler = std::function<void(SocketPtr)>;

// 事件处理类型
enum class HandlerType { Readable, Writable, Closed, Error };

#endif  // !TYPES
