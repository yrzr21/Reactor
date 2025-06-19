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

#include "../types.h"

class ThreadPool {
   private:
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
    bool shouldWakeUp();
    bool shouldStop();

   public:
    ThreadPool(size_t nThreads, ConstStr &threadType);
    ~ThreadPool();

    size_t size();

    void addTask(Task &&task);
    void stopAll();
};

#endif  // !THREADPOOL