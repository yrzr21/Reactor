#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t nThreads, const_str &threadType)
    : is_stop_(false), threadType_(threadType) {
    for (size_t i = 0; i < nThreads; i++) {
        thread_pool_.emplace_back([this]() { this->workerRoutine() });
    }
}

ThreadPool::~ThreadPool() { stopAll(); }

void ThreadPool::addTask(Task &&task) {
    {
        UniqueLock lock(mutex_);
        task_queue_.push(task);
    }
    condition_.notify_one();
}

void ThreadPool::stopAll() {
    {
        UniqueLock lock(mutex_);
        if (is_stop_) return;
    }

    is_stop_ = true;
    condition_.notify_all();

    for (auto &t : thread_pool_) t.join();
}

size_t ThreadPool::size() { return thread_pool_.size(); }

// 线程函数，阻塞在 wait 上
void ThreadPool::workerRoutine() {
    // log
    printf("create %s thread(%ld).\n", threadType_.c_str(),
           syscall(SYS_gettid));

    while (!is_stop_) {
        Task task;

        // fetch
        {
            UniqueLock lock(mutex_);

            condition_.wait(lock, [this] { return shouldWakeUp(); });
            if (shouldStop()) return;

            task = std::move(task_queue_.front());
            task_queue_.pop();
        }

        // run
        task();
    }
}

void ThreadPool::shouldWakeUp() { return is_stop_ || !task_queue_.empty(); }

void ThreadPool::shouldStop() { return is_stop_ && task_queue_.empty(); }
