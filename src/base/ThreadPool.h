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
#include <semaphore>
#include <thread>
#include <utility>
#include <vector>

#include "../types.h"

class ThreadPool {
    static constexpr size_t BATCH_SIZE = 5;

   private:
    ThreadVec thread_pool_;
    TaskQueue task_queue_;

    Mutex mutex_;
    size_t running_;  // 读写在临界区内

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

    // urgent 模式下，一个任务唤醒一个线程，否则批量唤醒
    void addTask(Task &&task, bool urgent = false);
    void stopAll();
};

#endif  // !THREADPOOL