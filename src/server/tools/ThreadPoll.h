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

class ThreadPoll
{
    using lk = std::unique_lock<std::mutex>;

private:
    std::vector<std::thread> threads_;

    std::queue<std::function<void()>> tasks_;
    std::mutex mutex_;
    std::condition_variable condition_;

    std::atomic_bool isStop_;

private:
    void thread_func();

public:
    ThreadPoll(size_t nThreads);
    ~ThreadPoll();

    void addTask(std::function<void()> task);
};

#endif // !THREADPOOL