#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t nThreads, ConstStr &threadType)
    : is_stop_(false), threadType_(threadType) {
    {
        UniqueLock lock(mutex_);  // 防乱序+并发造成的错误
        running_ = nThreads;
    }
    for (size_t i = 0; i < nThreads; i++) {
        thread_pool_.emplace_back([this]() { this->workerRoutine(); });
    }
}

ThreadPool::~ThreadPool() { stopAll(); }

// static constexpr size_t BATCH_SIZE = 50;
void ThreadPool::addTask(Task &&task, bool urgent) {
    static size_t max_task_size = 0;
    size_t to_wakeup;
    {
        UniqueLock lock(mutex_);
        task_queue_.push(std::move(task));

        // 1 个任务也应唤醒
        size_t want = (task_queue_.size() + BATCH_SIZE - 1) / BATCH_SIZE;
        to_wakeup = want > running_ ? want - running_ : 0;
        // if (task_queue_.size() > max_task_size) {
        //     std::cout << "max task size=" << task_queue_.size() << std::endl;
        //     max_task_size = std::max(max_task_size, task_queue_.size());
        // }

        // std::cout << "want=" << want << ", to_wakeup=" << to_wakeup
        //           << ", running=" << running_ << ", urgent=" << urgent
        //           << ", task size=" << task_queue_.size() << std::endl;
    }

    if (urgent)
        condition_.notify_all();
    else
        for (size_t i = 0; i < to_wakeup; i++) condition_.notify_one();
}

void ThreadPool::stopAll() {
    {
        UniqueLock lock(mutex_);
        if (is_stop_) return;
        // std::cout << "stop all" << std::endl;
        is_stop_ = true;
        condition_.notify_all();
    }
    // if (is_stop_.load(std::memory_order_acquire)) return;

    for (auto &t : thread_pool_) t.join();
}

size_t ThreadPool::size() { return thread_pool_.size(); }

// 线程函数，阻塞在 wait 上
void ThreadPool::workerRoutine() {
    // log
    // printf("create %s thread(%ld).\n", threadType_.c_str(),
    //        std::this_thread::get_id());

    while (!is_stop_) {
        Task task;

        // fetch
        {
            UniqueLock lock(mutex_);

            running_--;
            // std::cout << "tid=" << std::this_thread::get_id()
            //           << ", running=" << running_ << std::endl;
            condition_.wait(lock, [this] {
                bool res = shouldWakeUp();
                if (res) running_++;
                return res;
            });
            if (shouldStop()) return;
            // std::cout << "wakeup running=" << running_ << std::endl;

            task = std::move(task_queue_.front());
            task_queue_.pop();
        }

        // run
        task();
    }
}

bool ThreadPool::shouldWakeUp() { return is_stop_ || !task_queue_.empty(); }

bool ThreadPool::shouldStop() { return is_stop_ && task_queue_.empty(); }
