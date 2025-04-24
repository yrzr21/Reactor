#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t nThreads, const std::string &threadType)
    : isStop_(false), threadType_(threadType) {
    for (size_t ii = 0; ii < nThreads; ii++)
        threads_.emplace_back(std::bind(&ThreadPool::thread_func, this));
}

ThreadPool::~ThreadPool() { this->stopAll(); }

void ThreadPool::addTask(Task task) {
    {
        UniqueLock lock(this->mutex_);
        this->tasks_.push(task);
    }
    this->condition_.notify_one();
}

void ThreadPool::stopAll() {
    if (this->isStop_) return;
    this->isStop_ = true;
    this->condition_.notify_all();
    for (auto &t : this->threads_) t.join();
}

size_t ThreadPool::size() { return this->threads_.size(); }

// 线程的启动函数，阻塞在 wait 上
void ThreadPool::thread_func()  // 用 lambda 函数也可以
{
    // 显示线程ID
    printf("create %s thread(%ld).\n", this->threadType_.c_str(),
           syscall(SYS_gettid));
    // std::cout << "子线程：" << std::this_thread::get_id() << std::endl; //
    // C++11库分配的id

    while (!isStop_) {
        Task task;
        // fetch task
        {
            UniqueLock lock(this->mutex_);

            // wait & check
            this->condition_.wait(lock, [this] {
                return ((this->isStop_ == true) ||
                        (this->tasksWithName_.empty() == false));
            });
            if ((this->isStop_ == true) &&
                (this->tasksWithName_.empty() == true))
                return;

            task = std::move(this->tasksWithName_.front());
            this->tasksWithName_.pop();
        }

        // run
        task.first();
    }
}
