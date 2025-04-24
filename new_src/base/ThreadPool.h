#ifndef THREADPOOL
#define THREADPOOL
#include <sys/syscall.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

class ThreadPool {
   private:
    using Mutex = std::mutex;
    using UniqueLock = std::unique_lock<Mutex>;
    using CondVar = std::condition_variable;
    using Thread = std::thread;
    using ThreadVec = std::vector<Thread>;
    using Task = std::function<void()>;
    using TaskQueue = std::queue<Task>;
    using AtomicBool = std::atomic_bool;

   private:
    ThreadVec threads_;
    TaskQueue tasks_;
    Mutex mutex_;
    CondVar condition_;

    AtomicBool isStop_;

    // for debug
    const std::string threadType_;

   private:
    // 太长了，还是不用lambda了
    void thread_func();

   public:
    ThreadPool(size_t nThreads, const std::string &threadType);
    ~ThreadPool();

    size_t size();

    void addTask(Task task);
    void stopAll();
};

#endif  // !THREADPOOL