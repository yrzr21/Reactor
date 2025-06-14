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
    using const_str = const std::string;

   private:
    ThreadVec thread_pool_;
    TaskQueue task_queue_;

    Mutex mutex_;
    CondVar condition_;
    AtomicBool is_stop_;

    // for debug
    const std::string threadType_;

   private:
    // 太长了，还是不用lambda了
    void workerRoutine();
    void shouldWakeUp();
    void shouldStop();

   public:
    ThreadPool(size_t nThreads, const std::string &threadType);
    ~ThreadPool();

    size_t size();

    void addTask(Task &&task);
    void stopAll();
};

#endif  // !THREADPOOL