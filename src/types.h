#ifndef TYPES
#define TYPES

#include <sys/epoll.h>
#include <sys/uio.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <memory_resource>
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
class AutoReleasePool;
class MsgView;
class Header;

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
using ConnectionMapVector = std::vector<std::map<int, ConnectionPtr>>;
using MutexPtrVector = std::vector<std::unique_ptr<std::mutex>>;
using AtomicBool = std::atomic_bool;
using IntVector = std::vector<int>;
using Seconds = std::chrono::seconds;
using MutexGuard = std::lock_guard<std::mutex>;
using PChannelVector = std::vector<Channel *>;

using MemoryResource = std::pmr::memory_resource;
using PoolOptions = std::pmr::pool_options;
using MonotonicPool = std::pmr::monotonic_buffer_resource;
using SynchronizedPool = std::pmr::synchronized_pool_resource;
using UnsynchronizedPool = std::pmr::unsynchronized_pool_resource;
using MsgPoolPtr = std::unique_ptr<AutoReleasePool>;

// MsgView本身就是一个指针，所以存自身就行
using MsgVec = std::vector<MsgView>;
using MsgDeq = std::deque<MsgView>;
// 而 header 是实体，需要存指针
using HeaderVec = std::vector<Header *>;
using HeaderDeq = std::deque<Header *>;
using IoVecs = std::vector<iovec>;

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
