#ifndef THREADPOOL
#define THREADPOOL
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unistd.h>      // syscall
#include <sys/syscall.h> // syscall
#include <iostream>
#include <utility>

class ThreadPool
{
    using lk_t = std::unique_lock<std::mutex>;
    // 一个可调用对象+它的名字组成的pair
    using tsk_t = std::pair<std::function<void()>, std::string>;

private:
    std::vector<std::thread> threads_;

    // std::queue<std::function<void()>> tasks_;
    std::queue<tsk_t> tasksWithName_;
    std::mutex mutex_;
    std::condition_variable condition_;

    std::atomic_bool isStop_;
    const std::string threadType_;

private:
    void thread_func();

public:
    ThreadPool(size_t nThreads, const std::string &threadType);
    ~ThreadPool();

    // void addTask(std::function<void()> task);
    void addTask(std::function<void()> task, const std::string &taskName);
    size_t size();
};

#endif // !THREADPOOL